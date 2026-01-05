"""
Abstract Base Engine for Stem Separation

Defines the interface that all stem separation engines must implement.
"""

from abc import ABC, abstractmethod
from dataclasses import dataclass
from pathlib import Path
from typing import Optional


@dataclass
class SeparationResult:
    """Result of a stem separation operation."""
    success: bool
    stem_paths: dict[str, Path]  # stem_name -> file path
    processing_time_seconds: float
    engine_name: str
    error_message: Optional[str] = None


class StemEngine(ABC):
    """
    Abstract base class for stem separation engines.
    
    Implementations must provide:
        - separate(): Perform the actual stem separation
        - is_available(): Check if the engine can be used
        - name: Property returning the engine identifier
    """
    
    @property
    @abstractmethod
    def name(self) -> str:
        """Unique identifier for this engine."""
        pass
    
    @abstractmethod
    def is_available(self) -> bool:
        """
        Check if this engine is available for use.
        
        Returns:
            True if the engine can be used, False otherwise
        """
        pass
    
    @abstractmethod
    def separate(self, input_path: Path, output_dir: Path) -> SeparationResult:
        """
        Separate audio into stems.
        
        Args:
            input_path: Path to the source audio file
            output_dir: Directory to save the stems
            
        Returns:
            SeparationResult with success status and stem paths
        """
        pass
    
    def get_recommended_batch_size(self) -> int:
        """
        Get the recommended batch size for this engine.
        
        Default implementation returns 1, engines can override.
        
        Returns:
            Recommended number of files to process in parallel
        """
        return 1
