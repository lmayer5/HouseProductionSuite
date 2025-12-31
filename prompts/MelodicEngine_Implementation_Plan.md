# Melodic Engine - Phased Implementation Plan

This document outlines the implementation steps for the **Melodic Engine** VST3 plugin. It is designed to be executed by an AI agent in a sequential, phased manner.

> [!IMPORTANT]
> The MelodyEngine plugin already has a basic skeleton in place (`PluginProcessor.h/cpp`, `PluginEditor.h/cpp`, `HouseSampler.h/cpp`). This plan builds upon that existing structure.

---

## Phase 1: Data Model & State Management

**Goal:** Define the data structures that enforce musicality (Scales, Keys, Phrases).

### Tasks

#### 1.1 Create `MelodyStructures.h`

Create a new file `plugins/MelodyEngine/Source/MelodyStructures.h` with the following C++ structs:

```cpp
#pragma once
#include <JuceHeader.h>
#include <array>
#include <vector>
#include <string>

namespace Melodic {

// Number of steps in a phrase (16th notes for 4 bars)
constexpr int NUM_PHRASE_STEPS = 64;

struct NoteEvent {
    float pitch = 60.0f;       // MIDI note number (can be fractional for microtonal)
    float velocity = 1.0f;     // 0.0 to 1.0
    float duration = 0.25f;    // In beats (0.25 = 16th note)
    float probability = 1.0f;  // 0.0 to 1.0, chance of triggering
    bool active = false;
};

struct Phrase {
    std::vector<NoteEvent> events;
    int rootNote = 60;              // C4 default
    juce::String scaleName = "Minor"; // Scale type
};

} // namespace Melodic
```

#### 1.2 Create `ScaleQuantizer` Helper Class

Create `plugins/MelodyEngine/Source/ScaleQuantizer.h` and `.cpp`:

- Define scale intervals for common house scales:
  - **Minor**: `{0, 2, 3, 5, 7, 8, 10}` (Natural Minor)
  - **Dorian**: `{0, 2, 3, 5, 7, 9, 10}`
  - **Mixolydian**: `{0, 2, 4, 5, 7, 9, 10}`
  - **Phrygian**: `{0, 1, 3, 5, 7, 8, 10}`
- Method: `int quantize(int midiNote, int rootNote, const juce::String& scaleType)`:
  - Takes any MIDI note and snaps it to the nearest note in the specified scale.

#### 1.3 Update `PluginProcessor`

- Add `Melodic::Phrase phrase;` member.
- Add `ScaleQuantizer quantizer;` member.
- Initialize a default 8th-note arpeggio pattern in `initializeDefaultPhrase()`.
- Implement `getStateInformation` / `setStateInformation` to serialize/deserialize the phrase as a `juce::ValueTree`.

