/*
  ==============================================================================

    PluginProcessor.cpp
    Created: 29 Dec 2025
    Author:  DJstih

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
RhythmEngineAudioProcessor::RhythmEngineAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties().withOutput(
          "Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
#endif
{
  initializeDefaultPattern();
}

RhythmEngineAudioProcessor::~RhythmEngineAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout
RhythmEngineAudioProcessor::createParameterLayout() {
  juce::AudioProcessorValueTreeState::ParameterLayout layout;
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "KICK_FREQ", "Kick Freq", 40.0f, 150.0f, 60.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "KICK_DECAY", "Kick Decay", 0.1f, 1.0f, 0.4f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "BASS_CUTOFF", "Bass Cutoff", 20.0f, 2000.0f, 200.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "BASS_DRIVE", "Bass Drive", 0.0f, 1.0f, 0.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "SIDECHAIN_AMT", "Sidechain Amt", 0.0f, 1.0f, 0.5f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "BASS_ATTACK", "Bass Attack", 0.001f, 0.5f, 0.01f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "BASS_DECAY", "Bass Decay", 0.1f, 2.0f, 0.4f));

  // TE-style Punch-In FX (momentary, 0 = off, 1 = full)
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "FX_STUTTER", "FX Stutter", 0.0f, 1.0f, 0.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>("FX_SWEEP", "FX Sweep",
                                                         0.0f, 1.0f, 0.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "FX_BITCRUSH", "FX Bitcrush", 0.0f, 1.0f, 0.0f));

  return layout;
}

const juce::String RhythmEngineAudioProcessor::getName() const {
  return JucePlugin_Name;
}
bool RhythmEngineAudioProcessor::acceptsMidi() const { return true; }
bool RhythmEngineAudioProcessor::producesMidi() const { return false; }
bool RhythmEngineAudioProcessor::isMidiEffect() const { return false; }
double RhythmEngineAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int RhythmEngineAudioProcessor::getNumPrograms() { return 1; }
int RhythmEngineAudioProcessor::getCurrentProgram() { return 0; }
void RhythmEngineAudioProcessor::setCurrentProgram(int index) {}
const juce::String RhythmEngineAudioProcessor::getProgramName(int index) {
  return {};
}
void RhythmEngineAudioProcessor::changeProgramName(
    int index, const juce::String &newName) {}

void RhythmEngineAudioProcessor::prepareToPlay(double sampleRate,
                                               int samplesPerBlock) {
  kickPhasor.reset();
  bassPhasor.reset();

  kickEnv.setSampleRate(sampleRate);
  bassEnv.setSampleRate(sampleRate);
  clapEnv.setSampleRate(sampleRate);
  hatEnv.setSampleRate(sampleRate);

  // Set default drum envelopes
  clapEnv.setParameters(0.001f, 0.2f);
  hatEnv.setParameters(0.001f, 0.05f);

  // Cache atomic parameter pointers (Safe here)
  if (!kickFreqParam)
    kickFreqParam = apvts.getRawParameterValue("KICK_FREQ");
  if (!kickDecayParam)
    kickDecayParam = apvts.getRawParameterValue("KICK_DECAY");
  if (!bassCutoffParam)
    bassCutoffParam = apvts.getRawParameterValue("BASS_CUTOFF");
  if (!bassDriveParam)
    bassDriveParam = apvts.getRawParameterValue("BASS_DRIVE");
  if (!sidechainAmtParam)
    sidechainAmtParam = apvts.getRawParameterValue("SIDECHAIN_AMT");
  if (!bassAttackParam)
    bassAttackParam = apvts.getRawParameterValue("BASS_ATTACK");
  if (!bassDecayParam)
    bassDecayParam = apvts.getRawParameterValue("BASS_DECAY");

  // Pre-allocate scratch buffer to avoid re-allocation in processBlock
  scratchBuffer.setSize(2, samplesPerBlock);

  // Initialize parameter smoothers (50ms ramp)
  smoothKickFreq.reset(sampleRate, 0.05);
  smoothBassCutoff.reset(sampleRate, 0.05);
  smoothBassDrive.reset(sampleRate, 0.05);
  smoothSidechainAmt.reset(sampleRate, 0.05);

  // Prepare Punch-In FX
  stutterFX.prepare(sampleRate, samplesPerBlock);
  sweepFilterFX.prepare(sampleRate);

  // Cache FX parameter pointers
  if (!fxStutterParam)
    fxStutterParam = apvts.getRawParameterValue("FX_STUTTER");
  if (!fxSweepParam)
    fxSweepParam = apvts.getRawParameterValue("FX_SWEEP");
  if (!fxBitcrushParam)
    fxBitcrushParam = apvts.getRawParameterValue("FX_BITCRUSH");
}

void RhythmEngineAudioProcessor::releaseResources() {}

bool RhythmEngineAudioProcessor::isBusesLayoutSupported(
    const BusesLayout &layouts) const {
  if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
      layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
    return false;
  return true;
}

void RhythmEngineAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                              juce::MidiBuffer &midiMessages) {
  juce::ScopedNoDenormals noDenormals;
  auto totalNumInputChannels = getTotalNumInputChannels();
  auto totalNumOutputChannels = getTotalNumOutputChannels();

  for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    buffer.clear(i, 0, buffer.getNumSamples());

  // Process waiting commands from GUI
  int numToDo = commandFifo.getNumReady();
  if (numToDo > 0) {
    auto reader = commandFifo.read(numToDo);
    auto processCommands = [this](int start, int count) {
      for (int i = 0; i < count; ++i) {
        auto &cmd = commandBuffer[start + i];
        if (cmd.trackIdx >= 0 && cmd.trackIdx < RhythmEngine::NUM_TRACKS &&
            cmd.stepIdx >= 0 && cmd.stepIdx < RhythmEngine::NUM_STEPS) {

          if (cmd.type == RhythmCommand::ToggleStep) {
            pattern.tracks[cmd.trackIdx].steps[cmd.stepIdx].active =
                (cmd.value > 0.5f);
            patternDirty.store(true);
          } else if (cmd.type == RhythmCommand::UpdateVelocity) {
            pattern.tracks[cmd.trackIdx].steps[cmd.stepIdx].velocity =
                cmd.value;
            patternDirty.store(true);
          } else if (cmd.type == RhythmCommand::SetModifier) {
            pattern.tracks[cmd.trackIdx].steps[cmd.stepIdx].modifier =
                cmd.modifierValue;
            patternDirty.store(true);
          }
        }
      }
    };

    if (reader.blockSize1 > 0)
      processCommands(reader.startIndex1, reader.blockSize1);
    if (reader.blockSize2 > 0)
      processCommands(reader.startIndex2, reader.blockSize2);
  }

  // Sequencer Logic
  double sampleRate = getSampleRate();
  int numSamples = buffer.getNumSamples();

  // 1. Get Transport/Sync Info
  auto *playHead = getPlayHead();
  bool isPlaying = false;
  double blockStartSampleTime = 0.0;
  int currentStepForGui = -1;

  if (playHead != nullptr) {
    if (auto position = playHead->getPosition()) {
      isPlaying = position->getIsPlaying();
      currentBpm = position->getBpm().orFallback(120.0);
      blockStartSampleTime =
          static_cast<double>(position->getTimeInSamples().orFallback(0));
    }
  }

  // 2. MIDI Loop (External MIDI still works)
  for (const auto metadata : midiMessages) {
    auto msg = metadata.getMessage();
    if (msg.isNoteOn()) {
      int note = msg.getNoteNumber();
      if (note == 36) { // Kick
        kickEnv.trigger();
        kickPhasor.reset();
      }
      if (note >= 48) { // Bass
        currentBassFreq = 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
        bassEnv.trigger();
      }
    }
  }

  // 3. Sequencer Triggering (with TE-style Step Components)
  if (isPlaying) {
    double quarterNoteLengthSamples = (60.0 / currentBpm) * sampleRate;
    double samplesPerStep = quarterNoteLengthSamples / 4.0; // 16th notes

    for (int s = 0; s < numSamples; ++s) {
      double currentSampleTime = blockStartSampleTime + s;

      if (lastProcessedSampleTime >= 0.0) {
        double lastStepIdx =
            std::floor(lastProcessedSampleTime / samplesPerStep);
        double currentStepIdx = std::floor(currentSampleTime / samplesPerStep);

        // Detect new step boundary
        if (currentStepIdx > lastStepIdx) {
          int stepToTrigger =
              static_cast<int>(currentStepIdx) % RhythmEngine::NUM_STEPS;

          // Loop detection (for TE-style logic gates)
          if (stepToTrigger == 0 &&
              lastStepForLoopDetection == RhythmEngine::NUM_STEPS - 1) {
            currentLoopCount++;
          }
          lastStepForLoopDetection = stepToTrigger;

          // Update atomic for GUI
          currentStepAtomic.store(stepToTrigger);

          // Reset ratchet counters at step start
          for (auto &rc : ratchetCounters)
            rc = 0;

          // Lambda to check if step should trigger based on logic gates
          auto shouldTriggerLogicGate =
              [&](RhythmEngine::StepModifier mod) -> bool {
            switch (mod) {
            case RhythmEngine::StepModifier::SkipCycle:
              return (currentLoopCount % 2) == 0;
            case RhythmEngine::StepModifier::OnlyFirstCycle:
              return currentLoopCount == 0;
            default:
              return true; // None, Ratchet, Glide all pass logic check
            }
          };

          // Lambda for triggering with probability and modifiers
          auto triggerWithMods = [&](int trackId, auto &env,
                                     bool resetPhasor = false,
                                     bool isBass = false) {
            auto &step = pattern.tracks[trackId].steps[stepToTrigger];
            if (!step.active)
              return;
            if (!shouldTriggerLogicGate(step.modifier))
              return;
            if (random.nextFloat() > step.probability)
              return;

            // Trigger the voice
            env.trigger();
            if (resetPhasor)
              kickPhasor.reset();
            if (isBass) {
              int note = pattern.tracks[trackId].midiNote;
              currentBassFreq = 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
            }
          };

          triggerWithMods(static_cast<int>(RhythmEngine::TrackId::Kick),
                          kickEnv, true);
          triggerWithMods(static_cast<int>(RhythmEngine::TrackId::Bass),
                          bassEnv, false, true);
          triggerWithMods(static_cast<int>(RhythmEngine::TrackId::Clap),
                          clapEnv);
          triggerWithMods(static_cast<int>(RhythmEngine::TrackId::Hat), hatEnv);
        }

        // Ratchet sub-step triggers (within step duration)
        double positionInStep = std::fmod(currentSampleTime, samplesPerStep);
        int currentStep =
            static_cast<int>(std::floor(currentSampleTime / samplesPerStep)) %
            RhythmEngine::NUM_STEPS;

        // Check each track for ratchet modifiers
        auto checkRatchet = [&](int trackId, auto &env,
                                bool resetPhasor = false, bool isBass = false) {
          auto &step = pattern.tracks[trackId].steps[currentStep];
          if (!step.active)
            return;

          int divisions = 0;
          if (step.modifier == RhythmEngine::StepModifier::Ratchet2)
            divisions = 2;
          else if (step.modifier == RhythmEngine::StepModifier::Ratchet4)
            divisions = 4;

          if (divisions > 0) {
            double subStepLength = samplesPerStep / divisions;
            int expectedSubStep =
                static_cast<int>(positionInStep / subStepLength);

            // Trigger if we've crossed into a new sub-step (skip first, already
            // triggered)
            if (expectedSubStep > ratchetCounters[trackId] &&
                expectedSubStep < divisions) {
              ratchetCounters[trackId] = expectedSubStep;
              env.trigger();
              if (resetPhasor)
                kickPhasor.reset();
              if (isBass) {
                int note = pattern.tracks[trackId].midiNote;
                currentBassFreq = 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
              }
            }
          }
        };

        checkRatchet(static_cast<int>(RhythmEngine::TrackId::Kick), kickEnv,
                     true);
        checkRatchet(static_cast<int>(RhythmEngine::TrackId::Bass), bassEnv,
                     false, true);
        checkRatchet(static_cast<int>(RhythmEngine::TrackId::Clap), clapEnv);
        checkRatchet(static_cast<int>(RhythmEngine::TrackId::Hat), hatEnv);
      }
      lastProcessedSampleTime = currentSampleTime;
    }
  } else {
    lastProcessedSampleTime = -1.0;
    currentStepAtomic.store(-1);
    currentLoopCount = 0;
    lastStepForLoopDetection = -1;
  }

  // 4. Update DSP Parameters (Set Targets with Null Guards)
  float kickDecay = kickDecayParam ? kickDecayParam->load() : 0.4f;
  float bassAttack = bassAttackParam ? bassAttackParam->load() : 0.01f;
  float bassDecay = bassDecayParam ? bassDecayParam->load() : 0.4f;

  kickEnv.setParameters(0.005f, kickDecay);
  bassEnv.setParameters(bassAttack, bassDecay);

  smoothKickFreq.setTargetValue(kickFreqParam ? kickFreqParam->load() : 60.0f);
  smoothBassCutoff.setTargetValue(bassCutoffParam ? bassCutoffParam->load()
                                                  : 200.0f);
  smoothBassDrive.setTargetValue(bassDriveParam ? bassDriveParam->load()
                                                : 0.0f);
  smoothSidechainAmt.setTargetValue(
      sidechainAmtParam ? sidechainAmtParam->load() : 0.5f);

  auto *leftChannel = buffer.getWritePointer(0);
  auto *rightChannel =
      (totalNumOutputChannels > 1) ? buffer.getWritePointer(1) : nullptr;

  // 5. Audio Loop (Synthesis)
  for (int s = 0; s < numSamples; ++s) {
    // Get smoothed values for this sample
    float kFreq = smoothKickFreq.getNextValue();
    float bCutoff = smoothBassCutoff.getNextValue();
    float bDrive = smoothBassDrive.getNextValue();
    float scAmt = smoothSidechainAmt.getNextValue();

    // KICK
    float kEnv = kickEnv.getNextSample();
    float kPitchObj = kFreq * (1.0f + 3.0f * kEnv);
    float kPhase = kickPhasor.process(kPitchObj, sampleRate);
    float kickSample =
        std::sin(kPhase * 2.0f * std::numbers::pi_v<float>) * kEnv * kickGain;

    // BASS
    float bPhase = bassPhasor.process(currentBassFreq, sampleRate);
    float bassRaw = (bPhase * 2.0f) - 1.0f;
    float bEnv = bassEnv.getNextSample();

    float bAlpha = 2.0f * std::numbers::pi_v<float> * bCutoff /
                   static_cast<float>(sampleRate);
    bassFilterState +=
        std::clamp(bAlpha, 0.0f, 1.0f) * (bassRaw - bassFilterState);
    float bassSample = bassFilterState * bEnv * bassGain;

    float driveGain = 1.0f + (bDrive * 9.0f);
    bassSample = djstih::ClipFunctions::softClip(bassSample * driveGain);

    // Sidechain
    float ducking = 1.0f - (kEnv * scAmt);
    bassSample *= std::max(0.0f, ducking);

    // CLAP (White noise + LP Filter)
    float whiteNoise = (random.nextFloat() * 2.0f) - 1.0f;
    float cEnv = clapEnv.getNextSample();
    clapFilterState +=
        0.2f * (whiteNoise - clapFilterState); // Static LP for clap
    float clapSample = clapFilterState * cEnv * clapGain;

    // HAT (High-passed noise)
    float hEnv = hatEnv.getNextSample();
    float hNoise = (random.nextFloat() * 2.0f) - 1.0f;
    // 1-pole HP: out = in - lp
    hatFilterState += 0.8f * (hNoise - hatFilterState);
    float hatSample = (hNoise - hatFilterState) * hEnv * hatGain;

    float out = kickSample + bassSample + clapSample + hatSample;
    leftChannel[s] = out;
    if (rightChannel)
      rightChannel[s] = out;
  }

  // ========== TE-style Punch-In FX (post-render) ==========
  float stutterAmt = fxStutterParam ? fxStutterParam->load() : 0.0f;
  float sweepAmt = fxSweepParam ? fxSweepParam->load() : 0.0f;
  float bitcrushAmt = fxBitcrushParam ? fxBitcrushParam->load() : 0.0f;

  // Stutter FX: activate on rising edge, process while active
  if (stutterAmt > 0.5f) {
    if (!stutterFX.isActive()) {
      stutterFX.activate(djstih::StutterFX::Division::Sixteenth, currentBpm);
    }
    stutterFX.process(buffer);
  } else {
    stutterFX.deactivate();
  }

  // Sweep Filter FX: HP sweep based on amount
  if (sweepAmt > 0.01f) {
    sweepFilterFX.setMode(djstih::SweepFilterFX::Mode::HighPass);
    sweepFilterFX.process(buffer, sweepAmt);
  } else {
    sweepFilterFX.setMode(djstih::SweepFilterFX::Mode::Off);
  }

  // Bitcrush FX: reduce bit depth when active
  if (bitcrushAmt > 0.5f) {
    bitcrushFX.setActive(true);
    bitcrushFX.setBitDepth(4); // Lo-fi 4-bit crunch
    bitcrushFX.setDownsample(4);
    bitcrushFX.process(buffer);
  } else {
    bitcrushFX.setActive(false);
  }

  updateSnapshotFromAudio();
}

bool RhythmEngineAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor *RhythmEngineAudioProcessor::createEditor() {
  return new RhythmEngineAudioProcessorEditor(*this);
}

void RhythmEngineAudioProcessor::initializeDefaultPattern() {
  using namespace RhythmEngine;

  // Zero-initialize pattern first
  for (int t = 0; t < NUM_TRACKS; ++t) {
    pattern.tracks[t].name = "";
    pattern.tracks[t].midiNote = 0;
    for (int s = 0; s < NUM_STEPS; ++s) {
      pattern.tracks[t].steps[s].active = false;
      pattern.tracks[t].steps[s].velocity = 1.0f;
      pattern.tracks[t].steps[s].probability = 1.0f;
      pattern.tracks[t].steps[s].modifier = StepModifier::None;
    }
  }

  // Initialize track names and MIDI notes
  pattern.kick().name = "Kick";
  pattern.kick().midiNote = 36; // C1

  pattern.clap().name = "Clap";
  pattern.clap().midiNote = 38; // D1

  pattern.hat().name = "Hat";
  pattern.hat().midiNote = 42; // F#1

  pattern.bass().name = "Bass";
  pattern.bass().midiNote = 36; // C1 (matches default synth freq)

  // Four-on-the-floor kick pattern (steps 0, 4, 8, 12)
  pattern.kick().steps[0].active = true;
  pattern.kick().steps[4].active = true;
  pattern.kick().steps[8].active = true;
  pattern.kick().steps[12].active = true;

  // Off-beat bass (steps 2, 6, 10, 14 - syncopated with kick)
  pattern.bass().steps[2].active = true;
  pattern.bass().steps[6].active = true;
  pattern.bass().steps[10].active = true;
  pattern.bass().steps[14].active = true;

  // Off-beat hi-hats (every 8th note: 2, 6, 10, 14)
  pattern.hat().steps[2].active = true;
  pattern.hat().steps[6].active = true;
  pattern.hat().steps[10].active = true;
  pattern.hat().steps[14].active = true;

  // Clap on beats 2 and 4 (steps 4 and 12)
  pattern.clap().steps[4].active = true;
  pattern.clap().steps[12].active = true;
}

juce::ValueTree RhythmEngineAudioProcessor::patternToValueTree() const {
  using namespace RhythmEngine;

  juce::ValueTree patternTree("Pattern");

  for (int t = 0; t < NUM_TRACKS; ++t) {
    const auto &track = pattern.tracks[t];
    juce::ValueTree trackTree("Track");
    trackTree.setProperty("name", track.name, nullptr);
    trackTree.setProperty("midiNote", track.midiNote, nullptr);

    for (int s = 0; s < NUM_STEPS; ++s) {
      const auto &step = track.steps[s];
      juce::ValueTree stepTree("Step");
      stepTree.setProperty("index", s, nullptr);
      stepTree.setProperty("active", step.active, nullptr);
      stepTree.setProperty("velocity", step.velocity, nullptr);
      stepTree.setProperty("probability", step.probability, nullptr);
      stepTree.setProperty("modifier", static_cast<int>(step.modifier),
                           nullptr);
      trackTree.addChild(stepTree, -1, nullptr);
    }

    patternTree.addChild(trackTree, -1, nullptr);
  }

  return patternTree;
}

void RhythmEngineAudioProcessor::patternFromValueTree(
    const juce::ValueTree &tree) {
  using namespace RhythmEngine;

  if (!tree.isValid() || tree.getType().toString() != "Pattern")
    return;

  for (int t = 0; t < tree.getNumChildren() && t < NUM_TRACKS; ++t) {
    auto trackTree = tree.getChild(t);
    if (trackTree.getType().toString() != "Track")
      continue;

    auto &track = pattern.tracks[t];
    track.name = trackTree.getProperty("name", track.name).toString();
    track.midiNote = trackTree.getProperty("midiNote", track.midiNote);

    for (int s = 0; s < trackTree.getNumChildren(); ++s) {
      auto stepTree = trackTree.getChild(s);
      if (stepTree.getType().toString() != "Step")
        continue;

      int stepIndex = stepTree.getProperty("index", -1);
      if (stepIndex >= 0 && stepIndex < NUM_STEPS) {
        auto &step = track.steps[stepIndex];
        step.active = stepTree.getProperty("active", false);
        step.velocity = stepTree.getProperty("velocity", 1.0f);
        step.probability = stepTree.getProperty("probability", 1.0f);
        step.modifier = static_cast<StepModifier>(
            static_cast<int>(stepTree.getProperty("modifier", 0)));
      }
    }
  }
}

void RhythmEngineAudioProcessor::getStateInformation(
    juce::MemoryBlock &destData) {
  auto state = apvts.copyState();

  auto patternTree = patternToValueTree();
  state.addChild(patternTree, -1, nullptr);

  std::unique_ptr<juce::XmlElement> xml(state.createXml());
  copyXmlToBinary(*xml, destData);
}

void RhythmEngineAudioProcessor::setStateInformation(const void *data,
                                                     int sizeInBytes) {
  std::unique_ptr<juce::XmlElement> xmlState(
      getXmlFromBinary(data, sizeInBytes));

  if (xmlState.get() != nullptr) {
    if (xmlState->hasTagName(apvts.state.getType())) {
      auto tree = juce::ValueTree::fromXml(*xmlState);
      apvts.replaceState(tree);

      auto patternTree = tree.getChildWithName("Pattern");
      if (patternTree.isValid()) {
        patternFromValueTree(patternTree);
      }
    }
  }
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new RhythmEngineAudioProcessor();
}
