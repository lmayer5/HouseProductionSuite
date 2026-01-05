"""
Stem Remixer Utility

Time-stretching and vocal chopping tools for creative remixing.
"""

import os
from dataclasses import dataclass
from pathlib import Path
from typing import Optional

import librosa
import numpy as np
import soundfile as sf


@dataclass
class RemixResult:
    """Result of a remix operation."""
    success: bool
    output_path: Optional[Path]
    message: str


class StemRemixer:
    """
    Creative tools for remixing separated stems.
    
    Features:
        - High-quality time stretching (0.5x - 2.0x)
        - Vocal chopping based on transient detection
        - Pitch shifting
    """
    
    def __init__(self, output_dir: Optional[str] = None):
        """
        Initialize the stem remixer.
        
        Args:
            output_dir: Default output directory for processed files
        """
        self.output_dir = Path(output_dir) if output_dir else None
    
    def _get_output_path(
        self,
        input_path: Path,
        suffix: str,
        output_dir: Optional[Path] = None
    ) -> Path:
        """Generate output path with suffix."""
        out_dir = output_dir or self.output_dir or input_path.parent
        out_dir = Path(out_dir)
        out_dir.mkdir(parents=True, exist_ok=True)
        
        stem_name = input_path.stem
        return out_dir / f"{stem_name}_{suffix}.wav"
    
    def time_stretch_stem(
        self,
        input_path: Path,
        rate: float,
        output_path: Optional[Path] = None,
        preserve_pitch: bool = True
    ) -> RemixResult:
        """
        Time-stretch a stem by the given rate.
        
        Args:
            input_path: Path to input audio file
            rate: Stretch rate (0.5 = half speed, 2.0 = double speed)
            output_path: Output file path (auto-generated if not provided)
            preserve_pitch: Whether to preserve pitch during stretching
            
        Returns:
            RemixResult with output path
        """
        if not 0.25 <= rate <= 4.0:
            return RemixResult(
                success=False,
                output_path=None,
                message=f"Rate must be between 0.25 and 4.0, got {rate}"
            )
        
        try:
            # Load audio
            y, sr = librosa.load(str(input_path), sr=None, mono=False)
            
            # Handle stereo
            is_stereo = y.ndim == 2
            if is_stereo:
                channels = [y[0], y[1]]
            else:
                channels = [y]
            
            stretched_channels = []
            for channel in channels:
                if preserve_pitch:
                    # Use librosa's time_stretch which preserves pitch
                    stretched = librosa.effects.time_stretch(channel, rate=rate)
                else:
                    # Simple resampling (changes pitch)
                    stretched = librosa.resample(
                        channel,
                        orig_sr=sr,
                        target_sr=int(sr * rate)
                    )
                stretched_channels.append(stretched)
            
            # Reconstruct stereo if needed
            if is_stereo:
                # Ensure same length
                min_len = min(len(stretched_channels[0]), len(stretched_channels[1]))
                output = np.stack([
                    stretched_channels[0][:min_len],
                    stretched_channels[1][:min_len]
                ])
            else:
                output = stretched_channels[0]
            
            # Generate output path
            if not output_path:
                rate_str = f"{rate:.2f}x".replace(".", "p")
                output_path = self._get_output_path(
                    input_path,
                    f"stretched_{rate_str}"
                )
            
            # Save
            sf.write(str(output_path), output.T if is_stereo else output, sr)
            
            return RemixResult(
                success=True,
                output_path=output_path,
                message=f"Time-stretched to {rate}x"
            )
            
        except Exception as e:
            return RemixResult(
                success=False,
                output_path=None,
                message=f"Time stretch failed: {e}"
            )
    
    def pitch_shift_stem(
        self,
        input_path: Path,
        semitones: float,
        output_path: Optional[Path] = None
    ) -> RemixResult:
        """
        Pitch-shift a stem by the given number of semitones.
        
        Args:
            input_path: Path to input audio file
            semitones: Number of semitones to shift (positive = up, negative = down)
            output_path: Output file path
            
        Returns:
            RemixResult with output path
        """
        if not -12 <= semitones <= 12:
            return RemixResult(
                success=False,
                output_path=None,
                message=f"Semitones must be between -12 and 12, got {semitones}"
            )
        
        try:
            y, sr = librosa.load(str(input_path), sr=None, mono=False)
            
            is_stereo = y.ndim == 2
            if is_stereo:
                shifted = np.stack([
                    librosa.effects.pitch_shift(y[0], sr=sr, n_steps=semitones),
                    librosa.effects.pitch_shift(y[1], sr=sr, n_steps=semitones)
                ])
            else:
                shifted = librosa.effects.pitch_shift(y, sr=sr, n_steps=semitones)
            
            if not output_path:
                sign = "up" if semitones >= 0 else "down"
                output_path = self._get_output_path(
                    input_path,
                    f"pitch_{sign}_{abs(int(semitones))}st"
                )
            
            sf.write(str(output_path), shifted.T if is_stereo else shifted, sr)
            
            return RemixResult(
                success=True,
                output_path=output_path,
                message=f"Pitch-shifted by {semitones} semitones"
            )
            
        except Exception as e:
            return RemixResult(
                success=False,
                output_path=None,
                message=f"Pitch shift failed: {e}"
            )
    
    def create_vocal_chops(
        self,
        vocal_path: Path,
        output_dir: Optional[Path] = None,
        min_duration_ms: int = 100,
        max_chops: int = 32
    ) -> list[Path]:
        """
        Create vocal chops from a vocal stem based on transient detection.
        
        Args:
            vocal_path: Path to vocal stem
            output_dir: Directory to save chops
            min_duration_ms: Minimum chop duration in milliseconds
            max_chops: Maximum number of chops to create
            
        Returns:
            List of paths to created chop files
        """
        out_dir = output_dir or self.output_dir or vocal_path.parent / "chops"
        out_dir = Path(out_dir)
        out_dir.mkdir(parents=True, exist_ok=True)
        
        created_chops = []
        
        try:
            # Load audio
            y, sr = librosa.load(str(vocal_path), sr=None, mono=True)
            
            # Detect onsets (transients)
            onset_frames = librosa.onset.onset_detect(
                y=y, sr=sr,
                units='frames',
                hop_length=512
            )
            
            # Convert frames to samples
            onset_samples = librosa.frames_to_samples(onset_frames, hop_length=512)
            
            # Add end of audio as final boundary
            onset_samples = np.append(onset_samples, len(y))
            
            # Minimum samples based on min duration
            min_samples = int((min_duration_ms / 1000) * sr)
            
            chop_count = 0
            for i in range(len(onset_samples) - 1):
                if chop_count >= max_chops:
                    break
                
                start = onset_samples[i]
                end = onset_samples[i + 1]
                
                # Skip if too short
                if end - start < min_samples:
                    continue
                
                # Extract chop
                chop = y[start:end]
                
                # Apply short fade in/out to avoid clicks
                fade_samples = min(100, len(chop) // 10)
                chop[:fade_samples] *= np.linspace(0, 1, fade_samples)
                chop[-fade_samples:] *= np.linspace(1, 0, fade_samples)
                
                # Save chop
                chop_path = out_dir / f"chop_{chop_count + 1:02d}.wav"
                sf.write(str(chop_path), chop, sr)
                created_chops.append(chop_path)
                
                chop_count += 1
            
        except Exception as e:
            print(f"Error creating vocal chops: {e}")
        
        return created_chops
    
    def create_loop(
        self,
        input_path: Path,
        bars: int = 4,
        bpm: Optional[float] = None,
        output_path: Optional[Path] = None
    ) -> RemixResult:
        """
        Extract a loopable section from a stem.
        
        Args:
            input_path: Path to input audio
            bars: Number of bars to loop
            bpm: BPM (auto-detected if not provided)
            output_path: Output file path
            
        Returns:
            RemixResult with loop path
        """
        try:
            y, sr = librosa.load(str(input_path), sr=None, mono=False)
            
            # Detect BPM if not provided
            if bpm is None:
                y_mono = y if y.ndim == 1 else y[0]
                tempo, _ = librosa.beat.beat_track(y=y_mono, sr=sr)
                if hasattr(tempo, '__len__'):
                    bpm = float(tempo[0]) if len(tempo) > 0 else 120.0
                else:
                    bpm = float(tempo)
            
            # Calculate loop length in samples
            # 4 beats per bar
            beats_per_bar = 4
            seconds_per_beat = 60.0 / bpm
            loop_seconds = bars * beats_per_bar * seconds_per_beat
            loop_samples = int(loop_seconds * sr)
            
            # Extract loop from start
            is_stereo = y.ndim == 2
            if is_stereo:
                loop = y[:, :min(loop_samples, y.shape[1])]
            else:
                loop = y[:min(loop_samples, len(y))]
            
            if not output_path:
                output_path = self._get_output_path(
                    input_path,
                    f"loop_{bars}bar"
                )
            
            sf.write(str(output_path), loop.T if is_stereo else loop, sr)
            
            return RemixResult(
                success=True,
                output_path=output_path,
                message=f"Created {bars}-bar loop at {bpm:.1f} BPM"
            )
            
        except Exception as e:
            return RemixResult(
                success=False,
                output_path=None,
                message=f"Loop creation failed: {e}"
            )