### Verification
- Build successfully.
- Create a test that confirms `ScaleQuantizer::quantize(61, 60, "Minor")` returns `60` (C, not C#).

---

## Phase 2: DSP Engine (Wavetable Synthesis)

**Goal:** Build the sound generator using wavetable oscillators with morphing capability.

### Tasks

#### 2.1 Create `WavetableOscillator` Class

File: `plugins/MelodyEngine/Source/WavetableOscillator.h` and `.cpp`.

- Use a pre-computed lookup table (LUT) for common waveforms (Sine, Saw, Square, Pulse).
- Method: `float getSample(float phase)`.
- Method: `void setWaveform(int tableIndex)`.

#### 2.2 Create `WavetableSynth` Voice Class

File: `plugins/MelodyEngine/Source/WavetableSynth.h` and `.cpp`.

- Manages 2 `WavetableOscillator` instances (`osc1`, `osc2`).
- Expose a `morphAmount` parameter (0.0 - 1.0) that crossfades between the two oscillators.
- Integrate an `AdsrEnvelope` (from `djstih_dsp` module) for amplitude shaping.
- Integrate a simple 1-pole low-pass filter.

#### 2.3 Implement the Phrase Sequencer in `processBlock`

- On each `processBlock`:
  1. Get transport position from `getPlayHead()`.
  2. Calculate the current phrase step (based on BPM and samples elapsed).
  3. If a step boundary is crossed, check the `Phrase` for active notes.
  4. Apply probability (`random.nextFloat() <= event.probability`).
  5. If triggered, call `ScaleQuantizer::quantize()` and trigger the synth voice at that frequency.

#### 2.4 Add LFO for Filter Modulation

- Create a simple LFO class (Sine wave at ~0.1-10 Hz).
- Modulate the filter cutoff based on LFO output.
- Expose a `pulseAmount` parameter (0.0 - 1.0) that controls LFO depth.

### Verification
- Build successfully.
- Load in a DAW, hit play, and hear the default arpeggio pattern with morphable timbre.

---

## Phase 3: The UI Backbone (Melody Canvas)

**Goal:** Create a "Gesture Sequencer" grid for painting melodies.

### Tasks

#### 3.1 Create `MelodyCanvasComponent`

File: `plugins/MelodyEngine/Source/MelodyCanvasComponent.h` and `.cpp`.

- **Dimensions**: X-axis = Time (64 steps), Y-axis = Scale Degrees (1-7 within the selected octave range).
- **Background**: Dark industrial grid (use same colors as RhythmEngine: `0xFF0A0A0F`).
- **Notes**: Draw `NoteEvent` objects as horizontal neon bars.
  - **Height**: Proportional to velocity.
  - **Color**: Neon Purple (`0xFFCC00FF`), tonic notes brighter.

#### 3.2 Implement Interaction

- `mouseDown`: Adds a new note at the clicked position (snapped to scale degree and 1/16th grid).
- `mouseDrag`: Paints a stream of notes as the mouse moves.
- `mouseUp`: Ends the painting action.

#### 3.3 Playhead Animation

- Use a `juce::Timer` at 60 Hz.
- Draw a vertical cyan line that moves in sync with the DAW transport.

#### 3.4 Visual Chord Guides (Optional)

- Overlay faint vertical blocks indicating the underlying chord progression (e.g., i - iv - v - i).

### Verification
- Build successfully.
- UI opens and displays the canvas.
- Clicking adds notes that are audible when the DAW plays.

---

## Phase 4: Macros (XY Pad & Timbre Gear)

**Goal:** Abstract complex parameters into intuitive performance controls.

### Tasks

#### 4.1 Create `StylePad` Component (XY Control)

File: `plugins/MelodyEngine/Source/StylePadComponent.h` and `.cpp`.

- A touchable XY pad.
- **X-Axis (Density)**: Sparse (few notes) → Busy (many notes, arpeggios).
- **Y-Axis (Range)**: Static (chords, one octave) → Ascending (runs across octaves).
- On puck movement, algorithmically regenerate the `Phrase` data.

#### 4.2 Create `TimbreMorphWheel` Component

- A large rotary control (similar to a "Gear" dial).
- Controls:
  - Wavetable `morphAmount` (Phase 2).
  - Filter Cutoff.
- Visual: The center of the dial shows a spectral preview that shifts color/shape as the user rotates.

#### 4.3 Integrate into `PluginEditor`

- Layout: `MelodyCanvas` in the center, `StylePad` on the left, `TimbreMorphWheel` on the right.
- Window size: ~900x650 or similar.

### Verification
- Build successfully.
- Moving the XY pad changes the phrase in real-time.
- Rotating the morph wheel audibly changes the timbre.

---

## Phase 5: Visual Integration (The Cyberpunk Suite)

**Goal:** Unify the design with the Rhythm Engine to create a cohesive production suite.

### Tasks

#### 5.1 Apply "Glass Panel" Aesthetic

- `MelodyCanvas` should sit inside a rounded glass panel with a subtle gradient overlay.
- Use the same color palette: Neon Purple (`0xFFBC13FE`), Cyan (`0xFF00F3FF`), Deep Space Grey (`0xFF0A0A0F`).

#### 5.2 Create "Harmony Lock" Key Selector

- A dropdown or mechanical dial component.
- Options: All 12 chromatic roots (C, C#, D, ..., B) x Scale Types (Minor, Dorian, Mixolydian, etc.).
- When changed, the `MelodyCanvas` grid lines should visually shift to highlight the new "safe" notes.

#### 5.3 Modulation Cables (Optional Polish)

- Draw subtle animated lines connecting the `pulseAmount` macro to the filter section.
- The lines pulse with the LFO to visualize the modulation.

### Verification
- Build successfully.
- The plugin looks like it belongs in the same suite as the Rhythm Engine.
- All controls are functional and well-labeled.

---

## Summary Checklist

| Phase | Key Deliverables |
|-------|------------------|
| **1** | `MelodyStructures.h`, `ScaleQuantizer`, Phrase serialization |
| **2** | `WavetableOscillator`, `WavetableSynth`, LFO, Sequencer in `processBlock` |
| **3** | `MelodyCanvasComponent`, Playhead, Note painting |
| **4** | `StylePad`, `TimbreMorphWheel`, Macro integration |
| **5** | Glass UI, Harmony Lock Selector, Visual polish |

---

## Technical Reminders

- **Scale Quantization**: The Y-axis on the canvas should map to **Scale Degrees (1-7)**, not chromatic MIDI (0-127). This ensures the user cannot play "wrong" notes.
- **Wavetable LUT**: Use pre-calculated lookup tables for waveforms to reduce CPU usage. Avoid calling `std::sin()` in real-time loops.
- **Thread Safety**: The `Phrase` data is accessed from both the UI and audio thread. Use atomic operations or a lock-free mechanism for updates.
