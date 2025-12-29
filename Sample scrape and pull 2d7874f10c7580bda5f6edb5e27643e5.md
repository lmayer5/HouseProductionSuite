# Sample scrape and pull

Building a custom AI-powered sample scraper is a sophisticated engineering project that sits at the intersection of **web scraping**, **digital signal processing (DSP)**, and **machine learning**.

Because you intend to use this for music production, **audio quality** (bitrate, sample rate) and **legal safety** (copyright clearance) are critical constraints. The following report outlines a professional-grade architecture for building this system.

### **Executive Summary**

- **The Problem:** Generic web scraping is legally risky and often yields low-quality audio (128kbps MP3s).
- **The Solution:** Build a "Smart Harvester" that targets API-accessible, royalty-free databases (like [Freesound.org](http://freesound.org/)) rather than scraping UI-heavy sites (like YouTube).
- **The Stack:** Python is the industry standard for this. You will use **Freesound API** for sourcing, **Librosa** for audio analysis, **YamNet (TensorFlow)** for instrument classification, and **SQLite** for library management.

---

### **1. Sourcing Strategy: API vs. Scraping**

Do not scrape generic websites (YouTube, Spotify, Splice). They serve compressed audio (bad for production), actively block scrapers, and expose you to copyright lawsuits. Instead, target **Creative Commons Zero (CC0)** repositories which allow commercial use without attribution.

| Source | Method | Pros | Cons |
| --- | --- | --- | --- |
| [**Freesound.org**](http://freesound.org/) | **REST API** | Huge database, filters for CC0, high-quality WAV access. | Rate limits (requires an API key). |
| **Internet Archive** | **Python Wrapper** | Massive unstructured library. | Hard to filter for "musical" quality. |
| **Generic Web** | `BeautifulSoup` / `Selenium` | Access to anything. | **Legal risk**, low audio quality, brittle code. |

**Recommendation:** Build your MVP (Minimum Viable Product) around the **Freesound API**. It provides metadata (tags) that you can use to validate your AI's sorting.

---

### **2. System Architecture**

Your software should operate as a pipeline with four distinct stages.

### **Stage A: The Harvester (Scraping)**

- **Input:** User keywords (e.g., "Analog Kick", "Jazz Chord").
- **Action:** Queries the Freesound API for sounds with `license:"Creative Commons 0"`, `type:"wav"`, and `duration:<10s`.
- **Tool:** `requests` (Python library).

### **Stage B: The Gatekeeper (Quality Control)**

- **Action:** Before saving, analyze the file. Reject if it is:
    - **Low Quality:** Sample rate < 44.1kHz or Bit depth < 16-bit.
    - **Clipped:** Detects digital distortion.
    - **Duplicate:** Checks audio fingerprint to see if you already have this sound.
- **Tool:** `librosa` (analysis), `chromaprint` (duplicate detection).

### **Stage C: The Sorter (AI Classification)**

- **Action:** Ignore the file name. Listen to the audio to determine what it *actually* is.
    - *Is it a Kick, Snare, or Hi-Hat?*
    - *Is it a One-shot or a Loop?*
- **Tool:** `TensorFlow` (running the YamNet model) or `Essentia`.

### **Stage D: The Librarian (Storage)**

- **Action:** Renames and moves the file into your directory structure.
- **Structure:** `Library/Drums/Kicks/Analog_Kick_Fm_120BPM.wav`

---

### **3. Technical Implementation Details**

### **Step 1: Audio Analysis & Quality Check**

You need to filter out "trash" audio. Use **Librosa**, the standard library for audio analysis in Python.

```python
import librosa
import numpy as np

def check_quality(file_path):
    # Load audio
    y, sr = librosa.load(file_path, sr=None)

    # 1. Check Sample Rate (Standard is 44100Hz)
    if sr < 44100:
        return False, "Low Sample Rate"

    # 2. Check Duration (Ignore 5-minute field recordings if you want samples)
    duration = librosa.get_duration(y=y, sr=sr)
    if duration > 10.0:
        return False, "Too Long"

    # 3. Detect Clipping (Bad audio)
    if np.max(np.abs(y)) >= 1.0:
        return False, "Clipped/Distorted"

    return True, "Quality OK"

```

### **Step 2: AI Tagging (The "Brain")**

To sort samples automatically, you need an Audio Classifier.

- **YamNet:** A pre-trained deep learning model by Google that can recognize 521 audio events, including "Kick drum", "Snare drum", "Cymbal", "Bass guitar", etc.
- **How to use:** You pass the waveform to YamNet, and it returns the probability of each class.

```python
import tensorflow_hub as hub

# Load the model
model = hub.load('<https://tfhub.dev/google/yamnet/1>')

def classify_sound(waveform):
    scores, embeddings, spectrogram = model(waveform)
    # Get the top prediction class
    prediction = scores.numpy().mean(axis=0).argmax()
    return class_names[prediction] # Returns "Snare drum" or "Bass"

```

### **Step 3: Key & BPM Detection**

For loops, you need to know the Tempo and Key to make them usable.

- **BPM:** `librosa.beat.tempo(y=y, sr=sr)`
- **Key:** `librosa.feature.chroma_cqt(y=y, sr=sr)` (Extracts pitch class profile).

---

### **4. Recommended Tech Stack**

| Component | Technology | Why? |
| --- | --- | --- |
| **Language** | **Python 3.10+** | Unrivaled ecosystem for AI and Audio processing. |
| **API Client** | **Requests** | Simple, robust HTTP handling for APIs. |
| **Audio Processing** | **Librosa** | The "Swiss Army Knife" of audio analysis. |
| **Fingerprinting** | **Chromaprint / AcoustID** | Generates a unique hash for audio to find duplicates even if filenames differ. |
| **Database** | **SQLite** | Lightweight local database to store metadata (tags, source URL) without needing a server. |
| **AI Model** | **YamNet (TensorFlow)** | Pre-trained, effective, and free. No need to train your own model from scratch. |

### **5. Legal Warning: The "Royalty-Free" Trap**

**Do not skip this.**
In music production, you cannot use a sample just because you "found it on the internet."

- **Copyright Infringement:** If you scrape a snare hit from a copyrighted song and use it in a hit record, you can be sued for 100% of the royalties.
- **Safe Harbor:** Only download files with **Creative Commons Zero (CC0)** or **Attribution (CC-BY)** licenses.
- **Your Code's Duty:** Your scraper *must* verify the license field in the API metadata before downloading.

### **6. Development Roadmap**

1. **Week 1 (The Crawler):** Write a Python script to authenticate with Freesound API and download 10 CC0 WAV files.
2. **Week 2 (The Analyst):** Implement `librosa` to reject files under 44.1kHz.
3. **Week 3 (The Sorter):** Integrate `YamNet` to classify the 10 files into folders (Kicks vs Snares).
4. **Week 4 (The UI):** (Optional) Build a simple drag-and-drop interface or CLI (Command Line Interface) to run the tool.

### **Summary of Output Folder Structure**

Your software should automatically organize the chaos of the web into this:

```
/My_AI_Scraped_Library
    /Drums
        /Kick
            [C] Kick_Analog_Hard_120bpm.wav  <-- Renamed with Key/BPM
            [G] Kick_808_Sub.wav
        /Snare
        /Hats
    /Loops
        /120BPM
        /140BPM
    /FX
        /Risers

```

Sources
[1] How would I go about scraping CC licensed sounds from [FreeSound.org](http://freesound.org/)? [https://www.reddit.com/r/DataHoarder/comments/ikub0t/how_would_i_go_about_scraping_cc_licensed_sounds/](https://www.reddit.com/r/DataHoarder/comments/ikub0t/how_would_i_go_about_scraping_cc_licensed_sounds/)
[2] How to detect the bpm of a song? : r/AskProgramming [https://www.reddit.com/r/AskProgramming/comments/1klsx74/how_to_detect_the_bpm_of_a_song/](https://www.reddit.com/r/AskProgramming/comments/1klsx74/how_to_detect_the_bpm_of_a_song/)
[3] Automatically tag and organize your music library using ... [https://www.reddit.com/r/plexamp/comments/1pgkjuo/musicautotagger_automatically_tag_and_organize/](https://www.reddit.com/r/plexamp/comments/1pgkjuo/musicautotagger_automatically_tag_and_organize/)
[4] Freesound - Wikipedia [https://en.wikipedia.org/wiki/Freesound.org](https://en.wikipedia.org/wiki/Freesound.org)
[5] Bpm audio detection Library [https://stackoverflow.com/questions/477944/bpm-audio-detection-library](https://stackoverflow.com/questions/477944/bpm-audio-detection-library)
[6] One Tagger - free open source cross-platform music tagger for ... [https://onetagger.github.io](https://onetagger.github.io/)
[7] [freesound.org](http://freesound.org/): a website where you can legally get sounds for commercial use for free [https://www.reddit.com/r/edmproduction/comments/gok40u/freesoundorg_a_website_where_you_can_legally_get/](https://www.reddit.com/r/edmproduction/comments/gok40u/freesoundorg_a_website_where_you_can_legally_get/)
[8] How to get BPM and tempo audio features in Python [closed] [https://stackoverflow.com/questions/8635063/how-to-get-bpm-and-tempo-audio-features-in-python](https://stackoverflow.com/questions/8635063/how-to-get-bpm-and-tempo-audio-features-in-python)
[9] Sampling Meets AI: Manage & Organize Sample Library [https://www.musicradar.com/news/sampling-meets-ai-how-artificial-intelligence-can-help-you-manage-and-organize-your-sample-library](https://www.musicradar.com/news/sampling-meets-ai-how-artificial-intelligence-can-help-you-manage-and-organize-your-sample-library)
[10] [Guide] Ripping The Free Music Archive! [https://www.reddit.com/r/DataHoarder/comments/1prnu9/guide_ripping_the_free_music_archive/](https://www.reddit.com/r/DataHoarder/comments/1prnu9/guide_ripping_the_free_music_archive/)
[11] aabalke/drum-audio-classifier [https://github.com/aabalke33/drum-audio-classifier](https://github.com/aabalke33/drum-audio-classifier)
[12] Audio Classification Model for Music Transcription [https://pub.towardsai.net/building-an-audio-classification-model-for-automatic-drum-transcription-heres-what-i-learnt-a84f03d53413](https://pub.towardsai.net/building-an-audio-classification-model-for-automatic-drum-transcription-heres-what-i-learnt-a84f03d53413)
[13] I made a command-line tool to find similar sounding audio ... [https://www.reddit.com/r/Python/comments/fwgtu6/i_made_a_commandline_tool_to_find_similar/](https://www.reddit.com/r/Python/comments/fwgtu6/i_made_a_commandline_tool_to_find_similar/)
[14] Legal issues with Local LLMs scraping websites and using ... [https://www.reddit.com/r/LocalLLaMA/comments/1c74idm/legal_issues_with_local_llms_scraping_websites/](https://www.reddit.com/r/LocalLLaMA/comments/1c74idm/legal_issues_with_local_llms_scraping_websites/)
[15] Detecting and Classifying Musical Instruments with ... [http://cs230.stanford.edu/projects_winter_2021/reports/70770755.pdf](http://cs230.stanford.edu/projects_winter_2021/reports/70770755.pdf)
[16] Transfer Learning for the Audio Domain with TensorFlow Lite ... [https://ai.google.dev/edge/litert/libraries/modify/audio_classification](https://ai.google.dev/edge/litert/libraries/modify/audio_classification)
[17] Experimenting with acoustic fingerprinting for duplicate detection [https://www.augmentedmind.de/2021/02/14/acoustic-fingerprinting-experiment/](https://www.augmentedmind.de/2021/02/14/acoustic-fingerprinting-experiment/)
[18] Web Scraping Legal Issues: 2025 Enterprise Compliance ... [https://groupbwt.com/blog/is-web-scraping-legal/](https://groupbwt.com/blog/is-web-scraping-legal/)
[19] Musical Instrument Identification Using Deep Learning ... [https://pmc.ncbi.nlm.nih.gov/articles/PMC9025072/](https://pmc.ncbi.nlm.nih.gov/articles/PMC9025072/)
[20] 3 Ways to Classify Drum Sounds [https://www.soundsandwords.io/drum-sound-classification/](https://www.soundsandwords.io/drum-sound-classification/)

[Prompts](https://www.notion.so/Prompts-2d7874f10c75803da81ce363627b8c33?pvs=21)