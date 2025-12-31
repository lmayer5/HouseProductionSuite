#include "WavetableOscillator.h"
#include <cmath>

WavetableOscillator::WavetableOscillator() { createWavetables(); }

void WavetableOscillator::createWavetables() {
  for (int i = 0; i < NumWaveforms; ++i) {
    wavetables[i].resize(TABLE_SIZE);
    auto &table = wavetables[i];

    for (int n = 0; n < TABLE_SIZE; ++n) {
      float t = (float)n / (float)TABLE_SIZE;
      float angle = t * 2.0f * juce::MathConstants<float>::pi;

      switch (i) {
      case Sine:
        table[n] = std::sin(angle);
        break;
      case Saw:
        table[n] = 1.0f - 2.0f * t;
        break;
      case Square:
        table[n] = (t < 0.5f) ? 1.0f : -1.0f;
        break;
      case Triangle:
        table[n] = 2.0f * std::abs(2.0f * (t - std::floor(t + 0.5f))) - 1.0f;
        break;
      }
    }
  }
}

void WavetableOscillator::setFrequency(float frequency, float sampleRate) {
  if (sampleRate > 0.0f) {
    auto cyclesPerSample = frequency / sampleRate;
    phaseIncrement = cyclesPerSample * (float)TABLE_SIZE;
  }
}

void WavetableOscillator::setWaveform(Waveform wave) { currentWaveform = wave; }

float WavetableOscillator::getSample() {
  // Linear Interpolation
  int index0 = (int)currentPhase;
  int index1 = (index0 + 1) % TABLE_SIZE;
  float frac = currentPhase - (float)index0;

  const auto &table = wavetables[currentWaveform];
  float value = table[index0] + frac * (table[index1] - table[index0]);

  currentPhase += phaseIncrement;
  if (currentPhase >= (float)TABLE_SIZE)
    currentPhase -= (float)TABLE_SIZE;

  return value;
}

void WavetableOscillator::reset() { currentPhase = 0.0f; }
