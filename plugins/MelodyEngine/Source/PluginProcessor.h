/*
  ==============================================================================

    PluginProcessor.h
    Created: 29 Dec 2025
    Author:  DJstih

  ==============================================================================
*/

#pragma once

#include "MelodyStructures.h"
#include "ScaleQuantizer.h"
#include "WavetableSynth.h"
#include <JuceHeader.h>
#include <mutex>


//==============================================================================
class MelodyEngineAudioProcessor : public juce::AudioProcessor {
public:
  //==============================================================================
  MelodyEngineAudioProcessor();
  ~MelodyEngineAudioProcessor() override;

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

  //==============================================================================
  void initializeDefaultPhrase();

  Melodic::Phrase &getPhrase() { return phrase; }
  int getCurrentStep() const { return currentStep; }

  juce::AudioProcessorValueTreeState apvts;

  //==============================================================================
  // Thread-safe GUI API (accessible from MelodyCanvasComponent)
  std::atomic<int> currentStepAtomic{-1};

  // Thread-safe GUI API
  void getGuiSnapshot(Melodic::Phrase &target) {
    std::lock_guard<std::mutex> lock(phraseMutex);
    target = phraseSnapshot;
  }

  void updateSnapshotFromAudio() {
    if (phraseDirty) {
      if (phraseMutex.try_lock()) {
        phraseSnapshot = phrase;
        phraseMutex.unlock();
        phraseDirty = false;
      }
    }
  }

private:
  std::mutex phraseMutex;
  Melodic::Phrase phraseSnapshot;
  std::atomic<bool> phraseDirty{false};

public:
  struct MelodyCommand {
    enum Type { SetEvent };
    Type type;
    int stepIdx = 0;
    Melodic::NoteEvent eventData;
  };

  void queueCommand(const MelodyCommand &cmd) {
    if (cmd.stepIdx < 0 || cmd.stepIdx >= Melodic::NUM_PHRASE_STEPS)
      return;
    auto writer = commandFifo.write(1);
    if (writer.blockSize1 > 0) {
      commandBuffer[writer.startIndex1] = cmd;
    } else if (writer.blockSize2 > 0) {
      commandBuffer[writer.startIndex2] = cmd;
    }
  }

private:
  Melodic::Phrase phrase;
  ScaleQuantizer quantizer;
  WavetableSynth synth;

  // Sequencer State
  double accuracy = 0.0;
  int currentStep = 0;
  int lastStep = -1;
  juce::Random random;

  juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

  //==============================================================================
  // Transport tracking
  double lastProcessedSampleTime = -1.0;
  double currentBpm = 120.0;

  // Pre-allocated buffers
  juce::AudioBuffer<float> scratchBuffer;

  // Command Queue buffers (accessed through public queueCommand)
  juce::AbstractFifo commandFifo{1024};
  std::array<MelodyCommand, 1024> commandBuffer;

  // Atomic Parameter Pointers (Cached for realtime safety)
  std::atomic<float> *attackParam = nullptr;
  std::atomic<float> *decayParam = nullptr;
  std::atomic<float> *morphParam = nullptr;
  std::atomic<float> *cutoffParam = nullptr;
  std::atomic<float> *resonanceParam = nullptr;
  std::atomic<float> *lfoRateParam = nullptr;
  std::atomic<float> *lfoDepthParam = nullptr;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MelodyEngineAudioProcessor)
};
