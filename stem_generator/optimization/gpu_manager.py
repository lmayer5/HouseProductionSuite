"""
GPU Resource Manager

VRAM-aware batch sizing and PyTorch optimization settings.
"""

from dataclasses import dataclass
from typing import Optional


@dataclass
class GPUInfo:
    """Information about an available GPU."""
    name: str
    vram_gb: float
    cuda_version: str
    compute_capability: tuple[int, int]
    is_available: bool


class GPUManager:
    """
    Manages GPU resources for optimal stem separation performance.
    
    Features:
        - VRAM-aware dynamic batch sizing
        - PyTorch optimizations for Ampere+ GPUs
        - Multi-GPU detection and selection
    """
    
    # VRAM thresholds for batch sizing (in GB)
    BATCH_THRESHOLDS = {
        16.0: 4,   # 16GB+ VRAM: batch of 4
        12.0: 2,   # 12GB+ VRAM: batch of 2
        8.0: 1,    # 8GB+ VRAM: batch of 1
        0.0: 1     # Fallback
    }
    
    def __init__(self):
        """Initialize GPU manager."""
        self._torch = None
        self._gpu_info: Optional[GPUInfo] = None
    
    def _lazy_import(self) -> bool:
        """Lazily import PyTorch."""
        if self._torch is not None:
            return True
        try:
            import torch
            self._torch = torch
            return True
        except ImportError:
            return False
    
    @property
    def is_cuda_available(self) -> bool:
        """Check if CUDA is available."""
        if not self._lazy_import():
            return False
        return self._torch.cuda.is_available()
    
    @property
    def device_count(self) -> int:
        """Get number of available CUDA devices."""
        if not self.is_cuda_available:
            return 0
        return self._torch.cuda.device_count()
    
    def get_gpu_info(self, device_id: int = 0) -> Optional[GPUInfo]:
        """
        Get information about a specific GPU.
        
        Args:
            device_id: CUDA device index
            
        Returns:
            GPUInfo or None if not available
        """
        if not self.is_cuda_available:
            return None
            
        if device_id >= self.device_count:
            return None
        
        try:
            props = self._torch.cuda.get_device_properties(device_id)
            
            # Get CUDA version
            cuda_version = self._torch.version.cuda or "unknown"
            
            return GPUInfo(
                name=props.name,
                vram_gb=props.total_memory / (1024 ** 3),
                cuda_version=cuda_version,
                compute_capability=(props.major, props.minor),
                is_available=True
            )
        except Exception:
            return None
    
    def get_optimal_batch_size(self, device_id: int = 0) -> int:
        """
        Get optimal batch size based on available VRAM.
        
        Args:
            device_id: CUDA device index
            
        Returns:
            Recommended batch size (1-4)
        """
        gpu_info = self.get_gpu_info(device_id)
        
        if not gpu_info:
            return 1
        
        vram = gpu_info.vram_gb
        
        for threshold, batch_size in sorted(
            self.BATCH_THRESHOLDS.items(), 
            reverse=True
        ):
            if vram >= threshold:
                return batch_size
        
        return 1
    
    def enable_optimizations(self) -> dict[str, bool]:
        """
        Enable PyTorch optimizations for faster inference.
        
        Returns:
            Dictionary of applied optimizations
        """
        applied = {
            "tf32_matmul": False,
            "tf32_cudnn": False,
            "cudnn_benchmark": False,
            "amp_enabled": False
        }
        
        if not self._lazy_import() or not self.is_cuda_available:
            return applied
        
        gpu_info = self.get_gpu_info()
        
        # Enable TF32 for Ampere+ GPUs (compute capability >= 8.0)
        if gpu_info and gpu_info.compute_capability[0] >= 8:
            self._torch.backends.cuda.matmul.allow_tf32 = True
            self._torch.backends.cudnn.allow_tf32 = True
            applied["tf32_matmul"] = True
            applied["tf32_cudnn"] = True
        
        # Enable cuDNN benchmark mode for consistent input sizes
        self._torch.backends.cudnn.benchmark = True
        applied["cudnn_benchmark"] = True
        
        return applied
    
    def get_memory_usage(self, device_id: int = 0) -> dict[str, float]:
        """
        Get current GPU memory usage.
        
        Args:
            device_id: CUDA device index
            
        Returns:
            Dictionary with allocated, reserved, and total memory in GB
        """
        if not self.is_cuda_available:
            return {"allocated": 0, "reserved": 0, "total": 0}
        
        try:
            return {
                "allocated": self._torch.cuda.memory_allocated(device_id) / (1024 ** 3),
                "reserved": self._torch.cuda.memory_reserved(device_id) / (1024 ** 3),
                "total": self._torch.cuda.get_device_properties(device_id).total_memory / (1024 ** 3)
            }
        except Exception:
            return {"allocated": 0, "reserved": 0, "total": 0}
    
    def clear_cache(self) -> None:
        """Clear CUDA memory cache."""
        if self.is_cuda_available:
            self._torch.cuda.empty_cache()
    
    def select_device(self, prefer_high_memory: bool = True) -> int:
        """
        Select the best GPU device.
        
        Args:
            prefer_high_memory: Prefer GPU with more VRAM
            
        Returns:
            Selected device ID
        """
        if self.device_count <= 1:
            return 0
        
        # Get info for all devices
        devices = []
        for i in range(self.device_count):
            info = self.get_gpu_info(i)
            if info:
                devices.append((i, info))
        
        if not devices:
            return 0
        
        if prefer_high_memory:
            # Sort by VRAM descending
            devices.sort(key=lambda x: x[1].vram_gb, reverse=True)
        
        return devices[0][0]
