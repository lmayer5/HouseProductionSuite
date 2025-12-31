# Melodic Engine Phased dev plan from gemini

This is the logical next step. We are effectively building the "Brain" to compliment the Rhythm Engine’s "Heart."

Here is the phased development plan for the **Melodic Engine**. It adheres to the same architectural philosophy (Data $\rightarrow$ DSP $\rightarrow$ UI) but focuses on the specific needs of melodic generation: scales, arpeggios, and wavetable morphing.

### **Project Overview: "The Melodic Engine"**

- **Goal:** A House Music Melodic Sequencer & Wavetable Synth.
- **Aesthetic:** Cyberpunk/Neon Industrial (Matching the Rhythm Engine).
- **Core UX:** A "Melody Canvas" for painting phrases and a central "Timbre Morph Gear" that evolves the sound.

---

### **Phase 1: The Data Model (Musical Theory)**

**Objective:** Define the structures that enforce musicality (Scales, Keys, Phrases). Unlike drums, this engine requires pitch awareness.

**Agent Prompt:**

> "I need to implement the data structures for the 'Melodic Engine' plugin.
> 
> 
> **Requirements:**
> 
> 1. **Core Structs:** In `MelodyStructures.h`, define:
>     - `NoteEvent`: float pitch (MIDI note), float velocity, float duration, float probability.
>     - `Phrase`: A vector of `NoteEvent`, plus `int rootNote` and `std::string scale` (e.g., 'Minor', 'Dorian').
> 2. **Scale Logic:** Create a helper class `ScaleQuantizer`.
>     - Method: `float getNearestScaleNote(float inputNote, int root, std::string scaleType)`.
>     - Implementation: Map input chromatic notes to the nearest allowed interval in the selected scale (e.g., C Minor).
> 3. **State Management:**
>     - Add a `Phrase` object to the `PluginProcessor`.
>     - Implement `getStateInformation` to serialize the note data to XML/ValueTree.
> 4. **Defaults:** Initialize the engine with a C Minor scale and a basic 1/8th note arpeggio pattern over 4 bars."

---

### **Phase 2: The DSP Engine (Wavetable Synthesis)**

**Objective:** Build the sound generator. The research points to "Wavetable Oscillators" and "Timbre Morphing".

**Agent Prompt:**

> "Implement the audio generation in PluginProcessor using JUCE DSP.
> 
> 
> **Requirements:**
> 
> 1. **Oscillators:**
>     - Use `juce::dsp::Oscillator`.
>     - Create a `WavetableSynth` class that manages 2 oscillators.
>     - Implement a 'Morph' parameter (0.0 - 1.0) that interpolates between two wavetables (e.g., Sine to Saw, or Pulse to Square).
> 2. **The Sequencer (MIDI Generation):**
>     - In `processBlock`, use the `getPlayHead()` position.
>     - Look up the current position in the `Phrase` data.
>     - If a `NoteEvent` triggers, calculate the frequency using the `ScaleQuantizer` and trigger the oscillator.
> 3. **Modulation:**
>     - Implement a simple LFO targeting the Filter Cutoff.
>     - Expose a 'Pulse' macro param that controls the LFO depth (sidechain simulation)."

---

### **Phase 3: The UI Backbone (Melody Canvas)**

**Objective:** Replace the piano roll with a "Gesture Sequencer" grid.

**Agent Prompt:**

> "Create the MelodyCanvas component.
> 
> 
> **Visual Style:**
> 
> - Background: Dark, industrial grid.
> - Lanes: Horizontal representation of time, vertical representation of Pitch.
> 
> **Functionality:**
> 
> 1. **Paint Method:**
>     - Draw `NoteEvents` as horizontal bars.
>     - **Height:** The height/thickness of the bar should visualize `Velocity`.
>     - **Color:** Use the neon purple palette. Notes in the root key (Tonic) should be brighter than others.
> 2. **Auto-Accompaniment Grid:**
>     - Overlay faint vertical blocks representing the underlying chord progression (i-iv-v-i) as visual guides.
> 3. **Interaction:**
>     - `mouseDown`: Adds a note.
>     - `mouseDrag`: Paints a stream of notes. Snaps pitch to the Y-axis (Scale Degrees) and time to the X-axis (1/16th grid)."

---

### **Phase 4: The Macros (XY Pad & Timbre Gear)**

**Objective:** Abstract complex parameters into "Morph" controls.

**Agent Prompt:**

> "Implement the high-level macro controls found in the research.
> 
> 
> **1. The Style Pad (XY Control):**
> 
> - Create a component `StylePad`.
> - **X-Axis:** Maps to 'Density' (Sparse Plucks -> Busy Arps).
> - **Y-Axis:** Maps to 'Range' (Static Chords -> Ascending Runs).
> - **Logic:** When the puck moves, algorithmically regenerate the `Phrase` data based on these constraints.
> 
> **2. The Timbre Morph Wheel:**
> 
> - Reuse the 'Gear' concept from the Rhythm Engine.
> - **Function:** A large central rotary control.
> - **Target:** Controls the Wavetable Interpolation (Phase 2) and Filter Cutoff simultaneously.
> - **Visual:** As the user rotates the gear, the 'spectral preview' in the center should shift color/shape."

---

### **Phase 5: Visual Integration (The Cyberpunk Suite)**

**Objective:** Unify the design with the Rhythm Engine using the reference image.

**Agent Prompt:**

> "Polish the UI to match the 'Rhythm Engine' aesthetic (Reference: rhythm_engine.jpg).
> 
> 
> **1. Unified Look:**
> 
> - The `MelodyCanvas` should sit inside a 'glass' panel similar to the rhythm grid.
> - Use the same 'Neon Purple' (`0xffbc13fe`) and 'Cyan' (`0xff00f3ff`) color palette.
> 
> **2. The Harmony Lock:**
> 
> - Create a 'Dropdown' or 'Dial' that looks like a mechanical selector switch.
> - Allows the user to select the Key (e.g., C Min, F Maj).
> - When changed, the `MelodyCanvas` grid lines should visually shift to highlight the new safe notes.
> 
> **3. Modulation Cables (Optional):**
> 
> - Draw subtle animated lines connecting the 'Pulse' macro to the filter section, pulsing with the beat to visualize the sidechain effect."

---

### **Technical Tips for the Agent**

- **Scale Quantization:** Remind the agent that when "painting" notes on the canvas, the Y-position shouldn't map to *Chromatic* MIDI (0-127) but to *Scale Degrees* (1-7). This ensures the user literally cannot hit a wrong note.
- **Wavetables:** For Phase 2, suggest using a pre-calculated lookup table (LUT) for the sine/saw waves to keep CPU usage low, as `std::sin` in real-time can be expensive if overused.