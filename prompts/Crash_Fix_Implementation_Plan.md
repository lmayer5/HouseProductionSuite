# Crash Fix Implementation Plan

Based on the Perplexity crash analysis, this plan addresses 7 critical bugs causing Ableton Live crashes.

## Issues Summary

| Priority | Issue | Root Cause |
|----------|-------|------------|
| 🔴 Critical | Parameter pointer caching | Caching in constructor before `apvts` initialization |
| 🔴 Critical | Null pointer dereferences | No null checks when loading parameters in `processBlock` |
| 🔴 Critical | Missing function braces | Scope issues causing undefined behavior |
| 🟡 Secondary | Phrase data race | GUI modifies `phrase` while audio reads it |
| 🟡 Secondary | Synth uninitialized | `WavetableSynth` may have garbage values |
| 🟡 Secondary | Phrase uninitialized | `initializeDefaultPhrase()` may not zero all fields |
| 🟡 Secondary | Template type mismatch | Missing `<float>` in `processBlock` signature |

---

## Phase 1: Critical Initialization Fixes

### 1.1 Move Parameter Caching to prepareToPlay

Currently, parameter pointers are cached in the constructor body **before** `apvts` is fully constructed.

#### [MODIFY] `MelodyEngine/Source/PluginProcessor.cpp`

**Before (in constructor):**
```cpp
attackParam = apvts.getRawParameterValue("ATTACK");
// ... other params
```

**After (move to prepareToPlay):**
```cpp
void MelodyEngineAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
  // Cache parameter pointers (safe here - apvts fully initialized)
  if (!attackParam) attackParam = apvts.getRawParameterValue("ATTACK");
  if (!decayParam) decayParam = apvts.getRawParameterValue("DECAY");
  // ... other params
  
  // existing prepareToPlay code...
}
```

---

### 1.2 Add Null Guards in processBlock

#### [MODIFY] `MelodyEngine/Source/PluginProcessor.cpp`

Add null-safe parameter loading with sensible defaults:

```cpp
// In processBlock, replace direct loads with guarded versions
float attack = attackParam ? attackParam->load() : 0.01f;
float decay = decayParam ? decayParam->load() : 0.4f;
float morph = morphParam ? morphParam->load() : 0.0f;
float cutoff = cutoffParam ? cutoffParam->load() : 1000.0f;
float resonance = resonanceParam ? resonanceParam->load() : 0.0f;
float lfoRate = lfoRateParam ? lfoRateParam->load() : 1.0f;
float lfoDepth = lfoDepthParam ? lfoDepthParam->load() : 0.0f;
```

---

### 1.3 Verify Brace Matching

Scan both `PluginProcessor.cpp` files for unmatched `{ }`. Use IDE brace highlighting or a linter.

---

## Phase 2: Thread Safety Fixes

### 2.1 Use phraseSnapshot in Audio Thread

#### [MODIFY] `MelodyEngine/Source/PluginProcessor.cpp`

Ensure `processBlock` reads from `phraseSnapshot`, never from `phrase` directly:

```diff
- auto& event = phrase.events[stepToTrigger];
+ auto& event = phraseSnapshot.events[stepToTrigger];
```

Also call `copyPhraseForGui()` variant at start of processBlock:
```cpp
// At start of processBlock
if (phraseDirty.exchange(false)) {
  phraseSnapshot = phrase;
}
```

---

## Phase 3: Initialization Safety

### 3.1 Initialize WavetableSynth in prepareToPlay

#### [MODIFY] `MelodyEngine/Source/WavetableSynth.cpp`

Ensure `prepareToPlay` resets all oscillators and sets safe defaults:

```cpp
void WavetableSynth::prepareToPlay(double sr) {
  sampleRate = sr;
  envelope.setSampleRate(sr);
  
  // Reset oscillators
  osc1.reset();
  osc2.reset();
  lfo.reset();
  
  // Reset filter state
  z1 = 0.0f;
  
  // Initialize smoothers
  currentMorph.reset(sampleRate, 0.05);
  currentCutoff.reset(sampleRate, 0.05);
  lfoAmount.reset(sampleRate, 0.05);
  lfoRate.reset(sampleRate, 0.05);
  
  // Set initial values
  currentMorph.setCurrentAndTargetValue(0.0f);
  currentCutoff.setCurrentAndTargetValue(1000.0f);
  lfoAmount.setCurrentAndTargetValue(0.0f);
  lfoRate.setCurrentAndTargetValue(1.0f);
}
```

---

### 3.2 Zero-Initialize Phrase

#### [MODIFY] `MelodyEngine/Source/PluginProcessor.cpp`

In `initializeDefaultPhrase()`, explicitly zero all events:

```cpp
void MelodyEngineAudioProcessor::initializeDefaultPhrase() {
  // Zero all events first
  for (auto& event : phrase.events) {
    event.active = false;
    event.pitch = 60.0f;
    event.velocity = 1.0f;
    event.duration = 0.25f;
    event.probability = 1.0f;
  }
  
  phrase.rootNote = 60;
  phrase.scaleName = "Minor";
  
  // Copy to snapshot
  phraseSnapshot = phrase;
}
```

---

## Phase 4: Type Safety

### 4.1 Fix Template Type in Signature

#### [MODIFY] `MelodyEngine/Source/PluginProcessor.h`

Ensure signature uses explicit template:

```cpp
void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
```

---

## Verification Plan

### Standalone Testing
1. Build and run standalone app
2. Check console for assertion failures
3. Play MIDI notes - verify sound output

### Ableton Testing
1. Copy VST3 to system folder
2. Rescan plugins in Ableton
3. Add plugin to track - verify load without crash
4. Play transport - verify sequencer runs
5. Test at 64-sample buffer size

---

## Files to Modify

| File | Changes |
|------|---------|
| `MelodyEngine/PluginProcessor.cpp` | Move param caching, add null guards, use snapshot |
| `MelodyEngine/PluginProcessor.h` | Verify template type |
| `MelodyEngine/WavetableSynth.cpp` | Full reset in prepareToPlay |
| `RhythmEngine/PluginProcessor.cpp` | Same param caching + null guard fixes |
