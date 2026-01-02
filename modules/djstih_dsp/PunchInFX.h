/*
  ==============================================================================

    PunchInFX.h
    Created: 02 Jan 2026
    Author:  DJstih

    TE-inspired Punch-In Effects: momentary performance FX triggered by buttons.
    - StutterFX: Loops a portion of the audio buffer (1/4, 1/8, 1/16)
    - SweepFilterFX: Momentary high-pass or low-pass sweeps
    - BitcrushFX: Reduces bit depth for lo-fi effect

  ==============================================================================
*/

#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <juce_audio_basics/juce_audio_basics.h>

namespace djstih {

/**
 * Stutter/Loop effect - captures and loops a portion of the audio buffer.
 * When active, repeatedly plays back a captured segment.
 */
class StutterFX {
public:
  enum class Division { Off = 0, Quarter, Eighth, Sixteenth };

  StutterFX() = default;

  void prepare(double sampleRate, int maxBufferSize) {
    this->sampleRate =
        sampleRate > 0.0 ? sampleRate : 44100.0; // Guard against zero
    int bufferSize = static_cast<int>(this->sampleRate);
    if (bufferSize < 1)
      bufferSize = 44100;                  // Fallback for safety
    circularBuffer.setSize(2, bufferSize); // 1 second max
    circularBuffer.clear();
    writePosition = 0;
    readPosition = 0;
    loopLength = 0;
    isCapturing = false;
    isPlaying = false;
  }

  void setDivision(Division div, double bpm) {
    if (div == Division::Off || bpm <= 0.0 || sampleRate <= 0.0) {
      isPlaying = false;
      loopLength = 0;
      return;
    }

    double beatsPerSecond = bpm / 60.0;
    if (beatsPerSecond <= 0.0) {
      loopLength = 0;
      return;
    }
    double samplesPerBeat = sampleRate / beatsPerSecond;

    switch (div) {
    case Division::Quarter:
      loopLength = static_cast<int>(samplesPerBeat);
      break;
    case Division::Eighth:
      loopLength = static_cast<int>(samplesPerBeat / 2.0);
      break;
    case Division::Sixteenth:
      loopLength = static_cast<int>(samplesPerBeat / 4.0);
      break;
    default:
      loopLength = 0;
      break;
    }

    int maxLen = circularBuffer.getNumSamples();
    if (maxLen > 0) {
      loopLength = std::min(loopLength, maxLen);
    } else {
      loopLength = 0;
    }
  }

  void activate(Division div, double bpm) {
    setDivision(div, bpm);
    if (loopLength > 0) {
      isCapturing = true;
      capturedSamples = 0;
      readPosition = 0;
    }
  }

  void deactivate() {
    isPlaying = false;
    isCapturing = false;
  }

  void process(juce::AudioBuffer<float> &buffer) {
    int numSamples = buffer.getNumSamples();
    int bufferLen = circularBuffer.getNumSamples();

    // Safety: if buffer not properly prepared, skip processing
    if (bufferLen <= 0)
      return;

    int numChannels =
        std::min(buffer.getNumChannels(), circularBuffer.getNumChannels());

    for (int s = 0; s < numSamples; ++s) {
      if (isCapturing && loopLength > 0) {
        // Capture audio into circular buffer
        for (int ch = 0; ch < numChannels; ++ch) {
          circularBuffer.setSample(ch, writePosition, buffer.getSample(ch, s));
        }
        writePosition = (writePosition + 1) % bufferLen;
        capturedSamples++;

        if (capturedSamples >= loopLength) {
          isCapturing = false;
          isPlaying = true;
          readPosition = (writePosition - loopLength + bufferLen) % bufferLen;
        }
      }

      if (isPlaying && loopLength > 0 && bufferLen > 0) {
        // Play back from captured buffer (replacing input)
        for (int ch = 0; ch < numChannels; ++ch) {
          buffer.setSample(ch, s, circularBuffer.getSample(ch, readPosition));
        }

        // Simple wrap within loop region
        int loopStart = (writePosition - loopLength + bufferLen) % bufferLen;
        int offsetInLoop = (readPosition - loopStart + bufferLen) % bufferLen;
        offsetInLoop = (offsetInLoop + 1) % loopLength;
        readPosition = (loopStart + offsetInLoop) % bufferLen;
      }
    }
  }

