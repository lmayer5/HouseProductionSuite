#pragma once
#include "PluginProcessor.h"
#include "ScaleQuantizer.h"
#include <JuceHeader.h>

class MelodyCanvasComponent : public juce::Component, public juce::Timer {
public:
  MelodyCanvasComponent(MelodyEngineAudioProcessor &p);
  ~MelodyCanvasComponent() override;

  void paint(juce::Graphics &g) override;
  void resized() override;
  void timerCallback() override;

  void mouseDown(const juce::MouseEvent &event) override;
  void mouseDrag(const juce::MouseEvent &event) override;
  void mouseUp(const juce::MouseEvent &event) override;

private:
  MelodyEngineAudioProcessor &processor;

  // Constants
  const int NUM_STEPS = 64;
  const int NUM_OCTAVES = 3;
  const int START_OCTAVE = 1; // C1 to C3

  // Helpers
  float getStepWidth() const;
  float getNoteHeight() const;
  int getStepAtX(int x) const;
  int getPitchAtY(int y) const;

  // Note: The Scale logic means Y maps to Scale Degrees, not chromatic notes.
  // We need to map visual rows to scale degrees (1-7 per octave).
  // Total rows = NUM_OCTAVES * 7.

  int getScaleDegreeAtY(int y) const;
  float getYForScaleDegree(int degree,
                           int octave) const; // 0-indexed degree (0-6)

  // Current Scale context cache (to avoid repeated lookups if we wanted
  // optimization) For now we will pull from processor on each paint/click
  Melodic::Phrase cachedPhrase;
  ScaleQuantizer quantizer; // Cached to avoid allocations in paint()

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MelodyCanvasComponent)
};
