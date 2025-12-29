/*
  ==============================================================================

    BassVoice.h
    Created: 28 Dec 2025
    Author:  djstih

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <djstih_dsp/djstih_dsp.h>

class BassVoice {
public:
  BassVoice() = default;

  void prepare(double sampleRate, int samplesPerBlock) {
    phasor.prepare(sampleRate);

    // Filter Setup
    juce::dsp::ProcessSpec spec{sampleRate, (juce::uint32)samplesPerBlock,
                                1}; // Process mono then duplicate
    filter.prepare(spec);
    filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);

    ampEnvelope.prepare(sampleRate);
    filterEnvelope.prepare(sampleRate);
  }

  void noteOn(int noteNumber, float velocity) {
    float freq = (float)juce::MidiMessage::getMidiNoteInHertz(noteNumber);
    phasor.setFrequency(freq);

    // Retrigger envelopes
    ampEnvelope.noteOn();
    filterEnvelope.noteOn();

    currentVelocity = velocity;
  }

  void noteOff() {
    ampEnvelope.noteOff();
    filterEnvelope.noteOff();
  }

  void setParameters(float cutoff, float resonance, float envAmount) {
    baseCutoff = cutoff;
    this->resonance = resonance;
    this->envAmount = envAmount;

    // Standard ADSR params for bass (hardcoded or could be parameters later)
    djstih::Envelope::Parameters ampParams;
    ampParams.attack = 0.01f;
    ampParams.decay = 0.2f;
    ampParams.sustain = 0.5f;
    ampParams.release = 0.1f;
    ampEnvelope.setParameters(ampParams);

    // Filter Env follows amp generally or tighter
    djstih::Envelope::Parameters filtParams;
    filtParams.attack = 0.01f;
    filtParams.decay = 0.3f;
    filtParams.sustain = 0.0f; // Plucky filter
    filtParams.release = 0.1f;
    filterEnvelope.setParameters(filtParams);
  }

  void renderNextBlock(juce::AudioBuffer<float> &outputBuffer, int startSample,
                       int numSamples) {
    auto *left = outputBuffer.getWritePointer(0, startSample);
    auto *right = outputBuffer.getNumChannels() > 1
                      ? outputBuffer.getWritePointer(1, startSample)
                      : nullptr;

    for (int i = 0; i < numSamples; ++i) {
      if (!ampEnvelope.isActive())
        continue;

      float ampEnv = ampEnvelope.nextSample();
      float filtEnv = filterEnvelope.nextSample();

      // 1. Oscillator (Sawtooth: 2*phase - 1)
      // Phasor gives [0,1), so 2x - 1 gives [-1, 1).
      // PolyBLEP could be added here for anti-aliasing but keeping it raw for
      // now as per "Prototyping"
      float rawPhase = phasor.nextSample();
      float saw = (rawPhase * 2.0f) - 1.0f;

      // 2. Filter modulation
      float modCutoff = baseCutoff + (baseCutoff * 5.0f * filtEnv * envAmount);
      modCutoff = std::min(modCutoff, 20000.0f);

      filter.setCutoffFrequency(modCutoff);
      filter.setResonance(resonance);

      float filtered = filter.processSample(0, saw);

      // 3. Amp & Output
      float sample = filtered * ampEnv * currentVelocity;

      left[i] += sample;
      if (right)
        right[i] += sample;
    }
  }

  bool isActive() const { return ampEnvelope.isActive(); }

private:
  djstih::Phasor phasor;
  juce::dsp::StateVariableTPTFilter<float> filter;

  djstih::Envelope ampEnvelope;
  djstih::Envelope filterEnvelope;

  float baseCutoff = 500.0f;
  float resonance = 1.0f;
  float envAmount = 0.5f;
  float currentVelocity = 0.0f;
};
