import librosa
import numpy as np

class Analyzer:
    def __init__(self):
        pass

    def check_audio_properties(self, file_path):
        """
        Analyzes Key and BPM of the audio file.
        Returns: { 'bpm': float, 'key': str }
        """
        try:
            y, sr = librosa.load(file_path, sr=None)
            
            # Detect BPM
            tempo, _ = librosa.beat.beat_track(y=y, sr=sr)
            # tempo is usually a scalar, but can be an array in newer librosa versions
            bpm = float(tempo) if np.isscalar(tempo) else float(tempo[0])
            
            # Detect Key (Simple Chroma based approach)
            # This is a basic estimation. For robust key detection, we'd need more complex profiles.
            chroma = librosa.feature.chroma_cqt(y=y, sr=sr)
            # Sum chroma features over time
            chroma_sum = np.sum(chroma, axis=1)
            # Find the pitch class with max energy
            pitch_class = np.argmax(chroma_sum)
            
            # Map index to Note Name
            notes = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']
            key = notes[pitch_class]
            
            # Currently we don't determine Major/Minor easily without template matching.
            # Returning Root Note for now.
            
            return {
                'bpm': round(bpm, 1),
                'key': key
            }
            
        except Exception as e:
            print(f"Error analyzing properties for {file_path}: {e}")
            return {'bpm': 0, 'key': 'Unknown'}
