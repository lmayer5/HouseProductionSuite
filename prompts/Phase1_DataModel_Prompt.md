# Phase 1: Data Model & State Management

## 🎯 Objective

Add step sequencer data structures to the existing RhythmEngine VST3 plugin. The sequencer will internally trigger the existing Kick/Bass synthesis (not MIDI output). This phase focuses purely on data structures and serialization - no audio processing changes yet.

---

## 📁 Project Context

**Location:** `c:\personal_projects\house_prod_suite`

**Build System:** CMake with JUCE 8.0.0 (fetched via FetchContent), C++20

**Existing Files to Modify:**
- `plugins/RhythmEngine/Source/PluginProcessor.h`
- `plugins/RhythmEngine/Source/PluginProcessor.cpp`

**New Files to Create:**
- `plugins/RhythmEngine/Source/RhythmStructures.h`

---

## 📋 Task 1: Create RhythmStructures.h

Create the header file `plugins/RhythmEngine/Source/RhythmStructures.h` with the following structures:

```cpp
#pragma once
#include <vector>
#include <array>

namespace RhythmEngine {

// Number of steps in the pattern (16th notes for a 4-bar pattern at 4 steps per beat)
constexpr int NUM_STEPS = 16;

// Number of drum tracks
constexpr int NUM_TRACKS = 4;

// Track indices for clarity
enum class TrackId : int {
    Kick = 0,
    Clap = 1,
    Hat  = 2,
    Perc = 3
};

/**
 * Represents a single step in the sequencer grid.
 */
struct Step {
    bool active = false;         // Is this step triggered?
    float velocity = 1.0f;       // 0.0 to 1.0, affects volume/intensity
    float probability = 1.0f;    // 0.0 to 1.0, chance of triggering (for humanization)
};

/**
 * Represents a single track (row) in the sequencer.
 */
struct Track {
    std::array<Step, NUM_STEPS> steps{};  // Fixed-size array of steps
    int midiNote = 36;                     // MIDI note to trigger (for reference)
    juce::String name;                     // Display name
};

/**
 * The complete drum pattern containing all tracks.
 */
struct Pattern {
    std::array<Track, NUM_TRACKS> tracks{};
    
    // Convenience accessors
    Track& kick()  { return tracks[static_cast<int>(TrackId::Kick)]; }
    Track& clap()  { return tracks[static_cast<int>(TrackId::Clap)]; }
    Track& hat()   { return tracks[static_cast<int>(TrackId::Hat)];  }
    Track& perc()  { return tracks[static_cast<int>(TrackId::Perc)]; }
    
    const Track& kick()  const { return tracks[static_cast<int>(TrackId::Kick)]; }
    const Track& clap()  const { return tracks[static_cast<int>(TrackId::Clap)]; }
    const Track& hat()   const { return tracks[static_cast<int>(TrackId::Hat)];  }
    const Track& perc()  const { return tracks[static_cast<int>(TrackId::Perc)]; }
};

} // namespace RhythmEngine
```

---

## 📋 Task 2: Modify PluginProcessor.h

Update `plugins/RhythmEngine/Source/PluginProcessor.h`:

### 2.1 Add Include

At the top, add:
```cpp
#include "RhythmStructures.h"
```

### 2.2 Add Pattern Member

In the private section of `RhythmEngineAudioProcessor`, add:
```cpp
// Sequencer Pattern Data
RhythmEngine::Pattern pattern;

// Helper methods for serialization
juce::ValueTree patternToValueTree() const;
void patternFromValueTree(const juce::ValueTree& tree);

// Initialize default patterns
void initializeDefaultPattern();
```

### 2.3 Add Public Accessor

In the public section, add a getter so the UI can access the pattern:
```cpp
// Pattern access for UI
RhythmEngine::Pattern& getPattern() { return pattern; }
const RhythmEngine::Pattern& getPattern() const { return pattern; }
```

---

## 📋 Task 3: Modify PluginProcessor.cpp

Update `plugins/RhythmEngine/Source/PluginProcessor.cpp`:

### 3.1 Implement initializeDefaultPattern()

Add this method to set up the default "Four-on-the-floor" pattern:

```cpp
void RhythmEngineAudioProcessor::initializeDefaultPattern()
{
    using namespace RhythmEngine;
    
    // Initialize track names and MIDI notes
    pattern.kick().name = "Kick";
    pattern.kick().midiNote = 36;  // C1
    
    pattern.clap().name = "Clap";
    pattern.clap().midiNote = 38;  // D1
    
    pattern.hat().name = "Hat";
    pattern.hat().midiNote = 42;   // F#1
    
    pattern.perc().name = "Perc";
    pattern.perc().midiNote = 46;  // A#1
    
    // Four-on-the-floor kick pattern (steps 0, 4, 8, 12)
    pattern.kick().steps[0].active = true;
    pattern.kick().steps[4].active = true;
    pattern.kick().steps[8].active = true;
    pattern.kick().steps[12].active = true;
    
    // Off-beat hi-hats (steps 2, 6, 10, 14)
    pattern.hat().steps[2].active = true;
    pattern.hat().steps[6].active = true;
    pattern.hat().steps[10].active = true;
    pattern.hat().steps[14].active = true;
    
    // Clap on beats 2 and 4 (steps 4 and 12)
    pattern.clap().steps[4].active = true;
    pattern.clap().steps[12].active = true;
}
```

