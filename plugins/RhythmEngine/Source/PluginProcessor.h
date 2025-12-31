/*
  ==============================================================================

    PluginProcessor.h
    Created: 29 Dec 2025
    Author:  DJstih

  ==============================================================================
*/

#pragma once

#include "RhythmStructures.h"
#include <JuceHeader.h>
#include <djstih_dsp/djstih_dsp.h> // Updated include path
#include <mutex>

// Command structure for lock-free GUI -> Audio communication
struct RhythmCommand {
  enum Type { ToggleStep, UpdateVelocity, SetTrackGain };
  Type type;
  int trackIdx = 0;
  int stepIdx = 0;
  float value = 0.0f;
};

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

  // Pattern access for UI
  RhythmEngine::Pattern &getPattern() { return pattern; }
  const RhythmEngine::Pattern &getPattern() const { return pattern; }

  juce::AudioProcessorValueTreeState apvts;

  //==============================================================================
  // Thread-safe GUI API (accessible from SequencerGridComponent)
  std::atomic<int> currentStepAtomic{-1};

  void getGuiSnapshot(RhythmEngine::Pattern &target) {
    std::lock_guard<std::mutex> lock(patternMutex);
    target = patternSnapshot;
  }

  void updateSnapshotFromAudio() {
    if (patternDirty) {
      if (patternMutex.try_lock()) {
        patternSnapshot = pattern;
        patternMutex.unlock();
        patternDirty = false;
      }
    }
  }

private:
  std::mutex patternMutex;
  RhythmEngine::Pattern patternSnapshot;
  std::atomic<bool> patternDirty{false};

public:
  void queueCommand(const RhythmCommand &cmd) {
    auto writer = commandFifo.write(1);
    if (writer.blockSize1 > 0) {
      commandBuffer[writer.startIndex1] = cmd;
    } else if (writer.blockSize2 > 0) {
      commandBuffer[writer.startIndex2] = cmd;
    }
  }

private:
  juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

  djstih::AdsrEnvelope kickEnv;
  djstih::AdsrEnvelope bassEnv;
  djstih::AdsrEnvelope clapEnv;
  djstih::AdsrEnvelope hatEnv;

  djstih::Phasor kickPhasor;
  djstih::Phasor bassPhasor;

  float bassFilterState = 0.0f;
  float clapFilterState = 0.0f;
  float hatFilterState = 0.0f;

  float currentBassFreq = 60.0f;

  // Noise generator for percussion
  juce::Random random;

  // Track Gains
  float kickGain = 1.0f;
  float bassGain = 0.7f;
  float clapGain = 0.6f;
  float hatGain = 0.4f;

  std::atomic<float> *kickFreqParam = nullptr;
  std::atomic<float> *kickDecayParam = nullptr;
  std::atomic<float> *bassCutoffParam = nullptr;
  std::atomic<float> *bassDriveParam = nullptr;
  std::atomic<float> *sidechainAmtParam = nullptr;
  std::atomic<float> *bassAttackParam = nullptr;
  std::atomic<float> *bassDecayParam = nullptr;

  // Sequencer Pattern Data
  RhythmEngine::Pattern pattern;

  // Helper methods for serialization
  juce::ValueTree patternToValueTree() const;
  void patternFromValueTree(const juce::ValueTree &tree);

  // Initialize default patterns
  void initializeDefaultPattern();

  // Transport tracking
  double lastProcessedSampleTime = -1.0;
  double currentBpm = 120.0;

  // Pre-allocated buffers
  juce::AudioBuffer<float> scratchBuffer;

  // Command Queue buffers (accessed through public queueCommand)
  juce::AbstractFifo commandFifo{1024};
  std::array<RhythmCommand, 1024> commandBuffer;

  // Smoothed Parameters
  juce::SmoothedValue<float> smoothKickFreq;
  juce::SmoothedValue<float> smoothBassCutoff;
  juce::SmoothedValue<float> smoothBassDrive;
  juce::SmoothedValue<float> smoothSidechainAmt;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RhythmEngineAudioProcessor)
};
