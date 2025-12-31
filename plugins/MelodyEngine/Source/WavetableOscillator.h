#pragma once
#include <JuceHeader.h>
#include <array>
#include <vector>


class WavetableOscillator {
public:
  enum Waveform { Sine = 0, Saw, Square, Triangle, NumWaveforms };

  WavetableOscillator();
  ~WavetableOscillator() = default;

  void setFrequency(float frequency, float sampleRate);
  void setWaveform(Waveform wave);
  float getSample();
  void reset();

private:
  void createWavetables();

  static constexpr int TABLE_SIZE = 2048;
  std::array<std::vector<float>, NumWaveforms> wavetables;

  float currentPhase = 0.0f;
  float phaseIncrement = 0.0f;
  Waveform currentWaveform = Sine;
};