  bool isActive() const { return isPlaying || isCapturing; }

private:
  juce::AudioBuffer<float> circularBuffer;
  double sampleRate = 44100.0;
  int writePosition = 0;
  int readPosition = 0;
  int loopLength = 0;
  int capturedSamples = 0;
  bool isCapturing = false;
  bool isPlaying = false;
};

/**
 * Sweep Filter - momentary HP/LP sweep effect.
 * Uses a simple one-pole filter with exponential frequency sweep.
 */
class SweepFilterFX {
public:
  enum class Mode { Off, HighPass, LowPass };

  SweepFilterFX() = default;

  void prepare(double sampleRate) {
    this->sampleRate =
        sampleRate > 0.0 ? sampleRate : 44100.0; // Guard against zero
    filterState[0] = 0.0f;
    filterState[1] = 0.0f;
    currentCutoff = 1000.0f;
  }

  void setMode(Mode mode) { this->mode = mode; }

  void process(juce::AudioBuffer<float> &buffer, float sweepPosition) {
    if (mode == Mode::Off)
      return;

    // Exponential sweep: 20Hz to 20kHz based on sweepPosition (0-1)
    float minFreq = 20.0f;
    float maxFreq = 20000.0f;

    if (mode == Mode::HighPass) {
      // HP sweep from 20Hz to 10kHz
      currentCutoff = minFreq * std::pow(500.0f, sweepPosition);
    } else {
      // LP sweep from 20kHz down to 200Hz
      currentCutoff = maxFreq * std::pow(0.01f, sweepPosition);
    }

    currentCutoff = std::clamp(currentCutoff, minFreq, maxFreq);

    float alpha =
        2.0f * 3.14159f * currentCutoff / static_cast<float>(sampleRate);
    alpha = std::clamp(alpha, 0.0f, 1.0f);

    int numChannels = std::min(buffer.getNumChannels(), 2);
    int numSamples = buffer.getNumSamples();

    for (int ch = 0; ch < numChannels; ++ch) {
      auto *data = buffer.getWritePointer(ch);
      for (int s = 0; s < numSamples; ++s) {
        filterState[ch] += alpha * (data[s] - filterState[ch]);

        if (mode == Mode::LowPass) {
          data[s] = filterState[ch];
        } else {
          data[s] = data[s] - filterState[ch]; // HP = input - LP
        }
      }
    }
  }

  bool isActive() const { return mode != Mode::Off; }

private:
  double sampleRate = 44100.0;
  Mode mode = Mode::Off;
  std::array<float, 2> filterState{};
  float currentCutoff = 1000.0f;
};

/**
 * Bitcrush effect - reduces bit depth and optionally sample rate.
 */
class BitcrushFX {
public:
  BitcrushFX() = default;

  void setActive(bool active) { this->active = active; }
  void setBitDepth(int bits) { bitDepth = std::clamp(bits, 1, 16); }
  void setDownsample(int factor) {
    downsampleFactor = std::clamp(factor, 1, 16);
  }

  void process(juce::AudioBuffer<float> &buffer) {
    if (!active)
      return;

    float levels = std::pow(2.0f, static_cast<float>(bitDepth));
    int numChannels = buffer.getNumChannels();
    int numSamples = buffer.getNumSamples();

    for (int ch = 0; ch < numChannels; ++ch) {
      auto *data = buffer.getWritePointer(ch);
      for (int s = 0; s < numSamples; ++s) {
        // Downsample: hold previous sample
        if (s % downsampleFactor != 0) {
          data[s] = holdSample[ch];
        } else {
          // Quantize
          float sample = data[s];
          sample = std::round(sample * levels) / levels;
          data[s] = sample;
          holdSample[ch] = sample;
        }
      }
    }
  }

  bool isActive() const { return active; }

private:
  bool active = false;
  int bitDepth = 8;
  int downsampleFactor = 1;
  std::array<float, 2> holdSample{};
};

} // namespace djstih
