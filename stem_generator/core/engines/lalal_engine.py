"""
LALAL.AI Cloud Stem Separation Engine

Uses LALAL.AI REST API for cloud-based stem separation.
Requires API key set in environment variable LALAL_API_KEY.
"""

import logging
import os
import time
from pathlib import Path
from typing import Optional

import requests
from dotenv import load_dotenv

from .base_engine import StemEngine, SeparationResult


logger = logging.getLogger(__name__)

# Allowed audio file extensions
ALLOWED_EXTENSIONS = {'.mp3', '.wav', '.flac', '.ogg', '.m4a', '.aac', '.wma'}

# Maximum file size for upload (100 MB)
MAX_FILE_SIZE_BYTES = 100 * 1024 * 1024


class LalalEngine(StemEngine):
    """
    Cloud-based stem separation using LALAL.AI API.
    
    LALAL.AI provides high-quality stem separation as a cloud service.
    Useful as a fallback when local processing quality is insufficient.
    
    Requires: LALAL_API_KEY environment variable
    """
    
    API_BASE_URL = "https://www.lalal.ai/api"
    
    # API endpoints
    UPLOAD_ENDPOINT = "/upload/"
    CHECK_ENDPOINT = "/check/"
    DOWNLOAD_ENDPOINT = "/download/"
    
    # Stem type mapping for LALAL.AI API
    STEM_TYPES = {
        "vocals": "vocal",
        "drums": "drum",
        "bass": "bass",
        "other": "other"
    }
    
    def __init__(self, api_key: Optional[str] = None):
        """
        Initialize the LALAL.AI engine.
        
        Args:
            api_key: LALAL.AI API key (or set LALAL_API_KEY env var)
        """
        load_dotenv()
        self._api_key = api_key or os.getenv("LALAL_API_KEY")
    
    @property
    def name(self) -> str:
        return "lalal_cloud"
    
    def is_available(self) -> bool:
        """Check if LALAL.AI API key is configured."""
        return bool(self._api_key)
    
    def _validate_file(self, file_path: Path) -> tuple[bool, str]:
        """
        Validate file before upload.
        
        Returns:
            Tuple of (is_valid, error_message)
        """
        # Check extension
        if file_path.suffix.lower() not in ALLOWED_EXTENSIONS:
            return False, f"Unsupported file type: {file_path.suffix}"
        
        # Check file size
        file_size = file_path.stat().st_size
        if file_size > MAX_FILE_SIZE_BYTES:
            size_mb = file_size / (1024 * 1024)
            return False, f"File too large: {size_mb:.1f}MB (max 100MB)"
        
        # Check file exists and is readable
        if not file_path.exists():
            return False, "File does not exist"
        
        return True, ""
    
    def _validate_api_response(self, response: dict, required_fields: list[str]) -> bool:
        """Validate API response has required fields."""
        return all(field in response for field in required_fields)
    
    def _upload_file(self, file_path: Path) -> Optional[str]:
        """
        Upload a file to LALAL.AI for processing.
        
        Returns:
            Job ID if successful, None otherwise
        """
        headers = {
            "Authorization": f"license {self._api_key}"
        }
        
        try:
            with open(file_path, 'rb') as f:
                files = {"file": (file_path.name, f)}
                response = requests.post(
                    f"{self.API_BASE_URL}{self.UPLOAD_ENDPOINT}",
                    headers=headers,
                    files=files,
                    data={"stem": "vocals"},
                    timeout=120  # 2 minute timeout for upload
                )
            
            if response.status_code == 200:
                data = response.json()
                if self._validate_api_response(data, ["id"]):
                    logger.info(f"File uploaded successfully, job ID: {data['id']}")
                    return data.get("id")
                else:
                    logger.error("Invalid API response: missing 'id' field")
            else:
                # Don't log the full response as it might contain sensitive info
                logger.error(f"Upload failed with status code: {response.status_code}")
        except requests.RequestException as e:
            logger.error(f"Upload request failed: {type(e).__name__}")
        except Exception as e:
            logger.error(f"Unexpected error during upload: {type(e).__name__}")
        
        return None
    
    def _check_status(self, job_id: str) -> dict:
        """
        Check the status of a processing job.
        
        Returns:
            Status dictionary with 'status' and result URLs
        """
        headers = {
            "Authorization": f"license {self._api_key}"
        }
        
        try:
            response = requests.get(
                f"{self.API_BASE_URL}{self.CHECK_ENDPOINT}",
                headers=headers,
                params={"id": job_id},
                timeout=30
            )
            
            if response.status_code == 200:
                data = response.json()
                if self._validate_api_response(data, ["status"]):
                    return data
                logger.warning("Invalid status response: missing 'status' field")
            else:
                logger.warning(f"Status check failed: {response.status_code}")
        except requests.RequestException as e:
            logger.warning(f"Status check request failed: {type(e).__name__}")
        
        return {"status": "error"}
    
    def _download_stem(self, url: str, output_path: Path) -> bool:
        """
        Download a processed stem from LALAL.AI.
        
        Returns:
            True if successful, False otherwise
        """
        try:
            response = requests.get(url, stream=True, timeout=120)
            if response.status_code == 200:
                output_path.parent.mkdir(parents=True, exist_ok=True)
                with open(output_path, 'wb') as f:
                    for chunk in response.iter_content(chunk_size=8192):
                        f.write(chunk)
                logger.debug(f"Downloaded stem to: {output_path}")
                return True
            else:
                logger.error(f"Download failed: {response.status_code}")
        except requests.RequestException as e:
            logger.error(f"Download request failed: {type(e).__name__}")
        except IOError as e:
            logger.error(f"Failed to write file: {type(e).__name__}")
        
        return False
    
    def _wait_for_completion(self, job_id: str, timeout: int = 600) -> dict:
        """
        Poll for job completion.
        
        Args:
            job_id: The job ID to check
            timeout: Maximum seconds to wait
            
        Returns:
            Final status dictionary
        """
        start_time = time.time()
        poll_interval = 5  # seconds
        
        while time.time() - start_time < timeout:
            status = self._check_status(job_id)
            
            if status.get("status") in ("done", "error"):
                return status
                
            time.sleep(poll_interval)
        
        return {"status": "timeout"}
    
    def separate(self, input_path: Path, output_dir: Path) -> SeparationResult:
        """
        Separate audio into stems using LALAL.AI cloud API.
        
        Note: LALAL.AI processes one stem type at a time, so this
        makes multiple API calls (one per stem type).
        
        Args:
            input_path: Path to input audio file
            output_dir: Directory to save output stems
            
        Returns:
            SeparationResult with paths to generated stems
        """
        start_time = time.time()
        stem_paths = {}
        
        if not self.is_available():
            return SeparationResult(
                success=False,
                stem_paths={},
                processing_time_seconds=0,
                engine_name=self.name,
                error_message="LALAL.AI API key not configured"
            )
        
        # Validate file before upload
        is_valid, error_msg = self._validate_file(input_path)
        if not is_valid:
            return SeparationResult(
                success=False,
                stem_paths={},
                processing_time_seconds=0,
                engine_name=self.name,
                error_message=error_msg
            )
        
        try:
            output_dir.mkdir(parents=True, exist_ok=True)
            
            logger.info(f"Uploading to LALAL.AI: {input_path.name}")
            
            # Upload file
            job_id = self._upload_file(input_path)
            if not job_id:
                raise RuntimeError("Failed to upload file to LALAL.AI")
            
            # Wait for processing
            result = self._wait_for_completion(job_id)
            
            if result.get("status") == "error":
                raise RuntimeError(f"LALAL.AI processing error: {result.get('error', 'Unknown')}")
            
            if result.get("status") == "timeout":
                raise RuntimeError("LALAL.AI processing timeout")
            
            # Download available stems
            if "result" in result:
                for stem_name, lalal_name in self.STEM_TYPES.items():
                    url = result["result"].get(lalal_name)
                    if url:
                        output_path = output_dir / f"{stem_name}.wav"
                        if self._download_stem(url, output_path):
                            stem_paths[stem_name] = output_path
            
            processing_time = time.time() - start_time
            
            # Check if we got all stems
            success = len(stem_paths) == 4
            
            return SeparationResult(
                success=success,
                stem_paths=stem_paths,
                processing_time_seconds=processing_time,
                engine_name=self.name,
                error_message=None if success else "Not all stems were retrieved"
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
