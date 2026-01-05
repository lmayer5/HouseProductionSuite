"""
Stem Organizer for DJ Software

Organizes separated stems for Serato, Rekordbox, and other DJ software.
"""

import shutil
from dataclasses import dataclass
from pathlib import Path
from typing import Optional, Literal

from ..utils.file_manager import StemFileManager


OutputFormat = Literal["flat", "subdirectory", "mirror"]


@dataclass
class OrganizeResult:
    """Result of a stem organization operation."""
    success: bool
    output_paths: dict[str, Path]
    message: str


class StemOrganizer:
    """
    Organizes separated stems for DJ software compatibility.
    
    Output Formats:
        - "flat": All stems in one directory with prefix naming
        - "subdirectory": Each track gets its own subdirectory
        - "mirror": Mirrors the source library structure
        
    Naming Convention: {TrackName}_{StemType}.wav
    """
    
    STEM_COLORS = {
        "vocals": "Pink",
        "drums": "Green", 
        "bass": "Blue",
        "other": "Orange"
    }
    
    def __init__(
        self,
        file_manager: Optional[StemFileManager] = None,
        output_format: OutputFormat = "subdirectory"
    ):
        """
        Initialize the stem organizer.
        
        Args:
            file_manager: StemFileManager instance
            output_format: How to organize output files
        """
        self.file_manager = file_manager or StemFileManager()
        self.output_format = output_format
    
    def _sanitize_name(self, name: str) -> str:
        """Sanitize a name for use in filenames."""
        # Remove invalid characters
        invalid_chars = '<>:"/\\|?*'
        for char in invalid_chars:
            name = name.replace(char, '_')
        return name.strip('. ')[:100]
    
    def _get_track_name(self, source_path: Path) -> str:
        """Get a clean track name from metadata or filename."""
        metadata = self.file_manager.extract_metadata(source_path)
        
        if metadata.artist != "Unknown Artist" and metadata.title != "Unknown Title":
            return f"{self._sanitize_name(metadata.artist)} - {self._sanitize_name(metadata.title)}"
        
        return self._sanitize_name(source_path.stem)
    
    def organize_stems(
        self,
        source_path: Path,
        output_dir: Path,
        format_override: Optional[OutputFormat] = None
    ) -> OrganizeResult:
        """
        Organize stems for a single track.
        
        Args:
            source_path: Path to the original audio file
            output_dir: Target directory for organized stems
            format_override: Override the default output format
            
        Returns:
            OrganizeResult with organized stem paths
        """
        output_format = format_override or self.output_format
        stem_dir = self.file_manager.get_output_dir(source_path)
        
        # Check if stems exist
        if not self.file_manager.stems_exist(source_path):
            return OrganizeResult(
                success=False,
                output_paths={},
                message=f"Stems not found for {source_path}"
            )
        
        track_name = self._get_track_name(source_path)
        output_paths = {}
        
        try:
            output_dir = Path(output_dir)
            
            if output_format == "subdirectory":
                # Create subdirectory for this track
                track_dir = output_dir / track_name
                track_dir.mkdir(parents=True, exist_ok=True)
                
                for stem_name in self.file_manager.STEM_NAMES:
                    src = stem_dir / f"{stem_name}.wav"
                    dst = track_dir / f"{stem_name}.wav"
                    shutil.copy2(src, dst)
                    output_paths[stem_name] = dst
                    
            elif output_format == "flat":
                # All stems in one directory with prefix
                output_dir.mkdir(parents=True, exist_ok=True)
                
                for stem_name in self.file_manager.STEM_NAMES:
                    src = stem_dir / f"{stem_name}.wav"
                    dst = output_dir / f"{track_name}_{stem_name}.wav"
                    shutil.copy2(src, dst)
                    output_paths[stem_name] = dst
                    
            elif output_format == "mirror":
                # Mirror source structure (use source's parent folder name)
                parent_name = source_path.parent.name
                mirrored_dir = output_dir / parent_name / track_name
                mirrored_dir.mkdir(parents=True, exist_ok=True)
                
                for stem_name in self.file_manager.STEM_NAMES:
                    src = stem_dir / f"{stem_name}.wav"
                    dst = mirrored_dir / f"{stem_name}.wav"
                    shutil.copy2(src, dst)
                    output_paths[stem_name] = dst
            
            return OrganizeResult(
                success=True,
                output_paths=output_paths,
                message=f"Organized {len(output_paths)} stems for {track_name}"
            )
            
        except Exception as e:
            return OrganizeResult(
                success=False,
                output_paths=output_paths,
                message=f"Error organizing stems: {e}"
            )
    
    def organize_directory(
        self,
        source_dir: Path,
        output_dir: Path,
        format_override: Optional[OutputFormat] = None
    ) -> list[OrganizeResult]:
        """
        Organize all processed stems in a directory.
        
        Args:
            source_dir: Directory containing original audio files
            output_dir: Target directory for organized stems
            format_override: Override the default output format
            
        Returns:
            List of OrganizeResult for each track
        """
        results = []
        source_dir = Path(source_dir)
        
        # Find all audio files that have been processed
        audio_extensions = {'.mp3', '.wav', '.flac', '.aiff', '.aif', '.m4a', '.ogg'}
        
        for path in source_dir.rglob("*"):
            if path.is_file() and path.suffix.lower() in audio_extensions:
                if self.file_manager.stems_exist(path):
                    result = self.organize_stems(path, output_dir, format_override)
                    results.append(result)
        
        return results
    
    def create_rekordbox_structure(
        self,
        source_dir: Path,
        output_dir: Path
    ) -> list[OrganizeResult]:
        """
        Create Rekordbox-friendly stem organization.
        
        Rekordbox works well with a flat structure where stems
        are in the same folder as the original tracks.
        
        Args:
            source_dir: Directory with original audio
            output_dir: Target directory
            
        Returns:
            List of organization results
        """
        return self.organize_directory(source_dir, output_dir, "flat")
    
    def create_serato_structure(
        self,
        source_dir: Path,
        output_dir: Path
    ) -> list[OrganizeResult]:
        """
        Create Serato-friendly stem organization.
        
        Serato Stem feature expects stems in subdirectories.
        
        Args:
            source_dir: Directory with original audio
            output_dir: Target directory
            
        Returns:
            List of organization results
        """
        return self.organize_directory(source_dir, output_dir, "subdirectory")
