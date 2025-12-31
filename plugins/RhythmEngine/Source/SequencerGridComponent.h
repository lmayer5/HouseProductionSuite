#pragma once
#include "PluginProcessor.h"
#include "RhythmStructures.h"
#include <JuceHeader.h>

class SequencerGridComponent : public juce::Component, public juce::Timer {
public:
  SequencerGridComponent(RhythmEngineAudioProcessor &p);
  ~SequencerGridComponent() override;

  void paint(juce::Graphics &g) override;
  void resized() override;

  void mouseDown(const juce::MouseEvent &e) override;
  void mouseDrag(const juce::MouseEvent &e) override;

  void timerCallback() override;

private:
  void toggleStep(int x, int y, bool forceState = false,
                  bool stateToSet = true);
  juce::Rectangle<int> getStepBounds(int trackIdx, int stepIdx) const;

  RhythmEngineAudioProcessor &processor;

  // Playhead tracking
  double playheadPosition = 0.0; // In steps

  // Interaction tracking for 'painting'
  bool isPainting = true;
  int lastMouseTrack = -1;
  int lastMouseStep = -1;

  // Current pattern cache
  RhythmEngine::Pattern cachedPattern;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SequencerGridComponent)
};
