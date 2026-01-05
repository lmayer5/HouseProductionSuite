"""
File Manager for Stem Generation System

Handles standardized input/output paths for stem separation.
"""

import hashlib
import logging
import os
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Optional

import music_tag


logger = logging.getLogger(__name__)


@dataclass
class TrackMetadata:
    """Metadata extracted from an audio file."""
    artist: str = "Unknown Artist"
    title: str = "Unknown Title"
    bpm: Optional[float] = None
    key: Optional[str] = None
    genre: Optional[str] = None
    file_hash: str = ""


class StemFileManager:
    """
    Manages file paths and organization for stem separation.
    
    Output Structure:
        data/stems/{Artist} - {Title}_{Hash}/
            ├── vocals.wav
            ├── drums.wav
            ├── bass.wav
            ├── other.wav
            └── metadata.json
    """
    
    STEM_NAMES = ["vocals", "drums", "bass", "other"]
    
    def __init__(self, base_dir: str = "data/stems"):
        """
        Initialize the file manager.
        
        Args:
            base_dir: Base directory for stem output (relative or absolute)
        """
        self.base_dir = Path(base_dir).resolve()  # Use absolute path
        self.base_dir.mkdir(parents=True, exist_ok=True)
        logger.debug(f"File manager initialized with base_dir: {self.base_dir}")
    
    def get_file_hash(self, file_path: str | Path, length: int = 8) -> str:
        """
        Calculate SHA-256 hash of a file (first 1MB for speed).
        
        Uses SHA-256 for stronger collision resistance than MD5.
        
        Args:
            file_path: Path to the audio file
            length: Number of characters to return from hash
            
        Returns:
            Truncated SHA-256 hash string
        """
        hasher = hashlib.sha256()
        with open(file_path, 'rb') as f:
            # Read first 1MB for speed on large files
            chunk = f.read(1024 * 1024)
            hasher.update(chunk)
        return hasher.hexdigest()[:length]
    
    def extract_metadata(self, file_path: str | Path) -> TrackMetadata:
        """
        Extract ID3/metadata from an audio file.
        
        Args:
            file_path: Path to the audio file
            
        Returns:
            TrackMetadata dataclass with extracted info
        """
        file_path = Path(file_path)
        metadata = TrackMetadata()
        metadata.file_hash = self.get_file_hash(file_path)
        
        try:
            tags = music_tag.load_file(file_path)
            
            if tags.get('artist'):
                metadata.artist = str(tags['artist'])
            if tags.get('title'):
                metadata.title = str(tags['title'])
            if tags.get('genre'):
                metadata.genre = str(tags['genre'])
                
            # BPM might be stored as 'bpm' or 'TBPM'
            if tags.get('bpm'):
                try:
                    metadata.bpm = float(str(tags['bpm']))
                except (ValueError, TypeError):
                    pass
                    
            # Key might be stored as 'key' or 'initialkey'
            if tags.get('key'):
                metadata.key = str(tags['key'])
                
        except Exception as e:
            # If metadata extraction fails, use filename
            logger.debug(f"Metadata extraction failed, using filename: {e}")
            metadata.title = file_path.stem
            
        return metadata
    
    def sanitize_filename(self, name: str) -> str:
        """
        Remove/replace invalid filename characters.
        
        Strictly sanitizes to prevent directory traversal attacks.
        
        Args:
            name: Original name string
            
        Returns:
            Sanitized filename-safe string
        """
        # Replace path separators and invalid chars with underscore
        sanitized = re.sub(r'[<>:"/\\|?*\x00-\x1f]', '_', name)
        # Remove any remaining path separators (belt and suspenders)
        sanitized = sanitized.replace('/', '_').replace('\\', '_')
        # Remove leading/trailing whitespace, dots, and underscores
        sanitized = sanitized.strip('. _')
        # Remove any ".." sequences
        sanitized = sanitized.replace('..', '_')
        # Limit length
        return sanitized[:100] if len(sanitized) > 100 else sanitized
    
    def get_output_dir(self, file_path: str | Path) -> Path:
        """
        Get the output directory path for a track's stems.
        
        Args:
            file_path: Path to the source audio file
            
        Returns:
            Path to the output directory for this track's stems
            
        Raises:
            ValueError: If the computed path escapes the base directory
        """
        metadata = self.extract_metadata(file_path)
        
        # Create directory name: "Artist - Title_hash"
        artist = self.sanitize_filename(metadata.artist)
        title = self.sanitize_filename(metadata.title)
        dir_name = f"{artist} - {title}_{metadata.file_hash}"
        
        output_dir = (self.base_dir / dir_name).resolve()
        
        # SECURITY: Ensure output directory is within base_dir
        if not str(output_dir).startswith(str(self.base_dir)):
            logger.error(f"Path traversal attempt detected: {dir_name}")
            raise ValueError("Invalid output directory path")
        
        output_dir.mkdir(parents=True, exist_ok=True)
        
        return output_dir
    
    def get_stem_path(self, file_path: str | Path, stem_name: str) -> Path:
        """
        Get the full path for a specific stem file.
        
        Args:
            file_path: Path to the source audio file
            stem_name: One of 'vocals', 'drums', 'bass', 'other'
            
        Returns:
            Path to the stem wav file
        """
        if stem_name not in self.STEM_NAMES:
            raise ValueError(f"Invalid stem name: {stem_name}. Must be one of {self.STEM_NAMES}")
            
        output_dir = self.get_output_dir(file_path)
        return output_dir / f"{stem_name}.wav"
    
    def get_all_stem_paths(self, file_path: str | Path) -> dict[str, Path]:
        """
        Get paths for all stems of a track.
        
        Args:
            file_path: Path to the source audio file
            
        Returns:
            Dictionary mapping stem names to their file paths
        """
        output_dir = self.get_output_dir(file_path)
        return {stem: output_dir / f"{stem}.wav" for stem in self.STEM_NAMES}
    
    def stems_exist(self, file_path: str | Path) -> bool:
        """
        Check if all stems already exist for a track.
        
        Args:
            file_path: Path to the source audio file
            
        Returns:
            True if all 4 stems exist, False otherwise
        """
        stem_paths = self.get_all_stem_paths(file_path)
        return all(path.exists() for path in stem_paths.values())
    
    def get_metadata_path(self, file_path: str | Path) -> Path:
        """
        Get the path to the metadata.json file for a track.
        
        Args:
            file_path: Path to the source audio file
            
        Returns:
            Path to the metadata.json file
        """
        output_dir = self.get_output_dir(file_path)
        return output_dir / "metadata.json"
