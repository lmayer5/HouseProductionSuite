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
MelodyEngineAudioProcessor::MelodyEngineAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(
          BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
              ),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
#endif
{
  initializeDefaultPhrase();
}

MelodyEngineAudioProcessor::~MelodyEngineAudioProcessor() {}

//==============================================================================
const juce::String MelodyEngineAudioProcessor::getName() const {
  return JucePlugin_Name;
}

bool MelodyEngineAudioProcessor::acceptsMidi() const {
#if JucePlugin_WantsMidiInput
  return true;
#else
  return false;
#endif
}

bool MelodyEngineAudioProcessor::producesMidi() const {
#if JucePlugin_ProducesMidiOutput
  return true;
#else
  return false;
#endif
}

bool MelodyEngineAudioProcessor::isMidiEffect() const {
#if JucePlugin_IsMidiEffect
  return true;
#else
  return false;
#endif
}

double MelodyEngineAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int MelodyEngineAudioProcessor::getNumPrograms() {
  return 1; // NB: some hosts don't cope very well if you tell them there are 0
            // programs, so this should be at least 1, even if you're not really
            // implementing programs.
}

int MelodyEngineAudioProcessor::getCurrentProgram() { return 0; }

void MelodyEngineAudioProcessor::setCurrentProgram(int index) {}

const juce::String MelodyEngineAudioProcessor::getProgramName(int index) {
  return {};
}

void MelodyEngineAudioProcessor::changeProgramName(
    int index, const juce::String &newName) {}

//==============================================================================
//==============================================================================
void MelodyEngineAudioProcessor::prepareToPlay(double sampleRate,
                                               int samplesPerBlock) {
  synth.prepareToPlay(sampleRate);

  // Cache atomic parameter pointers (Safe here)
  if (!attackParam)
    attackParam = apvts.getRawParameterValue("ATTACK");
  if (!decayParam)
    decayParam = apvts.getRawParameterValue("DECAY");
  if (!morphParam)
    morphParam = apvts.getRawParameterValue("MORPH");
  if (!cutoffParam)
    cutoffParam = apvts.getRawParameterValue("CUTOFF");
  if (!resonanceParam)
    resonanceParam = apvts.getRawParameterValue("RESONANCE");
  if (!lfoRateParam)
    lfoRateParam = apvts.getRawParameterValue("LFO_RATE");
  if (!lfoDepthParam)
    lfoDepthParam = apvts.getRawParameterValue("LFO_DEPTH");

  // Pre-allocate scratch buffer
  scratchBuffer.setSize(2, samplesPerBlock);
}

void MelodyEngineAudioProcessor::releaseResources() {
  // When playback stops, you can use this as an opportunity to free up any
  // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool MelodyEngineAudioProcessor::isBusesLayoutSupported(
    const BusesLayout &layouts) const {
#if JucePlugin_IsMidiEffect
  juce::ignoreUnused(layouts);
  return true;
#else
  if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
      layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
    return false;

#if !JucePlugin_IsSynth
  if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
    return false;
#endif

  return true;
#endif
}
#endif

void MelodyEngineAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer,
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
        if (cmd.stepIdx >= 0 && cmd.stepIdx < Melodic::NUM_PHRASE_STEPS) {
          if (cmd.type == MelodyCommand::SetEvent) {
            phrase.events[cmd.stepIdx] = cmd.eventData;
            phraseDirty.store(true);
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
  if (auto *playHead = getPlayHead()) {
    auto position = playHead->getPosition();
    if (position && position->getIsPlaying()) {
      double bpm = position->getBpm().orFallback(120.0);
      double ppq = position->getPpqPosition().orFallback(0.0);
      double sampleRate = getSampleRate();

      double samplesPerBeat = (60.0 / bpm) * sampleRate;
      // double samplesPerStep = samplesPerBeat * 0.25; // 16th notes

      double currentSteps = ppq * 4.0;
      int stepIndex = (int)std::floor(currentSteps) % Melodic::NUM_PHRASE_STEPS;

      // Check for step boundary within this block
      double nextStepPpq = std::ceil(currentSteps) / 4.0;
      double ppqToNextStep = nextStepPpq - ppq;
      int samplesToNextStep = (int)(ppqToNextStep * samplesPerBeat);

      if (samplesToNextStep < buffer.getNumSamples()) {
        // Get Parameters from APVTS (Atomic Load with Null Guards)
        float attack = attackParam ? attackParam->load() : 0.01f;
        float decay = decayParam ? decayParam->load() : 0.4f;
        float morph = morphParam ? morphParam->load() : 0.0f;
        float cutoff = cutoffParam ? cutoffParam->load() : 2000.0f;
        float resonance = resonanceParam ? resonanceParam->load() : 0.0f;
        float lfoRate = lfoRateParam ? lfoRateParam->load() : 1.0f;
        float lfoDepth = lfoDepthParam ? lfoDepthParam->load() : 0.0f;

        synth.setParameters(attack, decay, morph, cutoff, resonance, lfoDepth,
                            lfoRate);

        // Process up to the new step
        if (samplesToNextStep > 0) {
          synth.processBlock(buffer, 0, samplesToNextStep);
        }

        // Trigger Logic for the NEW step
        int nextStepIndex = (stepIndex + 1) % Melodic::NUM_PHRASE_STEPS;
        auto &event = phrase.events[nextStepIndex];

        if (event.active) {
          // Probability check
          if (random.nextFloat() <= event.probability) {
            int quantizedNote = quantizer.quantize(
                (int)event.pitch, phrase.rootNote, phrase.scaleName);
            synth.triggerBaseNote(quantizedNote, event.velocity);
          }
        }

        // Process the rest of the block
        int remainingSamples = buffer.getNumSamples() - samplesToNextStep;
        if (remainingSamples > 0) {
          synth.processBlock(buffer, samplesToNextStep, remainingSamples);
        }

      } else {
        // No step change in this block
        synth.processBlock(buffer, 0, buffer.getNumSamples());
      }

      // Update GUI atomic
      currentStepAtomic.store(stepIndex);

    } else {
      // Not playing
      synth.processBlock(buffer, 0, buffer.getNumSamples());
      currentStepAtomic.store(-1);
    }
  } else {
    // No Playhead
    synth.processBlock(buffer, 0, buffer.getNumSamples());
    currentStepAtomic.store(-1);
  }

  updateSnapshotFromAudio();
}

//==============================================================================
bool MelodyEngineAudioProcessor::hasEditor() const {
  return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor *MelodyEngineAudioProcessor::createEditor() {
  return new MelodyEngineAudioProcessorEditor(*this);
}

//==============================================================================
void MelodyEngineAudioProcessor::getStateInformation(
    juce::MemoryBlock &destData) {
  juce::ValueTree state("MelodyEngineState");
  juce::ValueTree phraseTree("Phrase");
  phraseTree.setProperty("rootNote", phrase.rootNote, nullptr);
  phraseTree.setProperty("scaleName", phrase.scaleName, nullptr);

  for (int i = 0; i < phrase.events.size(); ++i) {
    auto &event = phrase.events[i];
    if (event.active) {
      juce::ValueTree eventTree("Event");
      eventTree.setProperty("index", i, nullptr);
      eventTree.setProperty("pitch", event.pitch, nullptr);
      eventTree.setProperty("velocity", event.velocity, nullptr);
      eventTree.setProperty("duration", event.duration, nullptr);
      eventTree.setProperty("probability", event.probability, nullptr);
      phraseTree.appendChild(eventTree, nullptr);
    }
  }

  state.appendChild(phraseTree, nullptr);
  std::unique_ptr<juce::XmlElement> xml(state.createXml());
  copyXmlToBinary(*xml, destData);
}

void MelodyEngineAudioProcessor::setStateInformation(const void *data,
                                                     int sizeInBytes) {
  std::unique_ptr<juce::XmlElement> xmlState(
      getXmlFromBinary(data, sizeInBytes));

  if (xmlState.get() != nullptr) {
    if (xmlState->hasTagName("MelodyEngineState")) {
      auto state = juce::ValueTree::fromXml(*xmlState);
      auto phraseTree = state.getChildWithName("Phrase");
      if (phraseTree.isValid()) {
        phrase.rootNote = phraseTree.getProperty("rootNote");
        phrase.scaleName = phraseTree.getProperty("scaleName").toString();

        // Reset all events first
        for (auto &event : phrase.events) {
          event.active = false;
        }

        for (int i = 0; i < phraseTree.getNumChildren(); ++i) {
          auto eventTree = phraseTree.getChild(i);
          int index = eventTree.getProperty("index");
          if (index >= 0 && index < phrase.events.size()) {
            auto &event = phrase.events[index];
            event.active = true;
            event.pitch = eventTree.getProperty("pitch");
            event.velocity = eventTree.getProperty("velocity");
            event.duration = eventTree.getProperty("duration");
            event.probability = eventTree.getProperty("probability");
          }
        }
      }
    }
  }
}

void MelodyEngineAudioProcessor::initializeDefaultPhrase() {
  // phrase.events is now a fixed-size array, no resize needed
  phrase.rootNote = 60; // C4
  phrase.scaleName = "Minor";

  // Zero-initialize all events first
  for (auto &event : phrase.events) {
    event = Melodic::NoteEvent(); // Reset to default constructor state
    event.active = false;
  }

  // Simple 8th note arpeggio (Minor triad: 0, 3, 7)
  int triad[] = {0, 3, 7, 12};
  for (int i = 0; i < Melodic::NUM_PHRASE_STEPS; i += 2) {
    auto &event = phrase.events[i];
    event.active = true;

    int interval = triad[(i / 2) % 4];
    event.pitch = quantizer.quantize(phrase.rootNote + interval,
                                     phrase.rootNote, phrase.scaleName);
    event.velocity = 0.8f;
    event.duration = 0.25f; // 16th note
    event.probability = 1.0f;
  }
}

juce::AudioProcessorValueTreeState::ParameterLayout
MelodyEngineAudioProcessor::createParameterLayout() {
  juce::AudioProcessorValueTreeState::ParameterLayout layout;

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "ATTACK", "Attack", juce::NormalisableRange<float>(0.01f, 2.0f, 0.01f),
      0.01f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "DECAY", "Decay", juce::NormalisableRange<float>(0.01f, 2.0f, 0.01f),
      0.4f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "MORPH", "Morph", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
      0.5f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "CUTOFF", "Cutoff",
      juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f),
      2000.0f)); // Skewed for Log-like feel
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "RESONANCE", "Resonance",
      juce::NormalisableRange<float>(0.0f, 0.95f, 0.01f), 0.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "LFO_RATE", "LFO Rate", juce::NormalisableRange<float>(0.1f, 20.0f, 0.1f),
      2.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "LFO_DEPTH", "LFO Depth",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.2f));

  return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new MelodyEngineAudioProcessor();
}
