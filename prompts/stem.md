# AI Stem Generation System: Implementation Plan
**Target Audience:** Autonomous Development Agent / Engineering Team  
**Version:** 2.0 (Aggregated)  
**Date:** January 2026  

---

## Executive Summary
This document outlines a 4-step implementation plan to build a robust AI-powered stem separation system. The system leverages **Demucs v4** (local) and **LALAL.AI** (cloud) to separate audio into 4 stems (Vocals, Drums, Bass, Other) for DJ performance and production workflows.

**Key Objectives:**
1.  **Automated Routing:** Intelligent selection between local and cloud engines based on quality/load.
2.  **DJ Workflow:** Batch processing entire libraries with Serato/Rekordbox compatibility.
3.  **Production Integration:** Automated generation of Ableton Live (`.als`) project files.
4.  **Optimization:** GPU-accelerated batch processing with caching.

---

## Step 1: Core Infrastructure & Unified Separation Engine
**Timeline:** Weeks 1-5  
**Goal:** Establish the development environment, file storage architecture, and the central routing engine.

### 1.1 Development Environment & Storage
* **Dependencies:** Initialize Python 3.10+ environment.
    * **Core:** `torch` (CUDA 11.8+), `demucs`, `audio-separator` (UVR wrapper), `librosa`, `scipy`.
    * **Utils:** `requests`, `python-dotenv`, `pydub`.
* **File Architecture (`utils/file_manager.py`):**
    * Implement `StemFileManager` to standardize input/output.
    * **Structure:** `data/stems/{Artist} - {Title}_{Hash}/` containing `vocals.wav`, `drums.wav`, etc.
    * **Database:** Setup SQLite/PostgreSQL schema for tracking tracks, processing jobs, and quality scores.

### 1.2 The "Router" Logic (Critical Path)
* **Class:** `core/stem_pipeline.py` -> `StemPipeline`
* **Routing Logic:**
    1.  **Analysis:** Check file size and GPU availability.
    2.  **Selection:**
        * **Local Demucs:** Default for files < 50MB or standard batch processing.
        * **LALAL.AI:** Fallback for high-priority vocal extraction or if local quality fails checks.
        * **UVR (Ultimate Vocal Remover):** Specific option for difficult vocal separations.
* **Quality Assurance:**
    * Implement `StemQualityAnalyzer` to calculate **SI-SDR** (Scale-Invariant Signal-to-Distortion Ratio).
    * **Trigger:** If `si_sdr < 7.0 dB` (vocals), automatically retry using a different engine (e.g., fallback to Cloud API).

---

## Step 2: DJ Library Integration & Batch Processing
**Timeline:** Weeks 6-9  
**Goal:** Enable ingestion of full DJ libraries with intelligent prioritization and hardware-compatible output.

### 2.1 Library Scanner & Prioritization
* **Class:** `dj/library_scanner.py` -> `DJLibraryScanner`
* **Metadata:** Use `music_tag` to parse ID3 tags (BPM, Key, Genre).
* **Priority Queue Logic:**
    1.  Tracks in specific "Crates" (highest priority).
    2.  Genre contains "House".
    3.  BPM > 120.
    4.  Tracks with detected vocals.

### 2.2 Threaded Batch Processor
* **Class:** `dj/batch_processor.py` -> `DJBatchProcessor`
* **Concurrency:** Implement `ThreadPoolExecutor` to manage parallel jobs without overloading the GPU.
* **Resiliency:**
    * Implement `resume_processing` to skip files with existing valid hashes in the DB.
    * Generate `metadata.json` in each track folder (processing time, model used, quality score).

### 2.3 DJ Software Output
* **Class:** `dj/stem_organizer.py` -> `StemOrganizer`
* **Compatibility:**
    * **Serato/Rekordbox:** Create subdirectories per track or mirror the library structure.
    * **Naming:** Standardize filenames (e.g., `SongName_Stem.wav`) for easy drag-and-drop.

---

## Step 3: Production Tools (Ableton & Remixing)
**Timeline:** Weeks 10-13  
**Goal:** Create "Production-Ready" assets by generating DAW project files and creative loops.

### 3.1 Ableton Project Generator
* **Class:** `production/ableton_exporter.py` -> `AbletonProjectExporter`
* **XML Generation:** Use `xml.etree.ElementTree` to construct `.als` (gzipped XML) files.
* **Project Structure:**
    * Set Global BPM to match track analysis.
    * Create a **Group Track** containing 4 audio tracks.
    * **Color Coding:** Vocals (Pink), Drums (Green), Bass (Blue), Other (Orange).
    * **Warping:** Set warp markers based on `librosa` beat grid analysis.

### 3.2 Stem Remixer Utility
* **Class:** `production/stem_remixer.py` -> `StemRemixer`
* **Functions:**
    * `time_stretch_stem`: High-quality stretch (0.5x - 2.0x) using `librosa` or `rubberband`.
    * `create_vocal_chops`: Detect transients/onsets in the Vocal stem and slice into rhythmic one-shot `.wav` files.

---

## Step 4: Optimization, Testing & Deployment
**Timeline:** Weeks 14-16  
**Goal:** Harden the system for scale, manage hardware resources, and deploy.

### 4.1 GPU & Resource Management
* **Class:** `optimization/gpu_manager.py` -> `GPUManager`
* **Dynamic Batching:**
    * IF VRAM > 16GB: Batch Size = 4.
    * IF VRAM < 8GB: Batch Size = 1.
* **Acceleration:** Enable PyTorch TF32 (`torch.backends.cuda.matmul.allow_tf32 = True`) for Ampere+ GPUs.

### 4.2 Caching System
* **Class:** `optimization/stem_cache.py` -> `StemCache`
* **Logic:**
    * Key = `MD5(AudioFile) + EngineID`.
    * Store results to prevent re-processing identical files.
    * Implement `clear_cache(older_than_days=30)` for maintenance.

### 4.3 CLI & Interface
* **Tool:** `cli/stem_cli.py` (Click framework)
* **Commands:**
    * `separate <file> --engine auto`
    * `batch <dir> --output <dir>`
    * `export-ableton <stem_dir>`

---

## Technical Appendix: Requirements

**Hardware:**
* **GPU:** NVIDIA RTX 3060 (12GB) or higher recommended.
* **RAM:** 16GB minimum.
* **Storage:** NVMe SSD (Stems are large uncompressed WAV files).

**Core Python Libraries:**
```text
torch>=2.0.0
torchaudio>=2.0.0
demucs>=4.0.0
librosa>=0.10.0
soundfile>=0.12.0
music-tag>=0.4.3
numpy>=1.24.0
click>=8.1.0
requests>=2.31.0