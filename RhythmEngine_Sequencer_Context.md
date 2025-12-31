# The Rhythm Engine - Sequencer Project Context

> **Date Created:** 2025-12-31  
> **Project:** House Production Suite  
> **New Feature:** House Music Sequencer VST3 with Cyberpunk/Neon Industrial aesthetic

---

## 📊 Current State of the Project

### Existing Structure

The `House Production Suite` already has a well-organized JUCE 8 / C++20 project with:

| Component | Status | Description |
|-----------|--------|-------------|
| **RhythmEngine** | ✅ Built | A drum/bass synthesizer (not a sequencer) - triggers via MIDI input |
| **MelodyEngine** | 🔨 Skeleton | Placeholder for polyphonic synth |
| **SampleEngine** | 📁 Exists | Folder structure only |
| **djstih_dsp** | ✅ Complete | Shared DSP module with `Phasor`, `AdsrEnvelope`, `ClipFunctions` |

### Current RhythmEngine Architecture

The existing `RhythmEngine` is a **synthesizer**, not a sequencer:

- **Kick synthesis**: Sine wave with pitch sweep via `Phasor` + `AdsrEnvelope`
- **Bass synthesis**: Sawtooth → 1-pole LPF → soft clipping → sidechain ducking
- **MIDI-triggered**: Responds to note 36 (C1) for kick, ≥48 (C2+) for bass
- **Parameters**: Kick Freq/Decay, Bass Cutoff/Drive/Attack/Decay, Sidechain Amount
- **State**: Uses `APVTS` with XML serialization via `getStateInformation`/`setStateInformation`

### Key Files

```
/plugins/RhythmEngine/Source/
├── PluginProcessor.h/cpp    # Audio processing, MIDI handling, state management
├── PluginEditor.h/cpp       # UI with rotary sliders (Kick/Bass sections)
├── Parameters.h             # Parameter ID definitions (alternate layout, not currently used)
└── Voices/
    ├── KickVoice.h
    └── BassVoice.h

/modules/djstih_dsp/
└── djstih_dsp.h             # Phasor, AdsrEnvelope, ClipFunctions
```

---

## 🆕 New "Rhythm Engine" Sequencer vs. Existing

The new plan describes a **fundamentally different plugin**:

| Aspect | Current RhythmEngine | New "Rhythm Engine" Sequencer |
|--------|---------------------|------------------------------|
| **Type** | MIDI-triggered synthesizer | Internal step sequencer |
| **Core Feature** | Sound generation (Kick/Bass) | Pattern grid + MIDI output |
| **UI Focus** | Parameter knobs | 16-step grid + "Macro Gears" |
| **Output** | Audio (stereo) | MIDI Effect (then Audio later?) |
| **Aesthetic** | Simple dark/cyan | Cyberpunk/Neon Industrial |

---

## 🎯 Implementation Plan Overview

### Phase 1: The Data Model & State Management

**Objective:** Define how the drum pattern exists in memory and how it saves/loads within the DAW.

**New Data Structures Required:**

```cpp
// RhythmStructures.h
struct Step {
    bool active = false;
    float velocity = 1.0f;
    float probability = 1.0f;
};

struct Track {
    std::vector<Step> steps;  // 16 steps
    int midiNote;             // e.g., 36 for Kick
};

struct Pattern {
    Track kick;   // midiNote: 36
    Track clap;   // midiNote: 38
    Track hat;    // midiNote: 42
    Track perc;   // midiNote: 46
};
```

**Default Patterns:**
- Kick: Four-on-the-floor (steps 0, 4, 8, 12 active)
- Hats: Off-beats (steps 2, 6, 10, 14)

**Serialization:** Must integrate with `getStateInformation`/`setStateInformation` using `ValueTree` or custom XML.

---

### Phase 2: The Audio/MIDI Engine (The Backend)

**Objective:** Get the sequencer running in sync with the DAW transport.

**Key Implementation Points:**

1. **Transport Sync:**
   ```cpp
   auto* playHead = getPlayHead();
   if (playHead) {
       auto position = playHead->getPosition();
       // Get BPM, sample position, time signature
   }
   ```

2. **Step Calculation:**
   - Calculate which of the 16 steps corresponds to current sample time
   - Handle step boundary crossings within `processBlock`

3. **Sample-Accurate MIDI:**
   ```cpp
   // Calculate exact sample offset for MIDI event
   int sampleOffset = calculateSampleOffsetForStep(stepIndex, bpm, sampleRate);
   midiMessages.addEvent(MidiMessage::noteOn(1, midiNote, velocity), sampleOffset);
   ```

4. **Initial Output:** MIDI Effect only (route to external synth for testing)

---

### Phase 3: The Visual Grid (The UI Backbone)

**Objective:** Create the central 16×4 step grid.

