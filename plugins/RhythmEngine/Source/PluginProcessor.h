/*
  ==============================================================================

    PluginProcessor.h
    Created: 29 Dec 2025
    Author:  DJstih

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <djstih_dsp/djstih_dsp.h> // Updated include path

//==============================================================================
class RhythmEngineAudioProcessor : public juce::AudioProcessor {
public:
  //==============================================================================
  RhythmEngineAudioProcessor();
  ~RhythmEngineAudioProcessor() override;

  //==============================================================================
  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
  bool isBusesLayoutSupported(const BusesLayout &layouts) const override;
#endif

  void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;

  //==============================================================================
  juce::AudioProcessorEditor *createEditor() override;
  bool hasEditor() const override;

  //==============================================================================
  const juce::String getName() const override;

  bool acceptsMidi() const override;
  bool producesMidi() const override;
  bool isMidiEffect() const override;
  double getTailLengthSeconds() const override;

  //==============================================================================
  int getNumPrograms() override;
  int getCurrentProgram() override;
  void setCurrentProgram(int index) override;
  const juce::String getProgramName(int index) override;
  void changeProgramName(int index, const juce::String &newName) override;

  //==============================================================================
  void getStateInformation(juce::MemoryBlock &destData) override;
  void setStateInformation(const void *data, int sizeInBytes) override;

  juce::AudioProcessorValueTreeState apvts;

private:
  juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

  djstih::Phasor kickPhasor;
  djstih::AdsrEnvelope kickEnv;

  djstih::Phasor bassPhasor;
  djstih::AdsrEnvelope bassEnv;

  float currentBassFreq = 55.0f;
  float bassFilterState = 0.0f;

  std::atomic<float> *kickFreqParam = nullptr;
  std::atomic<float> *kickDecayParam = nullptr;
  std::atomic<float> *bassCutoffParam = nullptr;
  std::atomic<float> *bassDriveParam = nullptr;
  std::atomic<float> *sidechainAmtParam = nullptr;
  std::atomic<float> *bassAttackParam = nullptr;
  std::atomic<float> *bassDecayParam = nullptr;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RhythmEngineAudioProcessor)
};
