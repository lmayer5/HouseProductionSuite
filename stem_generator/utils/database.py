"""
Database Module for Stem Generation System

SQLite-based tracking for processed tracks, jobs, and quality scores.
"""

import json
import logging
import sqlite3
from contextlib import contextmanager
from dataclasses import dataclass
from datetime import datetime
from enum import Enum
from pathlib import Path
from typing import Optional, Generator


logger = logging.getLogger(__name__)


class JobStatus(Enum):
    """Status of a stem separation job."""
    PENDING = "pending"
    PROCESSING = "processing"
    COMPLETED = "completed"
    FAILED = "failed"


@dataclass
class TrackRecord:
    """Database record for a processed track."""
    id: Optional[int]
    file_path: str
    file_hash: str
    artist: str
    title: str
    bpm: Optional[float]
    key: Optional[str]
    genre: Optional[str]
    created_at: datetime
    
    
@dataclass
class JobRecord:
    """Database record for a processing job."""
    id: Optional[int]
    track_id: int
    engine: str
    status: JobStatus
    processing_time_seconds: Optional[float]
    error_message: Optional[str]
    created_at: datetime
    completed_at: Optional[datetime]


@dataclass
class QualityRecord:
    """Database record for stem quality scores."""
    id: Optional[int]
    job_id: int
    stem_name: str
    si_sdr: float
    created_at: datetime


