#include "PluginProcessor.h"
#include "PluginEditor.h"

SampleEngineAudioProcessor::SampleEngineAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties().withOutput(
          "Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
#endif
{
}

SampleEngineAudioProcessor::~SampleEngineAudioProcessor() {}

const juce::String SampleEngineAudioProcessor::getName() const {
  return JucePlugin_Name;
}

bool SampleEngineAudioProcessor::acceptsMidi() const {
#if JucePlugin_WantsMidiInput
  return true;
#else
  return false;
#endif
}

bool SampleEngineAudioProcessor::producesMidi() const {
#if JucePlugin_ProducesMidiOutput
  return true;
#else
  return false;
#endif
}

bool SampleEngineAudioProcessor::isMidiEffect() const {
#if JucePlugin_IsMidiEffect
  return true;
#else
  return false;
#endif
}

double SampleEngineAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int SampleEngineAudioProcessor::getNumPrograms() { return 1; }

int SampleEngineAudioProcessor::getCurrentProgram() { return 0; }

void SampleEngineAudioProcessor::setCurrentProgram(int index) {}

const juce::String SampleEngineAudioProcessor::getProgramName(int index) {
  return {};
}

void SampleEngineAudioProcessor::changeProgramName(
    int index, const juce::String &newName) {}

void SampleEngineAudioProcessor::prepareToPlay(double sampleRate,
                                               int samplesPerBlock) {
  // prepare voice
}

void SampleEngineAudioProcessor::releaseResources() {}

bool SampleEngineAudioProcessor::isBusesLayoutSupported(
    const BusesLayout &layouts) const {
  if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
      layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
    return false;

  return true;
}

void SampleEngineAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                              juce::MidiBuffer &midiMessages) {
  juce::ScopedNoDenormals noDenormals;
  auto totalNumInputChannels = getTotalNumInputChannels();
  auto totalNumOutputChannels = getTotalNumOutputChannels();

  for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    buffer.clear(i, 0, buffer.getNumSamples());

  // Process logic
}

bool SampleEngineAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor *SampleEngineAudioProcessor::createEditor() {
  return new SampleEngineAudioProcessorEditor(*this);
}

void SampleEngineAudioProcessor::getStateInformation(
    juce::MemoryBlock &destData) {
  auto state = apvts.copyState();
  std::unique_ptr<juce::XmlElement> xml(state.createXml());
  copyXmlToBinary(*xml, destData);
}

void SampleEngineAudioProcessor::setStateInformation(const void *data,
                                                     int sizeInBytes) {
  std::unique_ptr<juce::XmlElement> xmlState(
      getXmlFromBinary(data, sizeInBytes));
  if (xmlState.get() != nullptr)
    if (xmlState->hasTagName(apvts.state.getType()))
      apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessorValueTreeState::ParameterLayout
SampleEngineAudioProcessor::createParameterLayout() {
  juce::AudioProcessorValueTreeState::ParameterLayout layout;
  // Add parameters here
  return layout;
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new SampleEngineAudioProcessor();
}