**Component Structure:**
```cpp
class SequencerGridComponent : public juce::Component, public juce::Timer {
public:
    // Data binding to PluginProcessor
    void setProcessor(RhythmEngineAudioProcessor* p);
    
    // Painting
    void paint(juce::Graphics& g) override;
    
    // Interaction
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    
    // Playhead animation (60Hz)
    void timerCallback() override;
};
```

**Visual Style:**
- Background: Dark grey/black
- 4 horizontal lanes (Kick, Clap, Hat, Perc)
- 16 rectangles per lane
- Active steps: Neon Purple
- Inactive steps: Dark outline
- Moving playhead: Vertical line synced to host playback

---

### Phase 4: The "Macro Gears" (The Algorithm)

**Objective:** Implement Swing, Humanize, and Density controls.

| Macro | Parameter | Algorithm |
|-------|-----------|-----------|
| **Swing** | `swingAmount` (0.0–1.0) | Delay even-numbered 16th notes by calculated sample amount |
| **Humanize** | `humanize` (0.0–1.0) | Random velocity offset (±%) + micro-timing offset (±ms) |
| **Density** | `density` (0.0–1.0) | Programmatically activate ghost notes (low velocity) on Hat/Perc tracks |

**Audio Thread Considerations:**
- Pre-calculate random values or use fast PRNG
- No allocations in `processBlock`
- Use `std::atomic` for parameter access

---

### Phase 5: The "Cyberpunk" Aesthetic (UI Polish)

**Objective:** Apply the Neon Industrial visual style.

**1. The Gears (Macro Controls):**
```cpp
class RotaryGearComponent : public juce::Component {
    // Use juce::AffineTransform to rotate gear image
    // Draw using juce::Path for vector scalability
};
```

**2. The Neon Glow:**
- Use `juce::DropShadowEffect` for outer glow on active steps
- `juce::ColourGradient` for glass overlay effect
- Colors: Neon purple/pink (#FF00FF, #CC00CC)

**3. Layout:**
- Use `juce::FlexBox` for responsive arrangement
- Grid in center, Gears surrounding/overlaid
- Dashboard aesthetic matching reference image

---

## ⚠️ Technical Constraints

### Separation of Concerns
- Keep UI painting separate from audio logic
- Use `std::atomic` or APVTS Listener for GUI updates

### Performance
- `repaint()` only on Grid component, not entire window
- No allocations in audio thread
- Pre-calculate DSP values where possible

### Assets
- Generate gears using `juce::Path` (vector graphics) for perfect scaling
- Avoid PNG dependencies initially

---

## ✅ Design Decisions (Answered)

1. **Refactor, not new plugin**  
   This sequencer will **refactor the existing RhythmEngine** plugin, keeping the drum/bass synthesis and adding the step sequencer functionality on top.

2. **Audio output**  
   The plugin will output **audio** (not MIDI). The internal sequencer will trigger the existing Kick/Bass synthesis voices directly.

3. **Reference image:**  
   See below for the target Cyberpunk/Neon Industrial aesthetic.

---

## 🎨 Visual Reference

![Rhythm Engine Cyberpunk Reference](./rhythm_engine_reference.jpg)

### Key Design Elements from Reference:

| Element | Implementation Notes |
|---------|---------------------|
| **Central Glass Panel** | Embedded screen with rounded corners, subtle gradient transparency overlay |
| **Mechanical Gears** | 3 interlocking gears with metallic texture, rotating based on parameter values |
| **Neon Glow** | Purple/magenta (#CC00FF) and cyan (#00FFFF) outer glow on active elements |
| **Waveform Visualization** | Stylized audio waveform flowing through/around the gears |
| **Hardware Knobs** | Dark metallic rotary knobs with indicator lines around the edges |
| **Spectrum Analyzer** | Purple/cyan gradient bars in background (optional) |
| **Button Pads** | Cyan/magenta lit square pads (could be step buttons) |
| **Faders** | Vertical sliders with backlit tracks |
| **Color Palette** | Deep black (#0A0A0F), Neon Purple (#CC00FF), Cyan (#00FFFF), Metallic Grey (#3A3A45) |

---

## 📁 Proposed New Files

```
/plugins/RhythmEngine/Source/       (or new SequenceEngine folder)
├── RhythmStructures.h              # Step, Track, Pattern structs
├── SequencerEngine.h/cpp           # Transport sync, step scheduling
├── SequencerGridComponent.h/cpp    # 16×4 visual grid
├── RotaryGearComponent.h/cpp       # Macro gear controls
├── MacroProcessor.h/cpp            # Swing, Humanize, Density algorithms
└── CyberpunkLookAndFeel.h/cpp      # Custom neon styling
```

---

## 🔗 Related Documentation

- [Drum and bass VST implementation plan](./Drum%20and%20bass%20VST%20implementation%20plan%202d7874f10c7580918bb4e6928bf99299.md)
- [Melody Engine notes](./Melody%20Engine%202d8874f10c75806fb916da207e179f85.md)
- [Sample scrape and pull](./Sample%20scrape%20and%20pull%202d7874f10c7580bda5f6edb5e27643e5.md)
