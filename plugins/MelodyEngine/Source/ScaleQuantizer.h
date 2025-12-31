#pragma once
#include <JuceHeader.h>
#include <map>
#include <vector>


class ScaleQuantizer {
public:
  ScaleQuantizer();
  ~ScaleQuantizer() = default;

  /**
   * Snaps a MIDI note to the nearest note in the given scale.
   */
  int quantize(int midiNote, int rootNote, const juce::String &scaleType);

  /**
   * Returns the intervals of the specified scale.
   */
  std::vector<int> getScaleIntervals(const juce::String &scaleType);

private:
  std::map<juce::String, std::vector<int>> scales;
};
