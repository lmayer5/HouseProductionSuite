#pragma once
#include <JuceHeader.h>
#include <array>
#include <vector>

namespace RhythmEngine {

// Number of steps in the pattern (16th notes for a 4-bar pattern at 4 steps per
// beat)
constexpr int NUM_STEPS = 16;

// Number of drum tracks
constexpr int NUM_TRACKS = 4;

// Track indices for clarity
enum class TrackId : int { Kick = 0, Bass = 1, Hat = 2, Clap = 3 };

/**
 * Represents a single step in the sequencer grid.
 */
struct Step {
  bool active = false;   // Is this step triggered?
  float velocity = 1.0f; // 0.0 to 1.0, affects volume/intensity
  float probability =
      1.0f; // 0.0 to 1.0, chance of triggering (for humanization)
};

/**
 * Represents a single track (row) in the sequencer.
 */
struct Track {
  std::array<Step, NUM_STEPS> steps{}; // Fixed-size array of steps
  int midiNote = 36;                   // MIDI note to trigger (for reference)
  juce::String name;                   // Display name
};

/**
 * The complete drum pattern containing all tracks.
 */
struct Pattern {
  std::array<Track, NUM_TRACKS> tracks{};

  // Convenience accessors
  Track &kick() { return tracks[static_cast<int>(TrackId::Kick)]; }
  Track &bass() { return tracks[static_cast<int>(TrackId::Bass)]; }
  Track &hat() { return tracks[static_cast<int>(TrackId::Hat)]; }
  Track &clap() { return tracks[static_cast<int>(TrackId::Clap)]; }

  const Track &kick() const { return tracks[static_cast<int>(TrackId::Kick)]; }
  const Track &bass() const { return tracks[static_cast<int>(TrackId::Bass)]; }
  const Track &hat() const { return tracks[static_cast<int>(TrackId::Hat)]; }
  const Track &clap() const { return tracks[static_cast<int>(TrackId::Clap)]; }
};

} // namespace RhythmEngine
