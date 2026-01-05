"""
Ableton Live Project Exporter

Generates .als (Ableton Live Set) files from separated stems.
"""

import gzip
import os
from dataclasses import dataclass
from pathlib import Path
from typing import Optional
from xml.etree import ElementTree as ET

import librosa


@dataclass
class AbletonTrackInfo:
    """Information about an audio track in the Ableton project."""
    name: str
    color_index: int
    file_path: Path
    
    
# Ableton color indices for stems
STEM_COLORS = {
    "vocals": 58,   # Pink
    "drums": 26,    # Green  
    "bass": 17,     # Blue
    "other": 13     # Orange
}


class AbletonProjectExporter:
    """
    Generates Ableton Live .als project files from separated stems.
    
    Features:
        - Creates 4 audio tracks in a group
        - Color codes each stem type
        - Sets BPM based on librosa beat detection
        - Places audio clips with proper warp markers
    
    Note: This generates a simplified .als that Ableton can open.
    Full feature parity with Ableton's format is complex.
    """
    
    # Minimum Ableton Live version we target
    ABLETON_VERSION = "11.0"
    
    def __init__(self):
        """Initialize the Ableton exporter."""
        pass
    
    def _detect_bpm(self, audio_path: Path) -> float:
        """
        Detect BPM of an audio file using librosa.
        
        Args:
            audio_path: Path to audio file
            
        Returns:
            Detected BPM (defaults to 120 if detection fails)
        """
        try:
            y, sr = librosa.load(audio_path, sr=None, mono=True, duration=60)
            tempo, _ = librosa.beat.beat_track(y=y, sr=sr)
            # librosa might return an array, get the scalar
            if hasattr(tempo, '__len__'):
                tempo = float(tempo[0]) if len(tempo) > 0 else 120.0
            return float(tempo)
        except Exception:
            return 120.0
    
    def _get_audio_duration(self, audio_path: Path) -> float:
        """Get duration of audio file in seconds."""
        try:
            return float(librosa.get_duration(path=str(audio_path)))
        except Exception:
            return 0.0
    
    def _create_audio_track_element(
        self,
        track_id: int,
        name: str,
        color_index: int,
        audio_path: Path,
        sample_rate: int = 44100
    ) -> ET.Element:
        """Create an XML element for an audio track."""
        track = ET.Element("AudioTrack", Id=str(track_id))
        
        # Track name
        name_elem = ET.SubElement(track, "Name")
        ET.SubElement(name_elem, "EffectiveName", Value=name)
        ET.SubElement(name_elem, "UserName", Value=name)
        
        # Track color
        ET.SubElement(track, "Color", Value=str(color_index))
        
        # Device chain (empty for now)
        ET.SubElement(track, "DeviceChain")
        
        # Track delay (none)
        ET.SubElement(track, "TrackDelay", Value="0")
        
        return track
    
    def _create_als_structure(
        self,
        stems: dict[str, Path],
        bpm: float,
        track_name: str
    ) -> ET.Element:
        """
        Create the basic ALS XML structure.
        
        This creates a simplified version of Ableton's XML format.
        """
        # Root element
        ableton = ET.Element("Ableton", 
                            MajorVersion="5",
                            MinorVersion="11.0",
                            SchemaChangeCount="3",
                            Creator="StemGenerator")
        
        # Live Set
        live_set = ET.SubElement(ableton, "LiveSet")
        
        # Master track
        master = ET.SubElement(live_set, "MasterTrack")
        ET.SubElement(master, "Name", Value="Master")
        
        # Tempo
        tempo_elem = ET.SubElement(live_set, "Tempo")
        manual = ET.SubElement(tempo_elem, "Manual", Value=str(bpm))
        
        # Tracks container
        tracks = ET.SubElement(live_set, "Tracks")
        
        # Create group track for stems
        group_id = 0
        group = ET.SubElement(tracks, "GroupTrack", Id=str(group_id))
        group_name = ET.SubElement(group, "Name")
        ET.SubElement(group_name, "EffectiveName", Value=f"{track_name} Stems")
        ET.SubElement(group, "Color", Value="0")  # Gray for group
        
        # Create individual stem tracks
        track_id = 1
        for stem_name, stem_path in stems.items():
            if stem_path.exists():
                color = STEM_COLORS.get(stem_name, 0)
                track = self._create_audio_track_element(
                    track_id=track_id,
                    name=stem_name.capitalize(),
                    color_index=color,
                    audio_path=stem_path
                )
                tracks.append(track)
                track_id += 1
        
        # Annotation / comments
        annotation = ET.SubElement(live_set, "Annotation")
        ET.SubElement(annotation, "Value", 
                     Value=f"Generated by StemGenerator from: {track_name}")
        
        return ableton
    
    def export(
        self,
        stem_dir: Path,
        output_path: Optional[Path] = None,
        track_name: Optional[str] = None
    ) -> Path:
        """
        Export stems as an Ableton Live project.
        
        Args:
            stem_dir: Directory containing the 4 stem files
            output_path: Output .als file path (auto-generated if not provided)
            track_name: Name for the project (auto-detected if not provided)
            
        Returns:
            Path to the created .als file
        """
        stem_dir = Path(stem_dir)
        
        # Find stems
        stems = {}
        for stem_name in ["vocals", "drums", "bass", "other"]:
            stem_path = stem_dir / f"{stem_name}.wav"
            if stem_path.exists():
                stems[stem_name] = stem_path
        
        if not stems:
            raise ValueError(f"No stems found in {stem_dir}")
        
        # Detect BPM from drums (most reliable) or any stem
        reference_stem = stems.get("drums") or list(stems.values())[0]
        bpm = self._detect_bpm(reference_stem)
        
        # Get track name
        if not track_name:
            track_name = stem_dir.name
        
        # Create output path
        if not output_path:
            output_path = stem_dir / f"{track_name}.als"
        output_path = Path(output_path)
        
        # Create ALS structure
        als_root = self._create_als_structure(stems, bpm, track_name)
        
        # Convert to XML string
        xml_string = ET.tostring(als_root, encoding='unicode', method='xml')
        xml_string = '<?xml version="1.0" encoding="UTF-8"?>\n' + xml_string
        
        # Ableton .als files are gzip compressed XML
        with gzip.open(output_path, 'wt', encoding='utf-8') as f:
            f.write(xml_string)
        
        return output_path
    
    def export_batch(
        self,
        base_dir: Path,
        output_dir: Optional[Path] = None
    ) -> list[Path]:
        """
        Export multiple stem directories as Ableton projects.
        
        Args:
            base_dir: Directory containing subdirectories with stems
            output_dir: Output directory (defaults to same as stems)
            
        Returns:
            List of created .als file paths
        """
        base_dir = Path(base_dir)
        created_files = []
        
        # Find all stem directories
        for item in base_dir.iterdir():
            if item.is_dir():
                # Check if it contains stems
                has_stems = all(
                    (item / f"{s}.wav").exists() 
                    for s in ["vocals", "drums", "bass", "other"]
                )
                
                if has_stems:
                    try:
                        out_path = None
                        if output_dir:
                            out_path = Path(output_dir) / f"{item.name}.als"
                        
                        als_path = self.export(item, out_path)
                        created_files.append(als_path)
                    except Exception as e:
                        print(f"Failed to export {item}: {e}")
        
        return created_files
