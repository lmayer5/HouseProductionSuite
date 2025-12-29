import os
from dotenv import load_dotenv

# Load environment variables from .env file
load_dotenv()

# Freesound API Configuration
FREESOUND_API_KEY = os.getenv("FREESOUND_API_KEY")
BASE_URL = "https://freesound.org/apiv2"

# Crawler Configuration
DEFAULT_LICENSE = "Creative Commons 0"
DEFAULT_DURATION_MAX = 10  # seconds
DEFAULT_Sample_RATE_MIN = 44100
DOWNLOAD_DIR = "My_AI_Scraped_Library"

# Validation
if not FREESOUND_API_KEY:
    print("Warning: FREESOUND_API_KEY not found in environment variables.")
