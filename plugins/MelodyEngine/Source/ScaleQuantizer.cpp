#include "ScaleQuantizer.h"

ScaleQuantizer::ScaleQuantizer() {
  scales["Minor"] = {0, 2, 3, 5, 7, 8, 10};
  scales["Dorian"] = {0, 2, 3, 5, 7, 9, 10};
  scales["Mixolydian"] = {0, 2, 4, 5, 7, 9, 10};
  scales["Phrygian"] = {0, 1, 3, 5, 7, 8, 10};
}

int ScaleQuantizer::quantize(int midiNote, int rootNote,
                             const juce::String &scaleType) {
  auto intervals = getScaleIntervals(scaleType);

  int octave = midiNote / 12;
  int noteInOctave = midiNote % 12;
  int rootInOctave = rootNote % 12;

  // Normalize note to be relative to rootNote 0
  int relativeNote = (noteInOctave - rootInOctave + 12) % 12;

  int bestDist = 12;
  int bestNote = 0;

  for (int interval : intervals) {
    int dist = std::abs(relativeNote - interval);
    if (dist < bestDist) {
      bestDist = dist;
      bestNote = interval;
    }

    // Also check distance across octave wrap
    dist = std::abs(relativeNote - (interval + 12));
    if (dist < bestDist) {
      bestDist = dist;
      bestNote = interval;
    }

    dist = std::abs(relativeNote - (interval - 12));
    if (dist < bestDist) {
      bestDist = dist;
      bestNote = interval;
    }
  }

  // Reconstruct the MIDI note
  int quantizedRelative = bestNote;
  int result = (octave * 12) + rootInOctave + quantizedRelative;

  // Ensure we are close to the target octave if shifting caused a jump
  if (std::abs(result - midiNote) > 6) {
    if (result > midiNote)
      result -= 12;
    else
      result += 12;
  }

  return juce::jlimit(0, 127, result);
}

std::vector<int>
ScaleQuantizer::getScaleIntervals(const juce::String &scaleType) {
  if (scales.find(scaleType) != scales.end()) {
    return scales[scaleType];
  }
  return scales["Minor"]; // Default to minor
}
