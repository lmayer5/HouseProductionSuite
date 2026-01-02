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
 * Step modifiers inspired by Teenage Engineering OP-XY "Step Components".
 * These modify HOW a step triggers rather than just IF it triggers.
 */
enum class StepModifier : uint8_t {
  None = 0,        // Normal trigger
  Ratchet2,        // Repeat note 2x within step duration
  Ratchet4,        // Repeat note 4x within step duration
  Glide,           // Parameter slide (for 808 pitch bends, filter sweeps)
  SkipCycle,       // Play every other loop (modulo 2)
  OnlyFirstCycle   // Play only on the first loop iteration
};

/**
 * Represents a single step in the sequencer grid.
 */
struct Step {
  bool active = false;   // Is this step triggered?
  float velocity = 1.0f; // 0.0 to 1.0, affects volume/intensity
  float probability =
      1.0f; // 0.0 to 1.0, chance of triggering (for humanization)
  StepModifier modifier = StepModifier::None; // TE-style step component
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
