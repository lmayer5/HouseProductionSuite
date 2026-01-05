"""
Quality Analyzer for Stem Separation

Calculates SI-SDR (Scale-Invariant Signal-to-Distortion Ratio) 
for quality assessment of separated stems.
"""

from pathlib import Path
from typing import Optional

import numpy as np


class StemQualityAnalyzer:
    """
    Analyzes the quality of separated audio stems.
    
    Uses SI-SDR (Scale-Invariant Signal-to-Distortion Ratio) as the 
    primary metric. Higher SI-SDR indicates better separation quality.
    
    Thresholds:
        - Excellent: SI-SDR >= 12 dB
        - Good: SI-SDR >= 8 dB
        - Acceptable: SI-SDR >= 5 dB
        - Poor: SI-SDR < 5 dB
        
    For vocals specifically, we use a stricter threshold of 7 dB
    as the cutoff for triggering a re-processing attempt.
    """
    
    # Quality thresholds in dB
    THRESHOLD_EXCELLENT = 12.0
    THRESHOLD_GOOD = 8.0
    THRESHOLD_ACCEPTABLE = 5.0
    
    # Minimum acceptable SI-SDR for vocals before fallback
    VOCAL_QUALITY_THRESHOLD = 7.0
    
    def __init__(self):
        """Initialize the quality analyzer."""
        self._librosa = None
        self._soundfile = None
    
    def _lazy_import(self) -> bool:
        """Lazily import audio processing libraries."""
        if self._librosa is not None:
            return True
        try:
            import librosa
            import soundfile as sf
            self._librosa = librosa
            self._soundfile = sf
            return True
        except ImportError:
            return False
    
    def _load_audio(self, path: Path) -> Optional[tuple[np.ndarray, int]]:
        """
        Load audio file as numpy array.
        
        Returns:
            Tuple of (audio_array, sample_rate) or None if failed
        """
        if not self._lazy_import():
            return None
            
        try:
            audio, sr = self._librosa.load(path, sr=None, mono=True)
            return audio, sr
        except Exception:
            return None
    
    @staticmethod
    def calculate_si_sdr(reference: np.ndarray, estimate: np.ndarray) -> float:
        """
        Calculate Scale-Invariant Signal-to-Distortion Ratio.
        
        SI-SDR is defined as:
            SI-SDR = 10 * log10(||s_target||^2 / ||e_noise||^2)
        
        where:
            s_target = (<estimate, reference> / ||reference||^2) * reference
            e_noise = estimate - s_target
        
        Args:
            reference: Reference (ground truth) signal
            estimate: Estimated (separated) signal
            
        Returns:
            SI-SDR value in dB
        """
        # Ensure same length
        min_len = min(len(reference), len(estimate))
        reference = reference[:min_len]
        estimate = estimate[:min_len]
        
        # Remove mean
        reference = reference - np.mean(reference)
        estimate = estimate - np.mean(estimate)
        
        # Avoid division by zero
        ref_energy = np.sum(reference ** 2)
        if ref_energy < 1e-10:
            return float('-inf')
        
        # Calculate scaling factor
        dot_product = np.sum(reference * estimate)
        s_target_scale = dot_product / ref_energy
        s_target = s_target_scale * reference
        
        # Calculate noise
        e_noise = estimate - s_target
        
        # Calculate SI-SDR
        target_energy = np.sum(s_target ** 2)
        noise_energy = np.sum(e_noise ** 2)
        
        if noise_energy < 1e-10:
            return float('inf')
            
        si_sdr = 10 * np.log10(target_energy / noise_energy)
        return float(si_sdr)
    
    def analyze_stem(self, stem_path: Path, original_path: Path) -> Optional[float]:
        """
        Analyze the quality of a separated stem.
        
        This compares the stem against the original mixture to estimate
        separation quality. Note: True SI-SDR requires ground truth stems,
        so this is an approximation based on spectral analysis.
        
        Args:
            stem_path: Path to the separated stem
            original_path: Path to the original mixture
            
        Returns:
            Estimated SI-SDR in dB, or None if analysis failed
        """
        if not self._lazy_import():
            return None
            
        stem_data = self._load_audio(stem_path)
        original_data = self._load_audio(original_path)
        
        if stem_data is None or original_data is None:
            return None
            
        stem_audio, stem_sr = stem_data
        original_audio, original_sr = original_data
        
        # Resample if needed
        if stem_sr != original_sr:
            stem_audio = self._librosa.resample(
                stem_audio, orig_sr=stem_sr, target_sr=original_sr
            )
        
        # Calculate SI-SDR
        return self.calculate_si_sdr(original_audio, stem_audio)
    
    def analyze_all_stems(self, stem_dir: Path, original_path: Path) -> dict[str, float]:
        """
        Analyze quality of all stems in a directory.
        
        Args:
            stem_dir: Directory containing stem files
            original_path: Path to the original mixture
            
        Returns:
            Dictionary mapping stem names to SI-SDR values
        """
        results = {}
        stem_names = ["vocals", "drums", "bass", "other"]
        
        for stem in stem_names:
            stem_path = stem_dir / f"{stem}.wav"
            if stem_path.exists():
                si_sdr = self.analyze_stem(stem_path, original_path)
                if si_sdr is not None:
                    results[stem] = si_sdr
        
        return results
    
    def get_quality_label(self, si_sdr: float) -> str:
        """
        Get a human-readable quality label for an SI-SDR value.
        
        Args:
            si_sdr: SI-SDR value in dB
            
        Returns:
            Quality label string
        """
        if si_sdr >= self.THRESHOLD_EXCELLENT:
            return "excellent"
        elif si_sdr >= self.THRESHOLD_GOOD:
            return "good"
        elif si_sdr >= self.THRESHOLD_ACCEPTABLE:
            return "acceptable"
        else:
            return "poor"
    
    def needs_reprocessing(self, stem_name: str, si_sdr: float) -> bool:
        """
        Check if a stem should be reprocessed due to low quality.
        
        Vocals have a stricter threshold than other stems.
        
        Args:
            stem_name: Name of the stem ('vocals', 'drums', etc.)
            si_sdr: SI-SDR value in dB
            
        Returns:
            True if the stem should be reprocessed
        """
        threshold = (
            self.VOCAL_QUALITY_THRESHOLD 
            if stem_name == "vocals" 
            else self.THRESHOLD_ACCEPTABLE
        )
        return si_sdr < threshold
