"""
DJ Library Scanner

Scans DJ libraries and prioritizes tracks for stem processing.
"""

import os
from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional, Iterator
from enum import IntEnum

import music_tag


class Priority(IntEnum):
    """Priority levels for track processing."""
    HIGHEST = 1  # In specific crates
    HIGH = 2     # Genre contains "House"
    MEDIUM = 3   # BPM > 120
    NORMAL = 4   # Has detected vocals (or no metadata)
    LOW = 5      # Everything else


@dataclass
class ScannedTrack:
    """A track discovered during library scanning."""
    path: Path
    artist: str = "Unknown Artist"
    title: str = "Unknown Title"
    bpm: Optional[float] = None
    key: Optional[str] = None
    genre: Optional[str] = None
    priority: Priority = Priority.NORMAL
    crate: Optional[str] = None
    
    @property
    def display_name(self) -> str:
        return f"{self.artist} - {self.title}"


@dataclass
class ScanResult:
    """Result of a library scan operation."""
    tracks: list[ScannedTrack] = field(default_factory=list)
    total_files: int = 0
    audio_files: int = 0
    errors: list[str] = field(default_factory=list)


class DJLibraryScanner:
    """
    Scans DJ libraries and prioritizes tracks for stem processing.
    
    Priority Queue Logic:
        1. Tracks in specific "Crates" (highest priority)
        2. Genre contains "House"
        3. BPM > 120
        4. Tracks with detected vocals (approximated by genre/checks)
    
    Supported formats: MP3, WAV, FLAC, AIFF, M4A, OGG
    """
    
    AUDIO_EXTENSIONS = {'.mp3', '.wav', '.flac', '.aiff', '.aif', '.m4a', '.ogg'}
    
    # Genres that suggest house music
    HOUSE_GENRES = {'house', 'deep house', 'tech house', 'progressive house', 
                    'electro house', 'future house', 'tropical house'}
    
    # Genres that typically have vocals
    VOCAL_GENRES = {'pop', 'r&b', 'rnb', 'soul', 'hip-hop', 'hip hop', 'vocal'}
    
    def __init__(self, priority_crates: Optional[list[str]] = None):
        """
        Initialize the library scanner.
        
        Args:
            priority_crates: List of crate/folder names to prioritize
        """
        self.priority_crates = set(c.lower() for c in (priority_crates or []))
    
    def _is_audio_file(self, path: Path) -> bool:
        """Check if a file is a supported audio format."""
        return path.suffix.lower() in self.AUDIO_EXTENSIONS
    
    def _extract_metadata(self, path: Path) -> ScannedTrack:
        """Extract metadata from an audio file."""
        track = ScannedTrack(path=path)
        
        try:
            tags = music_tag.load_file(path)
            
            if tags.get('artist'):
                track.artist = str(tags['artist'])
            if tags.get('title'):
                track.title = str(tags['title'])
            else:
                track.title = path.stem
            if tags.get('genre'):
                track.genre = str(tags['genre'])
            if tags.get('bpm'):
                try:
                    track.bpm = float(str(tags['bpm']))
                except (ValueError, TypeError):
                    pass
            if tags.get('key'):
                track.key = str(tags['key'])
                
        except Exception:
            # Fall back to filename
            track.title = path.stem
        
        return track
    
    def _calculate_priority(self, track: ScannedTrack) -> Priority:
        """
        Calculate priority based on track metadata.
        
        Priority logic:
            1. In priority crates -> HIGHEST
            2. House genre -> HIGH
            3. BPM > 120 -> MEDIUM
            4. Vocal genre -> NORMAL
            5. Everything else -> LOW
        """
        # Check if in priority crate
        if track.crate and track.crate.lower() in self.priority_crates:
            return Priority.HIGHEST
        
        # Check for parent folder that might be a crate
        parent_name = track.path.parent.name.lower()
        if parent_name in self.priority_crates:
            track.crate = parent_name
            return Priority.HIGHEST
        
        # Check genre
        if track.genre:
            genre_lower = track.genre.lower()
            
            # House genre gets high priority
            if any(g in genre_lower for g in self.HOUSE_GENRES):
                return Priority.HIGH
                
            # Vocal genres get normal priority
            if any(g in genre_lower for g in self.VOCAL_GENRES):
                return Priority.NORMAL
        
        # High BPM gets medium priority
        if track.bpm and track.bpm > 120:
            return Priority.MEDIUM
        
        return Priority.LOW
    
    def scan_directory(
        self,
        directory: str | Path,
        recursive: bool = True
    ) -> ScanResult:
        """
        Scan a directory for audio files.
        
        Args:
            directory: Path to the directory to scan
            recursive: Whether to scan subdirectories
            
        Returns:
            ScanResult with found tracks sorted by priority
        """
        directory = Path(directory)
        result = ScanResult()
        
        if not directory.exists():
            result.errors.append(f"Directory not found: {directory}")
            return result
        
        # Find all files
        pattern = "**/*" if recursive else "*"
        for path in directory.glob(pattern):
            if path.is_file():
                result.total_files += 1
                
                if self._is_audio_file(path):
                    result.audio_files += 1
                    
                    try:
                        track = self._extract_metadata(path)
                        track.priority = self._calculate_priority(track)
                        result.tracks.append(track)
                    except Exception as e:
                        result.errors.append(f"Error scanning {path}: {e}")
        
        # Sort by priority
        result.tracks.sort(key=lambda t: (t.priority, t.display_name))
        
        return result
    
    def scan_multiple(
        self,
        directories: list[str | Path],
        recursive: bool = True
    ) -> ScanResult:
        """
        Scan multiple directories and merge results.
        
        Args:
            directories: List of directories to scan
            recursive: Whether to scan subdirectories
            
        Returns:
            Combined ScanResult sorted by priority
        """
        combined = ScanResult()
        
        for directory in directories:
            result = self.scan_directory(directory, recursive)
            combined.tracks.extend(result.tracks)
            combined.total_files += result.total_files
            combined.audio_files += result.audio_files
            combined.errors.extend(result.errors)
        
        # Re-sort combined results
        combined.tracks.sort(key=lambda t: (t.priority, t.display_name))
        
        return combined
    
    def iter_prioritized(
        self,
        directory: str | Path,
        limit: Optional[int] = None
    ) -> Iterator[ScannedTrack]:
        """
        Iterate over tracks in priority order.
        
        Args:
            directory: Directory to scan
            limit: Maximum number of tracks to yield
            
        Yields:
            ScannedTrack instances in priority order
        """
        result = self.scan_directory(directory)
        
        for i, track in enumerate(result.tracks):
            if limit and i >= limit:
                break
            yield track
