"""
Demucs v4 Local Stem Separation Engine

Uses Facebook's Demucs model for local GPU-accelerated stem separation.
"""

import time
from pathlib import Path
from typing import Optional

from .base_engine import StemEngine, SeparationResult


class DemucsEngine(StemEngine):
    """
    Local stem separation using Demucs v4.
    
    Demucs is a state-of-the-art music source separation model
    that separates audio into 4 stems: vocals, drums, bass, other.
    
    Requires: torch, torchaudio, demucs packages
    """
    
    # Demucs model to use (htdemucs is the default hybrid transformer model)
    DEFAULT_MODEL = "htdemucs"
    
    # Stem name mapping (Demucs uses these exact names)
    STEM_NAMES = ["vocals", "drums", "bass", "other"]
    
    def __init__(self, model_name: str = DEFAULT_MODEL, device: Optional[str] = None):
        """
        Initialize the Demucs engine.
        
        Args:
            model_name: Demucs model to use ('htdemucs', 'htdemucs_ft', etc.)
            device: PyTorch device ('cuda', 'cpu', or None for auto)
        """
        self.model_name = model_name
        self._device = device
        self._model = None
        self._torch = None
    
    @property
    def name(self) -> str:
        return f"demucs_{self.model_name}"
    
    def _lazy_import(self) -> bool:
        """Lazily import heavy dependencies."""
        if self._torch is not None:
            return True
            
        try:
            import torch
            import demucs.pretrained
            self._torch = torch
            return True
        except ImportError:
            return False
    
    def is_available(self) -> bool:
        """Check if Demucs can be used (torch + demucs installed)."""
        return self._lazy_import()
    
    @property
    def device(self) -> str:
        """Get the device to use for inference."""
        if self._device:
            return self._device
            
        if self._lazy_import():
            return "cuda" if self._torch.cuda.is_available() else "cpu"
        return "cpu"
    
    def _load_model(self):
        """Load the Demucs model (lazy loading)."""
        if self._model is not None:
            return
            
        if not self._lazy_import():
            raise RuntimeError("Demucs dependencies not available")
        
        from demucs.pretrained import get_model
        
        # Load the pretrained model
        self._model = get_model(self.model_name)
        self._model.to(self._torch.device(self.device))
        self._model.eval()
    
    def separate(self, input_path: Path, output_dir: Path) -> SeparationResult:
        """
        Separate audio into stems using Demucs.
        
        Args:
            input_path: Path to input audio file
            output_dir: Directory to save output stems
            
        Returns:
            SeparationResult with paths to generated stems
        """
        start_time = time.time()
        stem_paths = {}
        
        try:
            self._load_model()
            
            import librosa
            from demucs.apply import apply_model
            
            # Ensure output directory exists
            output_dir.mkdir(parents=True, exist_ok=True)
            
            # Load audio file using librosa (better format support)
            audio_np, sample_rate = librosa.load(str(input_path), sr=None, mono=False)
            
            # librosa returns (samples,) for mono or (channels, samples) for stereo
            if audio_np.ndim == 1:
                # Mono - duplicate to stereo
                audio_np = self._torch.from_numpy(audio_np).float().unsqueeze(0).repeat(2, 1)
            else:
                audio_np = self._torch.from_numpy(audio_np).float()
            
            waveform = audio_np
            
            # Demucs expects stereo at model's sample rate
            if waveform.shape[0] == 1:
                waveform = waveform.repeat(2, 1)
            
            # Resample if needed (model expects 44100 Hz)
            if sample_rate != self._model.samplerate:
                resampler = torchaudio.transforms.Resample(
                    orig_freq=sample_rate, 
                    new_freq=self._model.samplerate
                )
                waveform = resampler(waveform)
                sample_rate = self._model.samplerate
            
            # Add batch dimension and move to device
            waveform = waveform.unsqueeze(0).to(self.device)
            
            # Apply the model
            with self._torch.no_grad():
                sources = apply_model(self._model, waveform, device=self.device)
            
            # sources shape: (batch, num_sources, channels, samples)
            # Remove batch dimension
            sources = sources.squeeze(0)
            
            # Save each stem using soundfile (better compatibility)
            import soundfile as sf
            
            for idx, stem_name in enumerate(self._model.sources):
                if stem_name in self.STEM_NAMES:
                    output_path = output_dir / f"{stem_name}.wav"
                    stem_audio = sources[idx].cpu().numpy()
                    
                    # soundfile expects (samples, channels) shape
                    sf.write(
                        str(output_path),
                        stem_audio.T,  # Transpose to (samples, channels)
                        sample_rate
                    )
                    stem_paths[stem_name] = output_path
            
            processing_time = time.time() - start_time
            
            return SeparationResult(
                success=True,
                stem_paths=stem_paths,
                processing_time_seconds=processing_time,
                engine_name=self.name
            )
            
        except Exception as e:
            processing_time = time.time() - start_time
            return SeparationResult(
                success=False,
                stem_paths=stem_paths,
                processing_time_seconds=processing_time,
                engine_name=self.name,
                error_message=str(e)
            )
    
    def get_recommended_batch_size(self) -> int:
        """Get recommended batch size based on available VRAM."""
        if not self._lazy_import():
            return 1
            
        if not self._torch.cuda.is_available():
            return 1
            
        try:
            # Get VRAM in GB
            vram_bytes = self._torch.cuda.get_device_properties(0).total_memory
            vram_gb = vram_bytes / (1024 ** 3)
            
            if vram_gb >= 16:
                return 4
            elif vram_gb >= 12:
                return 2
            else:
                return 1
        except Exception:
            return 1