class StemDatabase:
    """
    SQLite database for tracking stem separation jobs.
    
    Tables:
        - tracks: Source audio files
        - jobs: Processing jobs (each track can have multiple)
        - quality_scores: SI-SDR scores per stem per job
    
    Uses WAL mode for better concurrent access.
    """
    
    SCHEMA = """
    CREATE TABLE IF NOT EXISTS tracks (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        file_path TEXT NOT NULL,
        file_hash TEXT NOT NULL UNIQUE,
        artist TEXT,
        title TEXT,
        bpm REAL,
        key TEXT,
        genre TEXT,
        created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
    );
    
    CREATE TABLE IF NOT EXISTS jobs (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        track_id INTEGER NOT NULL,
        engine TEXT NOT NULL,
        status TEXT NOT NULL DEFAULT 'pending',
        processing_time_seconds REAL,
        error_message TEXT,
        created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        completed_at TIMESTAMP,
        FOREIGN KEY (track_id) REFERENCES tracks(id)
    );
    
    CREATE TABLE IF NOT EXISTS quality_scores (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        job_id INTEGER NOT NULL,
        stem_name TEXT NOT NULL,
        si_sdr REAL NOT NULL,
        created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        FOREIGN KEY (job_id) REFERENCES jobs(id)
    );
    
    CREATE INDEX IF NOT EXISTS idx_tracks_hash ON tracks(file_hash);
    CREATE INDEX IF NOT EXISTS idx_jobs_track ON jobs(track_id);
    CREATE INDEX IF NOT EXISTS idx_jobs_status ON jobs(status);
    """
    
    def __init__(self, db_path: str = "data/stems/stem_generator.db"):
        """
        Initialize database connection.
        
        Args:
            db_path: Path to SQLite database file
        """
        self.db_path = Path(db_path)
        self.db_path.parent.mkdir(parents=True, exist_ok=True)
        self._init_schema()
        logger.debug(f"Database initialized at: {self.db_path}")
    
    @contextmanager
    def _get_connection(self) -> Generator[sqlite3.Connection, None, None]:
        """Context manager for database connections with WAL mode."""
        conn = sqlite3.connect(
            self.db_path, 
            detect_types=sqlite3.PARSE_DECLTYPES,
            timeout=30  # Wait up to 30 seconds for locks
        )
        conn.row_factory = sqlite3.Row
        
        # Enable WAL mode for better concurrent access
        conn.execute("PRAGMA journal_mode=WAL")
        conn.execute("PRAGMA busy_timeout=30000")  # 30 second timeout
        
        try:
            yield conn
            conn.commit()
        except sqlite3.Error as e:
            conn.rollback()
            logger.error(f"Database error: {type(e).__name__}: {e}")
            raise
        finally:
            conn.close()
    
    def _init_schema(self) -> None:
        """Initialize database schema."""
        with self._get_connection() as conn:
            conn.executescript(self.SCHEMA)
    
    # -------------------------------------------------------------------------
    # Track Operations
    # -------------------------------------------------------------------------
    
    def add_track(self, file_path: str, file_hash: str, artist: str = "",
                  title: str = "", bpm: Optional[float] = None,
                  key: Optional[str] = None, genre: Optional[str] = None) -> int:
        """
        Add a new track to the database.
        
        Returns:
            The ID of the inserted track
        """
        with self._get_connection() as conn:
            cursor = conn.execute(
                """
                INSERT INTO tracks (file_path, file_hash, artist, title, bpm, key, genre)
                VALUES (?, ?, ?, ?, ?, ?, ?)
                """,
                (file_path, file_hash, artist, title, bpm, key, genre)
            )
            return cursor.lastrowid
    
    def get_track_by_hash(self, file_hash: str) -> Optional[TrackRecord]:
        """Get a track by its file hash."""
        with self._get_connection() as conn:
            row = conn.execute(
                "SELECT * FROM tracks WHERE file_hash = ?",
                (file_hash,)
            ).fetchone()
            
            if row:
                return TrackRecord(
                    id=row['id'],
                    file_path=row['file_path'],
                    file_hash=row['file_hash'],
                    artist=row['artist'],
                    title=row['title'],
                    bpm=row['bpm'],
                    key=row['key'],
                    genre=row['genre'],
                    created_at=row['created_at']
                )
        return None
    
    def track_exists(self, file_hash: str) -> bool:
        """Check if a track with the given hash exists."""
        with self._get_connection() as conn:
            row = conn.execute(
                "SELECT 1 FROM tracks WHERE file_hash = ?",
                (file_hash,)
            ).fetchone()
            return row is not None
    
    # -------------------------------------------------------------------------
    # Job Operations
    # -------------------------------------------------------------------------
    
    def create_job(self, track_id: int, engine: str) -> int:
        """
        Create a new processing job.
        
        Returns:
            The ID of the inserted job
        """
        with self._get_connection() as conn:
            cursor = conn.execute(
                """
                INSERT INTO jobs (track_id, engine, status)
                VALUES (?, ?, ?)
                """,
                (track_id, engine, JobStatus.PENDING.value)
            )
            return cursor.lastrowid
    
    def update_job_status(self, job_id: int, status: JobStatus,
                          processing_time: Optional[float] = None,
                          error_message: Optional[str] = None) -> None:
        """Update job status and optionally processing time/error."""
        with self._get_connection() as conn:
            completed_at = datetime.now() if status in (JobStatus.COMPLETED, JobStatus.FAILED) else None
            
            conn.execute(
                """
                UPDATE jobs 
                SET status = ?, processing_time_seconds = ?, error_message = ?, completed_at = ?
                WHERE id = ?
                """,
                (status.value, processing_time, error_message, completed_at, job_id)
            )
    
    def get_latest_job_for_track(self, track_id: int) -> Optional[JobRecord]:
        """Get the most recent job for a track."""
        with self._get_connection() as conn:
            row = conn.execute(
                """
                SELECT * FROM jobs 
                WHERE track_id = ? 
                ORDER BY created_at DESC 
                LIMIT 1
                """,
                (track_id,)
            ).fetchone()
            
            if row:
                return JobRecord(
                    id=row['id'],
                    track_id=row['track_id'],
                    engine=row['engine'],
                    status=JobStatus(row['status']),
                    processing_time_seconds=row['processing_time_seconds'],
                    error_message=row['error_message'],
                    created_at=row['created_at'],
                    completed_at=row['completed_at']
                )
        return None
    
    def has_successful_job(self, file_hash: str) -> bool:
        """Check if a track has a successfully completed job."""
        with self._get_connection() as conn:
            row = conn.execute(
                """
                SELECT 1 FROM jobs j
                JOIN tracks t ON j.track_id = t.id
                WHERE t.file_hash = ? AND j.status = ?
                LIMIT 1
                """,
                (file_hash, JobStatus.COMPLETED.value)
            ).fetchone()
            return row is not None
    
    # -------------------------------------------------------------------------
    # Quality Score Operations
    # -------------------------------------------------------------------------
    
    def add_quality_score(self, job_id: int, stem_name: str, si_sdr: float) -> int:
        """Add a quality score for a stem."""
        with self._get_connection() as conn:
            cursor = conn.execute(
                """
                INSERT INTO quality_scores (job_id, stem_name, si_sdr)
                VALUES (?, ?, ?)
                """,
                (job_id, stem_name, si_sdr)
            )
            return cursor.lastrowid
    
    def get_quality_scores(self, job_id: int) -> dict[str, float]:
        """Get all quality scores for a job."""
        with self._get_connection() as conn:
            rows = conn.execute(
                "SELECT stem_name, si_sdr FROM quality_scores WHERE job_id = ?",
                (job_id,)
            ).fetchall()
            return {row['stem_name']: row['si_sdr'] for row in rows}
    
    def get_average_quality(self, job_id: int) -> Optional[float]:
        """Get average SI-SDR across all stems for a job."""
        with self._get_connection() as conn:
            row = conn.execute(
                "SELECT AVG(si_sdr) as avg_sdr FROM quality_scores WHERE job_id = ?",
                (job_id,)
            ).fetchone()
            return row['avg_sdr'] if row and row['avg_sdr'] else None
