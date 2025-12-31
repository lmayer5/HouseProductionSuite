#include "WavetableSynth.h"
#include <cmath>

WavetableSynth::WavetableSynth() {
  osc1.setWaveform(WavetableOscillator::Saw);
  osc2.setWaveform(WavetableOscillator::Square);
  lfo.setWaveform(WavetableOscillator::Sine);
}

void WavetableSynth::prepareToPlay(double sr) {
  sampleRate = sr;
  envelope.setSampleRate(sr);

  // Initialize smoothers
  currentMorph.reset(sampleRate, 0.05); // 50ms ramp
  currentCutoff.reset(sampleRate, 0.05);
  lfoAmount.reset(sampleRate, 0.05);
  lfoRate.reset(sampleRate, 0.05);

  // Reset DSP state
  z1 = 0.0f;
  osc1.reset();
  osc2.reset();
  lfo.reset();
}

void WavetableSynth::setParameters(float attack, float decay, float morph,
                                   float cutoff, float resonance, float lfoAmt,
                                   float lfoFreq) {
  envelope.setParameters(attack, decay);

  // Set targets for smoothed values
  currentMorph.setTargetValue(morph);
  currentCutoff.setTargetValue(cutoff);
  currentResonance = resonance;
  lfoAmount.setTargetValue(lfoAmt);
  lfoRate.setTargetValue(lfoFreq);
  // Note: lfo.setFrequency will need per-sample update for smoothing to be
  // effective on rate
}

void WavetableSynth::triggerBaseNote(int midiNote, float velocity) {
  float frequency = juce::MidiMessage::getMidiNoteInHertz(midiNote);
  osc1.setFrequency(frequency, (float)sampleRate);
  osc2.setFrequency(frequency, (float)sampleRate); // Maybe detune slightly?

  // Optional: Reset phase for punchy bass
  osc1.reset();
  osc2.reset();

  envelope.trigger();
}

float WavetableSynth::applyOnePoleFilter(float input, float cutoff) {
  // Simple 1-pole LP
  // coeff = 1.0 - exp( -2pi * fc / fs)
  // For efficiency we can approximate or use std::exp
  float c = 1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * cutoff /
                            (float)sampleRate);
  z1 += c * (input - z1);
  return z1;
}

void WavetableSynth::processBlock(juce::AudioBuffer<float> &buffer,
                                  int startSample, int numSamples) {
  auto *channel0 = buffer.getWritePointer(0);
  auto *channel1 =
      (buffer.getNumChannels() > 1) ? buffer.getWritePointer(1) : nullptr;

  for (int i = 0; i < numSamples; ++i) {
    float env = envelope.getNextSample();

    // If envelope is effectively zero and we are not in attack/decay, we could
    // skip calculation But for now verify simply

    float s1 = osc1.getSample();
    float s2 = osc2.getSample();
    float cMorph = currentMorph.getNextValue();
    float cLfoAmt = lfoAmount.getNextValue();
    float cLfoRate = lfoRate.getNextValue();
    float cCutoff = currentCutoff.getNextValue();

    // Update LFO frequency
    lfo.setFrequency(cLfoRate, (float)sampleRate);
    float lfoVal = lfo.getSample();

    // Morph: 0.0 = Osc1, 1.0 = Osc2
    float mixed = (1.0f - cMorph) * s1 + cMorph * s2;

    // Apply Envelope
    mixed *= env;

    // Apply Filter with LFO Modulation
    // Modulate cutoff: base * (1 + lfo * amount)
    float modFactor = 1.0f + (lfoVal * cLfoAmt);
    float modulatedCutoff = cCutoff * modFactor;

    // Clamp cutoff
    modulatedCutoff = juce::jlimit(20.0f, 20000.0f, modulatedCutoff);

    mixed = applyOnePoleFilter(mixed, modulatedCutoff);

    // Output to buffer (Add or Overwrite? Usually add if polyphonic, but this
    // is monophonic voice class for now?) The PluginProcessor will likely
    // handle the buffer clearing, so we can Add But let's assume this writes
    // into a cleared buffer or mixes. Given how it's called, let's ADD to the
    // buffer to support multiple voices if we had them.

    int sampleIdx = startSample + i;

    // Gain down slightly to avoid clipping
    mixed *= 0.5f;

    channel0[sampleIdx] += mixed;
    if (channel1) {
      channel1[sampleIdx] += mixed;
    }
  }
}
