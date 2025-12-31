# Ableton Live Optimization Plan

The RhythmEngine and MelodyEngine VST3 plugins crash when loaded in Ableton Live. Based on analysis of the codebase against the [Ableton optimization reference](file:///c:/personal_projects/house_prod_suite/prompts/Abelton%20Live%20Optimization%202da874f10c758099986ad61484c1ed10.md), this is due to threading and real-time safety violations.

## Issues Identified

| Issue | File | Severity |
|-------|------|----------|
| No pre-allocated buffers | Both `PluginProcessor.cpp` | High |
| GUI accesses pattern data directly | `SequencerGridComponent.cpp` L24, L99-106, L126 | **Critical** |
| GUI accesses phrase data directly | `MelodyCanvasComponent.cpp` L14, L102, L119-140 | **Critical** |
| 60Hz timer triggers repaint during audio | Both canvas components | Medium |
| Per-call APVTS lookup in processBlock | `MelodyEngine/PluginProcessor.cpp` L149-155 | Medium |

> [!CAUTION]
> The GUI components directly call `processor.getPattern()` and `processor.getPhrase()` from the message thread while the audio thread is reading/writing the same data. This is a **data race** and the most likely cause of crashes.

---

## Phase 1: Critical Fixes (Immediate Stability)

Goal: Eliminate data races and allocations in audio thread.

### RhythmEngine

#### [MODIFY] [PluginProcessor.h](file:///c:/personal_projects/house_prod_suite/plugins/RhythmEngine/Source/PluginProcessor.h)
- Add `juce::AudioBuffer<float> scratchBuffer` for temporary audio processing
- Add `std::atomic<int> currentStepAtomic{-1}` for lock-free playhead display
- Add `std::atomic<bool> patternDirty{false}` flag for pattern updates
- Add `RhythmEngine::Pattern patternSnapshot` for GUI-safe copy

#### [MODIFY] [PluginProcessor.cpp](file:///c:/personal_projects/house_prod_suite/plugins/RhythmEngine/Source/PluginProcessor.cpp)
- Pre-allocate `scratchBuffer` in `prepareToPlay()`
- Write `currentStepAtomic.store()` each step transition
- Add `copyPatternForGui()` method called from message thread

#### [MODIFY] [SequencerGridComponent.cpp](file:///c:/personal_projects/house_prod_suite/plugins/RhythmEngine/Source/SequencerGridComponent.cpp)
- Read from `patternSnapshot` instead of `processor.getPattern()` in `paint()`
- Read from `currentStepAtomic` for playhead position
- Use `processor.copyPatternForGui()` to get thread-safe copy when needed

---

### MelodyEngine

#### [MODIFY] [PluginProcessor.h](file:///c:/personal_projects/house_prod_suite/plugins/MelodyEngine/Source/PluginProcessor.h)
- Add atomic pointers for cached APVTS values (like RhythmEngine already has)
- Add `std::atomic<int> currentStepAtomic{-1}` for playhead
- Add `Melodic::Phrase phraseSnapshot` for GUI-safe copy
- Add `std::atomic<bool> phraseDirty{false}` flag

#### [MODIFY] [PluginProcessor.cpp](file:///c:/personal_projects/house_prod_suite/plugins/MelodyEngine/Source/PluginProcessor.cpp)
- Cache APVTS parameter pointers in constructor
- Use cached atomic pointers instead of `getRawParameterValue()` calls in `processBlock`
- Write `currentStepAtomic.store()` each step transition
- Add `copyPhraseForGui()` method

#### [MODIFY] [MelodyCanvasComponent.cpp](file:///c:/personal_projects/house_prod_suite/plugins/MelodyEngine/Source/MelodyCanvasComponent.cpp)
- Read from `phraseSnapshot` instead of `processor.getPhrase()` in `paint()`
- Read from `currentStepAtomic` for playhead display
- Use `processor.copyPhraseForGui()` in timer callback

---

## Phase 2: GUI Thread Safety (Polish)

Goal: Ensure all GUI↔Audio communication is lock-free.

### Both Plugins

- Replace direct pattern/phrase modification in mouseDown/mouseDrag with:
  - Set values on GUI-owned copy
  - Set `patternDirty`/`phraseDirty` flag
  - Audio thread polls flag and copies data when safe (between process blocks)
  
- Consider using `juce::AbstractFifo` for pattern change queue if complex edits are needed

---

## Phase 3: Parameter Smoothing (Performance)

Goal: Eliminate zipper noise and improve automation response.

```diff
// MelodyEngine processBlock - before
-float attack = apvts.getRawParameterValue("ATTACK")->load();
+float attack = attackParam->load();  // Pre-cached in constructor
```

- Add `juce::SmoothedValue<float>` for critical DSP parameters (cutoff, resonance)
- Process parameter changes once per block, not per sample

---

## Verification Plan

### Build Test
```powershell
cd c:\personal_projects\house_prod_suite
mkdir build_test
cd build_test
cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --config Release
```

### Manual DAW Test (User Required)
1. Open Ableton Live
2. Create empty project
3. Add MIDI track
4. Load **RhythmEngine** VST3 → verify no crash
5. Load **MelodyEngine** VST3 → verify no crash
6. Set buffer to 64 samples (Settings > Audio > Buffer Size)
7. Press Play with both plugins active
8. Click grid cells to trigger pattern changes while playing
9. Move sliders during playback → verify no glitches

> [!IMPORTANT]
> User must manually test in Ableton Live - we cannot automate DAW testing. Please confirm after each phase if crashes are resolved.
