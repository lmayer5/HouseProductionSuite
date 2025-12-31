#pragma once
#include <JuceHeader.h>
#include <array>
#include <string>
#include <vector>

namespace Melodic {

// Number of steps in a phrase (16th notes for 4 bars)
constexpr int NUM_PHRASE_STEPS = 64;

/**
 * Represents a single note event in the phrase.
 */
struct NoteEvent {
  float pitch = 60.0f;    // MIDI note number (can be fractional for microtonal)
  float velocity = 1.0f;  // 0.0 to 1.0
  float duration = 0.25f; // In beats (0.25 = 16th note)
  float probability = 1.0f; // 0.0 to 1.0, chance of triggering
  bool active = false;
};

/**
 * The complete phrase containing note events and scale context.
 */
struct Phrase {
  std::array<NoteEvent, NUM_PHRASE_STEPS> events;
  int rootNote = 60;                // C4 default
  juce::String scaleName = "Minor"; // Scale type
};

} // namespace Melodic
