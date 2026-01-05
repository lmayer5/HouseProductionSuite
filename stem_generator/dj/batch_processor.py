"""
DJ Batch Processor

Threaded batch processing of DJ libraries with resume support.
"""

import json
import time
from concurrent.futures import ThreadPoolExecutor, as_completed
from dataclasses import dataclass
from pathlib import Path
from typing import Callable, Optional

from .library_scanner import DJLibraryScanner, ScannedTrack, ScanResult
from ..core.stem_pipeline import StemPipeline
from ..core.engines.base_engine import SeparationResult


@dataclass
class BatchProgress:
    """Progress tracking for batch processing."""
    total: int = 0
    completed: int = 0
    failed: int = 0
    skipped: int = 0
    
    @property
    def remaining(self) -> int:
        return self.total - self.completed - self.failed - self.skipped
    
    @property
    def percent_complete(self) -> float:
        if self.total == 0:
            return 0.0
        return ((self.completed + self.skipped) / self.total) * 100


@dataclass
class BatchResult:
    """Result of a batch processing operation."""
    progress: BatchProgress
    results: list[tuple[ScannedTrack, SeparationResult]]
    errors: list[tuple[ScannedTrack, str]]
    processing_time_seconds: float


ProgressCallback = Callable[[BatchProgress, ScannedTrack], None]


class DJBatchProcessor:
    """
    Threaded batch processor for stem separation.
    
    Features:
        - ThreadPoolExecutor for parallel processing
        - Resume support (skips already processed files)
        - Progress callbacks for UI integration
        - Respects GPU batch size recommendations
    """
    
    def __init__(
        self,
        pipeline: Optional[StemPipeline] = None,
        scanner: Optional[DJLibraryScanner] = None,
        max_workers: Optional[int] = None
    ):
        """
        Initialize the batch processor.
        
        Args:
            pipeline: StemPipeline instance (created if not provided)
            scanner: DJLibraryScanner instance (created if not provided)
            max_workers: Maximum parallel workers (auto-determined from engine)
        """
        self.pipeline = pipeline or StemPipeline()
        self.scanner = scanner or DJLibraryScanner()
        self._max_workers = max_workers
    
    @property
    def max_workers(self) -> int:
        """Get the maximum number of parallel workers."""
        if self._max_workers:
            return self._max_workers
        
        # Use engine's recommended batch size
        try:
            return self.pipeline.demucs.get_recommended_batch_size()
        except Exception:
            return 1
    
    def _process_track(
        self,
        track: ScannedTrack,
        skip_existing: bool = True
    ) -> SeparationResult:
        """Process a single track."""
        return self.pipeline.separate(
            track.path,
            engine="auto",
            skip_if_exists=skip_existing,
            quality_fallback=True
        )
    
    def process_directory(
        self,
        directory: str | Path,
        progress_callback: Optional[ProgressCallback] = None,
        skip_existing: bool = True,
        limit: Optional[int] = None
    ) -> BatchResult:
        """
        Process all audio files in a directory.
        
        Args:
            directory: Directory containing audio files
            progress_callback: Called after each track is processed
            skip_existing: Skip tracks that already have stems
            limit: Maximum number of tracks to process
            
        Returns:
            BatchResult with processing statistics
        """
        start_time = time.time()
        
        # Scan directory
        scan_result = self.scanner.scan_directory(directory)
        tracks = scan_result.tracks
        
        if limit:
            tracks = tracks[:limit]
        
        progress = BatchProgress(total=len(tracks))
        results: list[tuple[ScannedTrack, SeparationResult]] = []
        errors: list[tuple[ScannedTrack, str]] = []
        
        # Sequential processing for GPU-bound work
        # (GPU can't truly parallelize, so we process one at a time)
        for track in tracks:
            try:
                result = self._process_track(track, skip_existing)
                results.append((track, result))
                
                if result.success:
                    if result.engine_name == "cached":
                        progress.skipped += 1
                    else:
                        progress.completed += 1
                else:
                    progress.failed += 1
                    if result.error_message:
                        errors.append((track, result.error_message))
                        
            except Exception as e:
                progress.failed += 1
                errors.append((track, str(e)))
            
            # Call progress callback
            if progress_callback:
                progress_callback(progress, track)
        
        processing_time = time.time() - start_time
        
        return BatchResult(
            progress=progress,
            results=results,
            errors=errors,
            processing_time_seconds=processing_time
        )
    
    def process_tracks(
        self,
        tracks: list[ScannedTrack],
        progress_callback: Optional[ProgressCallback] = None,
        skip_existing: bool = True
    ) -> BatchResult:
        """
        Process a list of pre-scanned tracks.
        
        Args:
            tracks: List of ScannedTrack instances to process
            progress_callback: Called after each track is processed
            skip_existing: Skip tracks that already have stems
            
        Returns:
            BatchResult with processing statistics
        """
        start_time = time.time()
        
        progress = BatchProgress(total=len(tracks))
        results: list[tuple[ScannedTrack, SeparationResult]] = []
        errors: list[tuple[ScannedTrack, str]] = []
        
        for track in tracks:
            try:
                result = self._process_track(track, skip_existing)
                results.append((track, result))
                
                if result.success:
                    if result.engine_name == "cached":
                        progress.skipped += 1
                    else:
                        progress.completed += 1
                else:
                    progress.failed += 1
                    if result.error_message:
                        errors.append((track, result.error_message))
                        
            except Exception as e:
                progress.failed += 1
                errors.append((track, str(e)))
            
            if progress_callback:
                progress_callback(progress, track)
        
        processing_time = time.time() - start_time
        
        return BatchResult(
            progress=progress,
            results=results,
            errors=errors,
            processing_time_seconds=processing_time
        )
    
    def resume_processing(
        self,
        directory: str | Path,
        progress_callback: Optional[ProgressCallback] = None
    ) -> BatchResult:
        """
        Resume processing a directory, skipping already completed files.
        
        This is equivalent to process_directory with skip_existing=True,
        but provides clearer intent for resuming interrupted jobs.
        
        Args:
            directory: Directory to process
            progress_callback: Progress callback function
            
        Returns:
            BatchResult with processing statistics
        """
        return self.process_directory(
            directory,
            progress_callback=progress_callback,
            skip_existing=True
        )
    
    def get_pending_tracks(self, directory: str | Path) -> list[ScannedTrack]:
        """
        Get tracks that haven't been processed yet.
        
        Args:
            directory: Directory to scan
            
        Returns:
            List of tracks without existing stems
        """
        scan_result = self.scanner.scan_directory(directory)
        pending = []
        
        for track in scan_result.tracks:
            if not self.pipeline.file_manager.stems_exist(track.path):
                pending.append(track)
        
        return pending