### 3.2 Call initializeDefaultPattern() in Constructor

In the constructor of `RhythmEngineAudioProcessor`, after the existing initialization, add:
```cpp
initializeDefaultPattern();
```

### 3.3 Implement patternToValueTree()

This serializes the pattern to a ValueTree for saving with DAW sessions:

```cpp
juce::ValueTree RhythmEngineAudioProcessor::patternToValueTree() const
{
    using namespace RhythmEngine;
    
    juce::ValueTree patternTree("Pattern");
    
    for (int t = 0; t < NUM_TRACKS; ++t)
    {
        const auto& track = pattern.tracks[t];
        juce::ValueTree trackTree("Track");
        trackTree.setProperty("name", track.name, nullptr);
        trackTree.setProperty("midiNote", track.midiNote, nullptr);
        
        for (int s = 0; s < NUM_STEPS; ++s)
        {
            const auto& step = track.steps[s];
            juce::ValueTree stepTree("Step");
            stepTree.setProperty("index", s, nullptr);
            stepTree.setProperty("active", step.active, nullptr);
            stepTree.setProperty("velocity", step.velocity, nullptr);
            stepTree.setProperty("probability", step.probability, nullptr);
            trackTree.addChild(stepTree, -1, nullptr);
        }
        
        patternTree.addChild(trackTree, -1, nullptr);
    }
    
    return patternTree;
}
```

### 3.4 Implement patternFromValueTree()

This deserializes the pattern when loading a DAW session:

```cpp
void RhythmEngineAudioProcessor::patternFromValueTree(const juce::ValueTree& tree)
{
    using namespace RhythmEngine;
    
    if (!tree.isValid() || tree.getType().toString() != "Pattern")
        return;
    
    for (int t = 0; t < tree.getNumChildren() && t < NUM_TRACKS; ++t)
    {
        auto trackTree = tree.getChild(t);
        if (trackTree.getType().toString() != "Track")
            continue;
            
        auto& track = pattern.tracks[t];
        track.name = trackTree.getProperty("name", track.name);
        track.midiNote = trackTree.getProperty("midiNote", track.midiNote);
        
        for (int s = 0; s < trackTree.getNumChildren(); ++s)
        {
            auto stepTree = trackTree.getChild(s);
            if (stepTree.getType().toString() != "Step")
                continue;
                
            int stepIndex = stepTree.getProperty("index", -1);
            if (stepIndex >= 0 && stepIndex < NUM_STEPS)
            {
                auto& step = track.steps[stepIndex];
                step.active = stepTree.getProperty("active", false);
                step.velocity = stepTree.getProperty("velocity", 1.0f);
                step.probability = stepTree.getProperty("probability", 1.0f);
            }
        }
    }
}
```

### 3.5 Update getStateInformation()

Modify the existing `getStateInformation()` method to include the pattern data:

```cpp
void RhythmEngineAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // Create root state
    auto state = apvts.copyState();
    
    // Add pattern data as a child
    auto patternTree = patternToValueTree();
    state.addChild(patternTree, -1, nullptr);
    
    // Serialize to XML
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}
```

### 3.6 Update setStateInformation()

Modify the existing `setStateInformation()` method to restore the pattern data:

```cpp
void RhythmEngineAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    
    if (xmlState.get() != nullptr)
    {
        if (xmlState->hasTagName(apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml(*xmlState);
            
            // Restore APVTS parameters
            apvts.replaceState(tree);
            
            // Restore pattern data
            auto patternTree = tree.getChildWithName("Pattern");
            if (patternTree.isValid())
            {
                patternFromValueTree(patternTree);
            }
        }
    }
}
```

---

## ✅ Acceptance Criteria

1. **Compiles cleanly** with no warnings in C++20 mode
2. **RhythmStructures.h** exists with `Step`, `Track`, and `Pattern` structs
3. **Default pattern** initializes with:
   - Kick on steps 0, 4, 8, 12 (four-on-the-floor)
   - Hi-hats on steps 2, 6, 10, 14 (off-beats)
   - Clap on steps 4, 12 (beats 2 and 4)
4. **State serialization** works - pattern saves/loads with DAW session
5. **No changes** to `processBlock()` yet - that's Phase 2
6. **Existing functionality preserved** - current MIDI-triggered synthesis still works

---

## ⚠️ Important Notes

- Use `std::array` instead of `std::vector` for fixed-size data (avoids heap allocations)
- The `RhythmEngine` namespace prevents name collisions
- The `TrackId` enum provides type-safe track indexing
- The pattern data is stored separately from APVTS but serialized together
- Do NOT modify `processBlock()` in this phase

---

## 🔗 Reference Files

Before making changes, review these existing files:
- `plugins/RhythmEngine/Source/PluginProcessor.h` - current header
- `plugins/RhythmEngine/Source/PluginProcessor.cpp` - current implementation
- `plugins/RhythmEngine/CMakeLists.txt` - may need to add new source file

---

## 🧪 Testing

After implementation, build the project:
```cmd
cd c:\personal_projects\house_prod_suite
build_plugins.bat
```

The build should complete with 0 errors. The plugin should still function identically to before (MIDI triggering still works). The pattern data is internal only until Phase 2 connects it to the audio engine.
