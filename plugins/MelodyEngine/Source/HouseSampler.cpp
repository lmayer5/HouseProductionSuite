#include "HouseSampler.h"

HouseSynthVoice::HouseSynthVoice() {
  adsr.setSampleRate(48000.0); // Will be updated in render if needed, or we can
                               // add a setCurrentPlaybackSampleRate method
}

bool HouseSynthVoice::canPlaySound(juce::SynthesiserSound *sound) {
  return dynamic_cast<HouseSynthSound *>(sound) != nullptr;
}

void HouseSynthVoice::startNote(int midiNoteNumber, float velocity,
                                juce::SynthesiserSound *sound,
                                int currentPitchWheelPosition) {
  if (auto *houseSound = dynamic_cast<HouseSynthSound *>(sound)) {
    auto midiFreq = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
    pitchRatio = midiFreq / houseSound->rootFrequency;
    currentSamplePos = 0.0;

    adsr.noteOn();
  }
}

void HouseSynthVoice::stopNote(float velocity, bool allowTailOff) {
  adsr.noteOff();

  if (!allowTailOff || !adsr.isActive())
    clearCurrentNote();
}

void HouseSynthVoice::updateADSR(float a, float d, float s, float r) {
  adsrParams.attack = a;
  adsrParams.decay = d;
  adsrParams.sustain = s;
  adsrParams.release = r;
  adsr.setParameters(adsrParams);
}

void HouseSynthVoice::renderNextBlock(juce::AudioBuffer<float> &outputBuffer,
                                      int startSample, int numSamples) {
  if (auto *houseSound =
          dynamic_cast<HouseSynthSound *>(getCurrentlyPlayingSound().get())) {
    adsr.setSampleRate(getSampleRate());

    auto &data = houseSound->sampleData;
    const float *inL = data.getReadPointer(0);
    const float *inR =
        data.getNumChannels() > 1 ? data.getReadPointer(1) : nullptr;

    float *outL = outputBuffer.getWritePointer(0, startSample);
    float *outR = outputBuffer.getNumChannels() > 1
                      ? outputBuffer.getWritePointer(1, startSample)
                      : nullptr;

    for (int i = 0; i < numSamples; ++i) {
      if (!adsr.isActive()) {
        clearCurrentNote();
        break;
      }

      auto posInt = (int)currentSamplePos;
      auto alpha = (float)(currentSamplePos - posInt);
      auto nextPos = posInt + 1;

      if (nextPos >= data.getNumSamples()) {
        stopNote(0.0f, false);
        break;
      }

      // Linear Interpolation
      float currentSampleL = inL[posInt];
      float nextSampleL = inL[nextPos];
      float interpL = currentSampleL + alpha * (nextSampleL - currentSampleL);

      float envelope = adsr.getNextSample();

      float valL = interpL * envelope;

      // Add to output
      outL[i] += valL;

      if (outR != nullptr) {
        float currentSampleR = inR ? inR[posInt] : currentSampleL;
        float nextSampleR = inR ? inR[nextPos] : nextSampleL;
        float interpR = currentSampleR + alpha * (nextSampleR - currentSampleR);
        float valR = interpR * envelope;
        outR[i] += valR;
      }

      currentSamplePos += pitchRatio;
    }
  }
}
