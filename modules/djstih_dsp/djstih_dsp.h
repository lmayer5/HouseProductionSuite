/*
  ==============================================================================

    djstih_dsp.h
    Created: 29 Dec 2025
    Author:  djstih

  ==============================================================================
*/

/*******************************************************************************
 The block below describes the properties of this module, and is read by
 the Projucer to automatically generate project code that uses it.
 For details, see the Juce Module Format.txt file.

 BEGIN_JUCE_MODULE_DECLARATION
  ID:               djstih_dsp
  vendor:           djstih
  version:          1.0.0
  name:             DSP Utilities
  description:      Core DSP building blocks for synthesis
  website:          http://www.juce.com
  license:          GPL/Commercial

  dependencies:     juce_core juce_audio_basics
 END_JUCE_MODULE_DECLARATION

*******************************************************************************/

#pragma once
#include <cmath>
#include <juce_core/juce_core.h>
#include <numbers>

namespace djstih {

/**
 * A phase accumulator forgenerating raw waveforms.
 */
class Phasor {
public:
  Phasor() = default;

  /**
   * Increments the phase and returns the current value.
   * @param frequencyHz   The frequency of the oscillator.
   * @param sampleRate    The system sample rate.
   * @return              Current phase in range [0.0, 1.0).
   */
  float process(float frequencyHz, double sampleRate) noexcept {
    if (sampleRate > 0.0) {
      double increment = frequencyHz / sampleRate;
      phase += increment;

      // Wrap phase to [0.0, 1.0)
      if (phase >= 1.0)
        phase -= 1.0;
    }
    return static_cast<float>(phase);
  }

  void reset() noexcept { phase = 0.0; }

private:
  double phase = 0.0;
};

/**
 * Fast, per-sample envelope generation for drums.
 * State Machine: IDLE, ATTACK, DECAY, HOLD.
 */
class AdsrEnvelope {
public:
  enum class State { Idle, Attack, Decay, Hold };

  AdsrEnvelope() = default;

  void setSampleRate(double newSampleRate) noexcept {
    sampleRate = newSampleRate;
  }

  void setParameters(float attackTimeSec, float decayTimeSec) noexcept {
    attackRate = static_cast<float>(
        1.0 / (attackTimeSec * sampleRate + 1.0)); // Simple linear increment
    decayCoeff =
        static_cast<float>(std::exp(-1.0 / (decayTimeSec * sampleRate)));
  }

  void trigger() noexcept {
    state = State::Attack;
    currentLevel = 0.0f;
  }

  float getNextSample() noexcept {
    switch (state) {
    case State::Idle:
      currentLevel = 0.0f;
      break;

    case State::Attack:
      currentLevel += attackRate;
      if (currentLevel >= 1.0f) {
        currentLevel = 1.0f;
        state = State::Decay;
      }
      break;

    case State::Decay:
      currentLevel *= decayCoeff;
      if (currentLevel < 0.001f) {
        currentLevel = 0.0f;
        state = State::Idle;
      }
      break;

    case State::Hold: // Not used for one-shot but available
      break;
    }
    return currentLevel;
  }

private:
  State state = State::Idle;
  double sampleRate = 48000.0;
  float currentLevel = 0.0f;
  float attackRate = 0.1f;
  float decayCoeff = 0.99f;
};

/**
 * Static utility for saturation.
 */
class ClipFunctions {
public:
  /**
   * Soft clip using x / (1 + |x|).
   */
  static float softClip(float input) noexcept {
    return input / (1.0f + std::abs(input));
  }
};

} // namespace djstih

// Include additional DSP components
#include "PunchInFX.h"
