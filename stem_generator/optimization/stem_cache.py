"""
Stem Cache System

MD5-based caching to prevent re-processing identical files.
"""

import hashlib
import json
import os
import shutil
from dataclasses import dataclass
from datetime import datetime, timedelta
from pathlib import Path
from typing import Optional


@dataclass
class CacheEntry:
    """A cached stem separation result."""
    file_hash: str
    engine_id: str
    created_at: datetime
    stem_paths: dict[str, str]
    quality_scores: dict[str, float]


class StemCache:
    """
    MD5-based cache for stem separation results.
    
    Prevents re-processing identical audio files by caching
    results indexed by file hash + engine ID.
    
    Cache Structure:
        cache_dir/
            {md5_hash}_{engine_id}/
                cache_meta.json
                vocals.wav
                drums.wav
                bass.wav
                other.wav
    """
    
    META_FILE = "cache_meta.json"
    
    def __init__(self, cache_dir: str = "data/cache"):
        """
        Initialize the cache system.
        
        Args:
            cache_dir: Directory for cached results
        """
        self.cache_dir = Path(cache_dir)
        self.cache_dir.mkdir(parents=True, exist_ok=True)
    
    def _compute_file_hash(self, file_path: Path) -> str:
        """
        Compute MD5 hash of a file.
        
        Uses the entire file for accurate caching.
        
        Args:
            file_path: Path to file
            
        Returns:
            MD5 hash string
        """
        hasher = hashlib.md5()
        with open(file_path, 'rb') as f:
            # Read in chunks to handle large files
            for chunk in iter(lambda: f.read(8192), b''):
                hasher.update(chunk)
        return hasher.hexdigest()
    
    def _get_cache_key(self, file_hash: str, engine_id: str) -> str:
        """Generate cache key from hash and engine."""
        return f"{file_hash}_{engine_id}"
    
    def _get_cache_path(self, cache_key: str) -> Path:
        """Get the cache directory path for a key."""
        return self.cache_dir / cache_key
    
    def get(
        self,
        file_path: Path,
        engine_id: str
    ) -> Optional[CacheEntry]:
        """
        Get cached result for a file.
        
        Args:
            file_path: Path to original audio file
            engine_id: Engine used for separation
            
        Returns:
            CacheEntry if found, None otherwise
        """
        file_hash = self._compute_file_hash(file_path)
        cache_key = self._get_cache_key(file_hash, engine_id)
        cache_path = self._get_cache_path(cache_key)
        
        meta_path = cache_path / self.META_FILE
        
        if not meta_path.exists():
            return None
        
        try:
            with open(meta_path, 'r') as f:
                meta = json.load(f)
            
            # Verify stems exist and paths stay within cache directory
            stem_paths = {}
            cache_path_resolved = cache_path.resolve()
            
            for stem_name, rel_path in meta.get('stem_paths', {}).items():
                stem_path = (cache_path / rel_path).resolve()
                
                # SECURITY: Prevent path traversal attacks
                if not str(stem_path).startswith(str(cache_path_resolved)):
                    continue  # Skip malicious paths
                
                if stem_path.exists():
                    stem_paths[stem_name] = str(stem_path)
            
            if len(stem_paths) != 4:
                # Cache is incomplete, remove it
                self.invalidate(file_path, engine_id)
                return None
            
            return CacheEntry(
                file_hash=meta['file_hash'],
                engine_id=meta['engine_id'],
                created_at=datetime.fromisoformat(meta['created_at']),
                stem_paths=stem_paths,
                quality_scores=meta.get('quality_scores', {})
            )
            
        except (json.JSONDecodeError, KeyError):
            return None
    
    def put(
        self,
        file_path: Path,
        engine_id: str,
        stem_paths: dict[str, Path],
        quality_scores: Optional[dict[str, float]] = None
    ) -> CacheEntry:
        """
        Store a separation result in the cache.
        
        Args:
            file_path: Path to original audio file
            engine_id: Engine used for separation
            stem_paths: Dictionary of stem names to paths
            quality_scores: Optional quality scores
            
        Returns:
            CacheEntry for the stored result
        """
        file_hash = self._compute_file_hash(file_path)
        cache_key = self._get_cache_key(file_hash, engine_id)
        cache_path = self._get_cache_path(cache_key)
        
        # Create cache directory
        cache_path.mkdir(parents=True, exist_ok=True)
        
        # Copy stems to cache
        cached_stems = {}
        for stem_name, src_path in stem_paths.items():
            dst_path = cache_path / f"{stem_name}.wav"
            shutil.copy2(src_path, dst_path)
            cached_stems[stem_name] = f"{stem_name}.wav"
        
        # Create metadata
        meta = {
            'file_hash': file_hash,
            'engine_id': engine_id,
            'created_at': datetime.now().isoformat(),
            'stem_paths': cached_stems,
            'quality_scores': quality_scores or {}
        }
        
        with open(cache_path / self.META_FILE, 'w') as f:
            json.dump(meta, f, indent=2)
        
        return CacheEntry(
            file_hash=file_hash,
            engine_id=engine_id,
            created_at=datetime.now(),
            stem_paths={k: str(cache_path / v) for k, v in cached_stems.items()},
            quality_scores=quality_scores or {}
        )
    
    def exists(self, file_path: Path, engine_id: str) -> bool:
        """Check if a cached result exists."""
        return self.get(file_path, engine_id) is not None
    
    def invalidate(self, file_path: Path, engine_id: str) -> bool:
        """
        Remove a cached result.
        
        Args:
            file_path: Path to original audio file
            engine_id: Engine used for separation
            
        Returns:
            True if cache was removed, False if not found
        """
        file_hash = self._compute_file_hash(file_path)
        cache_key = self._get_cache_key(file_hash, engine_id)
        cache_path = self._get_cache_path(cache_key)
        
        if cache_path.exists():
            shutil.rmtree(cache_path)
            return True
        return False
    
    def clear_cache(self, older_than_days: Optional[int] = None) -> int:
        """
        Clear the cache.
        
        Args:
            older_than_days: Only clear entries older than this many days.
                           If None, clears all entries.
            
        Returns:
            Number of entries cleared
        """
        cleared = 0
        cutoff = None
        
        if older_than_days is not None:
            cutoff = datetime.now() - timedelta(days=older_than_days)
        
        for item in self.cache_dir.iterdir():
            if not item.is_dir():
                continue
            
            meta_path = item / self.META_FILE
            
            should_delete = True
            if cutoff and meta_path.exists():
                try:
                    with open(meta_path, 'r') as f:
                        meta = json.load(f)
                    created = datetime.fromisoformat(meta['created_at'])
                    should_delete = created < cutoff
                except (json.JSONDecodeError, KeyError):
                    should_delete = True
            
            if should_delete:
                shutil.rmtree(item)
                cleared += 1
        
        return cleared
    
    def get_cache_stats(self) -> dict:
        """
        Get cache statistics.
        
        Returns:
            Dictionary with cache stats
        """
        total_entries = 0
        total_size_bytes = 0
        
        for item in self.cache_dir.iterdir():
            if item.is_dir():
                total_entries += 1
                for file in item.rglob('*'):
                    if file.is_file():
                        total_size_bytes += file.stat().st_size
        
        return {
            'total_entries': total_entries,
            'total_size_mb': total_size_bytes / (1024 * 1024),
            'cache_dir': str(self.cache_dir)
        }
