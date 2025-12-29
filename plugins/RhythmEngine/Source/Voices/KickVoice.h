/*
  ==============================================================================

    KickVoice.h
    Created: 28 Dec 2025
    Author:  djstih

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <djstih_dsp/djstih_dsp.h>

class KickVoice {
public:
  KickVoice() = default;

  void prepare(double sampleRate, int samplesPerBlock) {
    fs = sampleRate;

    phasor.prepare(sampleRate);
    ampEnvelope.prepare(sampleRate);
    pitchEnvelope.prepare(sampleRate);
    clickEnvelope.prepare(sampleRate);
  }

  void setParameters(float pitch, float decay, float clickAmount) {
    basePitch = pitch;

    // Amp Env
    djstih::Envelope::Parameters ampParams;
    ampParams.attack = 0.001f;
    ampParams.decay = decay;
    ampParams.sustain = 0.0f;
    ampParams.release = 0.01f;
    ampEnvelope.setParameters(ampParams);

    // Pitch Env
    djstih::Envelope::Parameters pitchParams;
    pitchParams.attack = 0.0001f;
    pitchParams.decay = 0.05f;
    pitchParams.sustain = 0.0f;
    pitchParams.release = 0.01f;
    pitchEnvelope.setParameters(pitchParams);

    // Click Env
    djstih::Envelope::Parameters clickParams;
    clickParams.attack = 0.0001f;
    clickParams.decay = 0.005f;
    clickParams.sustain = 0.0f;
    clickParams.release = 0.001f;
    clickEnvelope.setParameters(clickParams);

    this->clickAmount = clickAmount;
  }

  void trigger() {
    phasor.reset();
    ampEnvelope.reset();
    ampEnvelope.noteOn();
    pitchEnvelope.reset();
    pitchEnvelope.noteOn();
    clickEnvelope.reset();
    clickEnvelope.noteOn();
  }

  /**
   * Renders audio to outputBuffer and writes the amplitude envelope to
   * envelopeBuffer. envelopeBuffer must be at least numSamples long.
   */
  void renderNextBlock(juce::AudioBuffer<float> &outputBuffer,
                       float *envelopeBuffer, int startSample, int numSamples) {
    auto *left = outputBuffer.getWritePointer(0, startSample);
    auto *right = outputBuffer.getNumChannels() > 1
                      ? outputBuffer.getWritePointer(1, startSample)
                      : nullptr;

    for (int i = 0; i < numSamples; ++i) {
      float ampEnv = 0.0f;

      if (ampEnvelope.isActive()) {
        ampEnv = ampEnvelope.nextSample();
        float pitchEnv = pitchEnvelope.nextSample();
        float clickEnv = clickEnvelope.nextSample();

        // Pitch Mod
        float freqMult = 1.0f + (3.0f * pitchEnv);
        phasor.setFrequency(basePitch * freqMult);

        // Osc
        float osc = std::sin(phasor.nextSampleRadians());

        // Click
        float noise = (random.nextFloat() * 2.0f - 1.0f);
        float click = noise * clickEnv * clickAmount;

        float sample = (osc + click) * ampEnv;

        left[i] += sample;
        if (right)
          right[i] += sample;
      } else {
        // If inactive, we still need to zero the envelope buffer?
        // Or just output 0.
        // Assuming buffer was technically added to, we don't need to clear
        // audio if inactive? But we DO need to write 0 to envelope buffer for
        // safety.
        ampEnv = 0.0f;
      }

      if (envelopeBuffer != nullptr)
        envelopeBuffer[startSample + i] = ampEnv;
    }
  }

private:
  double fs = 44100.0;

  djstih::Phasor phasor;
  djstih::Envelope ampEnvelope;
  djstih::Envelope pitchEnvelope;
  djstih::Envelope clickEnvelope;

  float basePitch = 50.0f;
  float clickAmount = 0.5f;

  juce::Random random;
};
