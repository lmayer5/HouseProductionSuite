"""
AI Stem Generation System

A robust AI-powered stem separation system using Demucs v4 (local) 
and LALAL.AI (cloud) to separate audio into 4 stems.
"""

__version__ = "1.0.0"

from .core.stem_pipeline import StemPipeline
from .utils.file_manager import StemFileManager

__all__ = ["StemPipeline", "StemFileManager"]
