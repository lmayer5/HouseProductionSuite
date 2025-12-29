import requests
import os
import time
from .config import BASE_URL, FREESOUND_API_KEY, DEFAULT_LICENSE, DOWNLOAD_DIR

class FreesoundCrawler:
    def __init__(self, api_key=FREESOUND_API_KEY):
        self.api_key = api_key
        if not self.api_key:
            raise ValueError("API Key is missing. Please check your .env file.")
        
        self.headers = {
            "Authorization": f"Token {self.api_key}"
        }

    def search(self, query, duration_max=10, limit=10):
        """
        Search for sounds on Freesound.
        """
        params = {
            "query": query,
            "filter": f"license:\"{DEFAULT_LICENSE}\" type:wav duration:[0 TO {duration_max}]",
            "fields": "id,name,previews,download,type,duration,description,tags",
            "page_size": limit,
            "sort": "rating_desc"
        }
        
        print(f"Searching for: {query}...")
        response = requests.get(f"{BASE_URL}/search/text/", params=params, headers=self.headers)
        
        if response.status_code != 200:
            print(f"Error searching: {response.status_code} - {response.text}")
            return []
            
        data = response.json()
        return data.get('results', [])

    def download_sound(self, sound_data, output_dir=DOWNLOAD_DIR):
        """
        Download a sound file from Freesound.
        Note: The 'download' field in APIv2 usually requires OAuth2 for high quality original files.
        For simpler token-based access, we often have to use previews OR perform proper OAuth flow.
        However, the prompt implies we should try to get high quality. 
        
        If we only have Token auth, we might be limited to what the API allows.
        According to Freesound docs, downloading the original file requires OAuth2 or Cookie based auth for some resources, 
        but some docs say Token is enough for 'download' if the user allows it? 
        Actually, for 'download' link, it typically requires OAuth2 (3-legged). 
        
        For this MVP, we will try the 'download' link with the Token header. 
        If that fails, we might fall back to high-quality previews or warn the user.
        
        Actually, Freesound API documentation says:
        "Downloading sounds... requires OAuth2 authentication."
        "You can however download low quality previews without OAuth2."
        
        Wait, for the purpose of this "Smart Harvester" which emphasizes "Audio Quality", getting previews (usually MP3) is bad.
        But doing full OAuth2 flow in a CLI script is complex (requires browser redirect).
        
        Let's try to use the Token. If it fails, I will add a method to download the HQ preview as a fallback 
        or simply report the limitation.
        
        BUT, let's look at similar projects. Some users use their browser cookies.
        For now, I will implement the download request using the provided download URI.
        """
        
        # Ensure output directory exists
        if not os.path.exists(output_dir):
            os.makedirs(output_dir)
            
        sound_id = sound_data['id']
        name = sound_data['name'].replace('/', '_').replace('\\', '_')
        filename = f"{sound_id}_{name}.wav" # Assuming WAV as per filter
        file_path = os.path.join(output_dir, filename)
        
        if os.path.exists(file_path):
            print(f"File already exists: {filename}")
            return file_path

        download_url = sound_data['download']
        
        print(f"Downloading {name}...")
        # Note: The download URL usually redirects.
        # We need to pass the Authorization header.
        
        try:
            with requests.get(download_url, headers=self.headers, stream=True) as r:
                if r.status_code == 200:
                    with open(file_path, 'wb') as f:
                        for chunk in r.iter_content(chunk_size=8192):
                            f.write(chunk)
                    print(f"Saved to {file_path}")
                    return file_path
                elif r.status_code == 401:
                    print(f"Unauthorized (401) for download. OAuth2 might be required for original files.")
                    # Fallback to preview-hq-ogg or preview-hq-mp3 if available, but goal is WAV.
                    # For now just return None to signal failure.
                    return None
                else:
                    print(f"Failed to download: {r.status_code}")
                    return None
        except Exception as e:
            print(f"Exception during download: {e}")
            return None
