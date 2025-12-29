import tensorflow as tf
import tensorflow_hub as hub
import numpy as np
import csv
import io
import librosa
import os

# YamNet model handle
YAMNET_MODEL_HANDLE = 'https://tfhub.dev/google/yamnet/1'

class Sorter:
    def __init__(self):
        print("Loading YamNet model...")
        self.model = hub.load(YAMNET_MODEL_HANDLE)
        self.class_names = self.load_class_names()
        print("YamNet model loaded.")

    def load_class_names(self):
        """
        Load component to get class names from the CSV file included in the model.
        The default YamNet model exposes the class map.
        """
        class_map_path = self.model.class_map_path().numpy().decode('utf-8')
        class_names = []
        with tf.io.gfile.GFile(class_map_path) as csvfile:
            reader = csv.DictReader(csvfile)
            for row in reader:
                class_names.append(row['display_name'])
        return class_names

    def classify(self, file_path):
        """
        Classifies the audio file and returns the top predicted class.
        Returns: (str) Class Name
        """
        # Load audio at 16k Hz as required by YamNet
        wav_data, sr = librosa.load(file_path, sr=16000, mono=True)
        
        # Run model (YamNet expects normalized float32 waveform)
        scores, embeddings, spectrogram = self.model(wav_data)
        
        # Scores is a matrix of (N, 521) where N is number of frames (approx 2 per sec)
        # We can average the scores across all frames to get the overall prediction
        mean_scores = np.mean(scores, axis=0)
        top_class_index = np.argmax(mean_scores)
        top_class = self.class_names[top_class_index]
        
        return top_class
