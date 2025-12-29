import os
import shutil
from .config import DOWNLOAD_DIR

class Librarian:
    def __init__(self, base_dir=DOWNLOAD_DIR):
        self.base_dir = base_dir
        if not os.path.exists(self.base_dir):
            os.makedirs(self.base_dir)

    def organize_file(self, original_path, classification, audio_props):
        """
        Moves and renames the file based on its classification and properties.
        Format: Library/{Category}/{Subcategory}/[Key]_{OriginalName}_{BPM}.wav
        
        Note: YamNet classes are fine-grained (e.g. "Snare drum", "Bass drum").
        We might want to group them into broader categories if needed, 
        but for now we'll use the class name as the subfolder.
        """
        
        # Clean up classification name for folder safety
        category = "Samples" # Top level
        subcategory = classification.replace(' ', '_').replace('/', '-')
        
        # Destination folder
        dest_folder = os.path.join(self.base_dir, category, subcategory)
        if not os.path.exists(dest_folder):
            os.makedirs(dest_folder)
            
        # Extract filename parts
        filename = os.path.basename(original_path)
        name_only, ext = os.path.splitext(filename)
        
        # Construct new filename
        # [Key]_[Name]_[BPM].wav
        # If Key is unknown, maybe omit it or keep as Unknown.
        key_str = f"[{audio_props['key']}]" if audio_props['key'] != 'Unknown' else ""
        bpm_str = f"_{audio_props['bpm']}bpm" if audio_props['bpm'] > 0 else ""
        
        new_filename = f"{key_str}{name_only}{bpm_str}{ext}"
        # Clean double underscores if any part is missing
        new_filename = new_filename.replace('__', '_').strip('_')
        
        dest_path = os.path.join(dest_folder, new_filename)
        
        try:
            shutil.move(original_path, dest_path)
            print(f"Filed: {original_path} -> {dest_path}")
            return dest_path
        except Exception as e:
            print(f"Error moving file {original_path}: {e}")
            return None
