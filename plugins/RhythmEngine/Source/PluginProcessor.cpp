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
  kickFreqParam = apvts.getRawParameterValue("KICK_FREQ");
  kickDecayParam = apvts.getRawParameterValue("KICK_DECAY");
  bassCutoffParam = apvts.getRawParameterValue("BASS_CUTOFF");
  bassDriveParam = apvts.getRawParameterValue("BASS_DRIVE");
  sidechainAmtParam = apvts.getRawParameterValue("SIDECHAIN_AMT");
  bassAttackParam = apvts.getRawParameterValue("BASS_ATTACK");
  bassDecayParam = apvts.getRawParameterValue("BASS_DECAY");
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

  float kickFreq = *kickFreqParam;
  float kickDecay = *kickDecayParam;
  float bassCutoff = *bassCutoffParam;
  float bassDrive = *bassDriveParam;
  float sidechainAmt = *sidechainAmtParam;

  float bassAttack = *bassAttackParam;
  float bassDecay = *bassDecayParam;

  kickEnv.setParameters(0.005f, kickDecay);
  bassEnv.setParameters(bassAttack, bassDecay);

  for (const auto metadata : midiMessages) {
    auto msg = metadata.getMessage();
    if (msg.isNoteOn()) {
      int note = msg.getNoteNumber();
      if (note == 36) {
        kickEnv.trigger();
        kickPhasor.reset();
      }
      if (note >= 48) {
        currentBassFreq = 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
        bassEnv.trigger();
      }
    }
  }

  auto *leftChannel = buffer.getWritePointer(0);
  auto *rightChannel =
      (totalNumOutputChannels > 1) ? buffer.getWritePointer(1) : nullptr;
  double fs = getSampleRate();

  for (int s = 0; s < buffer.getNumSamples(); ++s) {
    float kEnv = kickEnv.getNextSample();
    float kPitchObj = kickFreq * (1.0f + 3.0f * kEnv);
    float kPhase = kickPhasor.process(kPitchObj, fs);
    float kickSample =
        std::sin(kPhase * 2.0f * std::numbers::pi_v<float>) * kEnv;

    float bPhase = bassPhasor.process(currentBassFreq, fs);
    float bassRaw = (bPhase * 2.0f) - 1.0f;
    float bEnv = bassEnv.getNextSample();

    float alpha =
        2.0f * std::numbers::pi_v<float> * bassCutoff / static_cast<float>(fs);
    if (alpha > 1.0f)
      alpha = 1.0f;
    bassFilterState += alpha * (bassRaw - bassFilterState);
    float bassSample = bassFilterState * bEnv;

    float driveGain = 1.0f + (bassDrive * 9.0f);
    bassSample = djstih::ClipFunctions::softClip(bassSample * driveGain);

    float ducking = 1.0f - (kEnv * sidechainAmt);
    if (ducking < 0.0f)
      ducking = 0.0f;
    bassSample *= ducking;

    float out = kickSample + bassSample;
    leftChannel[s] = out;
    if (rightChannel)
      rightChannel[s] = out;
  }
}

bool RhythmEngineAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor *RhythmEngineAudioProcessor::createEditor() {
  return new RhythmEngineAudioProcessorEditor(*this);
}

void RhythmEngineAudioProcessor::getStateInformation(
    juce::MemoryBlock &destData) {
  auto state = apvts.copyState();
  std::unique_ptr<juce::XmlElement> xml(state.createXml());
  copyXmlToBinary(*xml, destData);
}

void RhythmEngineAudioProcessor::setStateInformation(const void *data,
                                                     int sizeInBytes) {
  std::unique_ptr<juce::XmlElement> xmlState(
      getXmlFromBinary(data, sizeInBytes));
  if (xmlState.get() != nullptr)
    if (xmlState->hasTagName(apvts.state.getType()))
      apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new RhythmEngineAudioProcessor();
}
