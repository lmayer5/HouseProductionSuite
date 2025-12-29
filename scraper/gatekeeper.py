import librosa
import numpy as np
import os
import hashlib
from .config import DEFAULT_Sample_RATE_MIN, DEFAULT_DURATION_MAX

class Gatekeeper:
    def __init__(self):
        pass

    def check_quality(self, file_path):
        """
        Analyzes the audio file for quality standards.
        Returns (bool, str): (Passed, Reason)
        """
        try:
            # Load audio (sr=None to preserve original sample rate)
            y, sr = librosa.load(file_path, sr=None)
        except Exception as e:
            return False, f"Load Error: {e}"

        # 1. Check Sample Rate
        if sr < DEFAULT_Sample_RATE_MIN:
            return False, f"Low Sample Rate ({sr}Hz)"

        # 2. Check Duration (for one-shots, we check max duration)
        # Note: If we are scraping loops, this might be flexible, but for now enforcing config.
        duration = librosa.get_duration(y=y, sr=sr)
        if duration > DEFAULT_DURATION_MAX:
             # Just a warning or soft fail? The plan says "Reject if > 10s"
             return False, f"Too Long ({duration:.2f}s)"

        # 3. Detect Clipping
        # A simple check is if max absolute amplitude is >= 1.0 (or very close)
        # We'll use 0.999 as a threshold to be safe, or exactly 1.0. 
        # Often floating point audio is normalized. Clipping usually means hard 1.0.
        if np.max(np.abs(y)) >= 1.0:
            return False, "Clipped/Distorted"

        return True, "Quality OK"

    def get_file_hash(self, file_path):
        """
        Generates a SHA256 hash of the file content to detect exact duplicates.
        For audio content-based dupes, we'd need acoustic fingerprinting (Chromaprint),
        but that requires external binaries (fpcalc). SHA256 is a good start for exact file dupes.
        """
        sha256_hash = hashlib.sha256()
        try:
            with open(file_path, "rb") as f:
                # Read and update hash string value in blocks of 4K
                for byte_block in iter(lambda: f.read(4096), b""):
                    sha256_hash.update(byte_block)
            return sha256_hash.hexdigest()
        except Exception as e:
            return None

    def is_duplicate(self, file_path, existing_hashes):
        """
        Checks if file hash exists in the known hashes set.
        """
        current_hash = self.get_file_hash(file_path)
        if current_hash in existing_hashes:
            return True
        return False
