#pragma once
#include "WavetableOscillator.h"
#include <JuceHeader.h>
#include <djstih_dsp/djstih_dsp.h>

class WavetableSynth {
public:
  WavetableSynth();
  ~WavetableSynth() = default;

  void prepareToPlay(double sampleRate);
  void setParameters(float attack, float decay, float morph, float cutoff,
                     float resonance, float lfoAmount, float lfoRate);
  void triggerBaseNote(int midiNote, float velocity);
  void processBlock(juce::AudioBuffer<float> &buffer, int startSample,
                    int numSamples);

private:
  WavetableOscillator osc1;
  WavetableOscillator osc2;
  WavetableOscillator lfo; // LFO for filter modulation
  djstih::AdsrEnvelope envelope;

  double sampleRate = 48000.0;
  juce::SmoothedValue<float> currentMorph;
  juce::SmoothedValue<float> currentCutoff;
  juce::SmoothedValue<float> lfoAmount;
  juce::SmoothedValue<float> lfoRate;

  // Not smoothed as they are usually triggered per note or don't cause zipper
  // noise as noticeably
  float currentResonance = 0.0f;

  // One Pole Filter State
  float z1 = 0.0f; // Unit delay

  float applyOnePoleFilter(float input, float cutoff);
};
