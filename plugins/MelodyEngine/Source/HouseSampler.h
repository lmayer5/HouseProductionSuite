#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>

class HouseSynthSound : public juce::SynthesiserSound {
public:
  HouseSynthSound() {}

  bool appliesToNote(int midiNoteNumber) override { return true; }
  bool appliesToChannel(int midiChannel) override { return true; }

  juce::AudioBuffer<float> sampleData;
  double rootFrequency = 261.63; // Middle C
};

class HouseSynthVoice : public juce::SynthesiserVoice {
public:
  HouseSynthVoice();

  bool canPlaySound(juce::SynthesiserSound *sound) override;
  void startNote(int midiNoteNumber, float velocity,
                 juce::SynthesiserSound *sound,
                 int currentPitchWheelPosition) override;
  void stopNote(float velocity, bool allowTailOff) override;
  void pitchWheelMoved(int newPitchWheelValue) override {}
  void controllerMoved(int controllerNumber, int newControllerValue) override {}
  void renderNextBlock(juce::AudioBuffer<float> &outputBuffer, int startSample,
                       int numSamples) override;

  void updateADSR(float a, float d, float s, float r);

private:
  double currentSamplePos = 0.0;
  double pitchRatio = 0.0;
  juce::ADSR adsr;
  juce::ADSR::Parameters adsrParams;
};
