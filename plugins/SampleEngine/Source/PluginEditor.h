#pragma once

#include "PluginProcessor.h"
#include <JuceHeader.h>


class SampleEngineAudioProcessorEditor : public juce::AudioProcessorEditor {
public:
  SampleEngineAudioProcessorEditor(SampleEngineAudioProcessor &);
  ~SampleEngineAudioProcessorEditor() override;

  void paint(juce::Graphics &) override;
  void resized() override;

private:
  SampleEngineAudioProcessor &audioProcessor;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleEngineAudioProcessorEditor)
};
