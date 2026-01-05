"""
Stem Pipeline - Main Orchestrator

The central routing engine that manages stem separation jobs,
selecting the appropriate engine and handling quality fallbacks.
"""

import json
import logging
import os
import time
from dataclasses import asdict
from pathlib import Path
from typing import Optional, Literal

from .engines.base_engine import SeparationResult
from .engines.demucs_engine import DemucsEngine
from .engines.lalal_engine import LalalEngine
from .quality_analyzer import StemQualityAnalyzer
from ..utils.file_manager import StemFileManager
from ..utils.database import StemDatabase, JobStatus


logger = logging.getLogger(__name__)

EngineChoice = Literal["auto", "demucs", "lalal", "uvr"]


class StemPipeline:
    """
    Main orchestrator for stem separation.
    
    Routing Logic:
        1. Check file size and GPU availability
        2. Default to Demucs for files < 50MB or standard batch processing
        3. Fall back to LALAL.AI for high-priority vocal extraction
           or if local quality check fails
        
    Quality Assurance:
        - Calculates SI-SDR for each stem after separation
        - If vocals SI-SDR < 7.0 dB, automatically retries with fallback engine
    """
    
    # File size threshold for local vs cloud processing (in bytes)
    LOCAL_SIZE_THRESHOLD = 50 * 1024 * 1024  # 50 MB
    
    def __init__(
        self,
        base_dir: str = "data/stems",
        db_path: Optional[str] = None
    ):
        """
        Initialize the stem pipeline.
        
        Args:
            base_dir: Base directory for stem output
            db_path: Path to SQLite database (defaults to base_dir/stem_generator.db)
        """
        self.file_manager = StemFileManager(base_dir)
        self.db = StemDatabase(db_path or f"{base_dir}/stem_generator.db")
        self.quality_analyzer = StemQualityAnalyzer()
        
        # Initialize engines (lazy loaded)
        self._demucs_engine: Optional[DemucsEngine] = None
        self._lalal_engine: Optional[LalalEngine] = None
    
    @property
    def demucs(self) -> DemucsEngine:
        """Get the Demucs engine (lazy initialization)."""
        if self._demucs_engine is None:
            self._demucs_engine = DemucsEngine()
        return self._demucs_engine
    
    @property
    def lalal(self) -> LalalEngine:
        """Get the LALAL.AI engine (lazy initialization)."""
        if self._lalal_engine is None:
            self._lalal_engine = LalalEngine()
        return self._lalal_engine
    
    def _select_engine(self, file_path: Path, preference: EngineChoice = "auto"):
        """
        Select the appropriate engine based on file and availability.
        
        Args:
            file_path: Path to the audio file
            preference: User preference for engine selection
            
        Returns:
            The selected engine instance
        """
        # Explicit engine selection
        if preference == "demucs":
            if not self.demucs.is_available():
                raise RuntimeError("Demucs is not available (missing dependencies)")
            return self.demucs
            
        if preference == "lalal":
            if not self.lalal.is_available():
                raise RuntimeError("LALAL.AI is not available (missing API key)")
            return self.lalal
        
        # Auto-selection logic
        file_size = file_path.stat().st_size
        
        # Prefer local Demucs for smaller files or if cloud not available
        if self.demucs.is_available():
            if file_size < self.LOCAL_SIZE_THRESHOLD or not self.lalal.is_available():
                return self.demucs
        
        # Fall back to LALAL.AI
        if self.lalal.is_available():
            return self.lalal
            
        # No engines available
        raise RuntimeError("No stem separation engines available")
    
    def _get_fallback_engine(self, current_engine):
        """Get the fallback engine if the current one produces poor results."""
        if current_engine.name.startswith("demucs") and self.lalal.is_available():
            return self.lalal
        if current_engine.name == "lalal_cloud" and self.demucs.is_available():
            return self.demucs
        return None
    
    def _save_metadata(
        self,
        file_path: Path,
        output_dir: Path,
        result: SeparationResult,
        quality_scores: dict[str, float]
    ) -> None:
        """Save processing metadata to JSON file."""
        metadata = {
            "source_file": str(file_path),
            "engine": result.engine_name,
            "processing_time_seconds": result.processing_time_seconds,
            "success": result.success,
            "quality_scores": quality_scores,
            "stems": {
                name: str(path) for name, path in result.stem_paths.items()
            }
        }
        
        metadata_path = output_dir / "metadata.json"
        with open(metadata_path, 'w') as f:
            json.dump(metadata, f, indent=2)
    
    def separate(
        self,
        file_path: str | Path,
        engine: EngineChoice = "auto",
        skip_if_exists: bool = True,
        quality_fallback: bool = True
    ) -> SeparationResult:
        """
        Separate an audio file into stems.
        
        Args:
            file_path: Path to the audio file
            engine: Engine selection ('auto', 'demucs', 'lalal')
            skip_if_exists: Skip if stems already exist
            quality_fallback: Retry with fallback engine if quality is poor
            
        Returns:
            SeparationResult with stem paths and metadata
        """
        file_path = Path(file_path)
        
        if not file_path.exists():
            return SeparationResult(
                success=False,
                stem_paths={},
                processing_time_seconds=0,
                engine_name="none",
                error_message=f"File not found: {file_path}"
            )
        
        # Check if already processed
        if skip_if_exists and self.file_manager.stems_exist(file_path):
            output_dir = self.file_manager.get_output_dir(file_path)
            return SeparationResult(
                success=True,
                stem_paths=self.file_manager.get_all_stem_paths(file_path),
                processing_time_seconds=0,
                engine_name="cached"
            )
        
        # Get output directory
        output_dir = self.file_manager.get_output_dir(file_path)
        
        # Extract metadata and create database record
        metadata = self.file_manager.extract_metadata(file_path)
        
        # Check if track exists in DB, create if not
        track_record = self.db.get_track_by_hash(metadata.file_hash)
        if track_record is None:
            track_id = self.db.add_track(
                file_path=str(file_path),
                file_hash=metadata.file_hash,
                artist=metadata.artist,
                title=metadata.title,
                bpm=metadata.bpm,
                key=metadata.key,
                genre=metadata.genre
            )
        else:
            track_id = track_record.id
        
        # Select engine and run separation
        selected_engine = self._select_engine(file_path, engine)
        
        # Create job record
        job_id = self.db.create_job(track_id, selected_engine.name)
        self.db.update_job_status(job_id, JobStatus.PROCESSING)
        
        # Run separation
        result = selected_engine.separate(file_path, output_dir)
        
        if not result.success:
            self.db.update_job_status(
                job_id, 
                JobStatus.FAILED, 
                result.processing_time_seconds,
                result.error_message
            )
            return result
        
        # Analyze quality
        quality_scores = self.quality_analyzer.analyze_all_stems(output_dir, file_path)
        
        # Store quality scores
        for stem_name, si_sdr in quality_scores.items():
            self.db.add_quality_score(job_id, stem_name, si_sdr)
        
        # Check if we need to fallback
        needs_fallback = False
        if quality_fallback:
            for stem_name, si_sdr in quality_scores.items():
                if self.quality_analyzer.needs_reprocessing(stem_name, si_sdr):
                    needs_fallback = True
                    break
        
        if needs_fallback:
            fallback_engine = self._get_fallback_engine(selected_engine)
            if fallback_engine:
                print(f"Quality check failed, retrying with {fallback_engine.name}")
                
                # Create new job for fallback
                fallback_job_id = self.db.create_job(track_id, fallback_engine.name)
                self.db.update_job_status(fallback_job_id, JobStatus.PROCESSING)
                
                # Run fallback separation
                fallback_result = fallback_engine.separate(file_path, output_dir)
                
                if fallback_result.success:
                    # Re-analyze quality
                    quality_scores = self.quality_analyzer.analyze_all_stems(
                        output_dir, file_path
                    )
                    for stem_name, si_sdr in quality_scores.items():
                        self.db.add_quality_score(fallback_job_id, stem_name, si_sdr)
                    
                    self.db.update_job_status(
                        fallback_job_id,
                        JobStatus.COMPLETED,
                        fallback_result.processing_time_seconds
                    )
                    result = fallback_result
                else:
                    self.db.update_job_status(
                        fallback_job_id,
                        JobStatus.FAILED,
                        fallback_result.processing_time_seconds,
                        fallback_result.error_message
                    )
        
        # Save metadata
        self._save_metadata(file_path, output_dir, result, quality_scores)
        
        # Update original job status
        self.db.update_job_status(
            job_id,
            JobStatus.COMPLETED,
            result.processing_time_seconds
        )
        
        return result
    
    def get_stats(self) -> dict:
        """
        Get pipeline statistics.
        
        Returns:
            Dictionary with processing statistics
        """
        # This is a simple implementation - could be expanded
        return {
            "base_dir": str(self.file_manager.base_dir),
            "demucs_available": self.demucs.is_available(),
            "lalal_available": self.lalal.is_available(),
        }
