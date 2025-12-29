/*
  ==============================================================================

    PluginEditor.h
    Created: 29 Dec 2025
    Author:  DJstih

  ==============================================================================
*/

#pragma once

#include "PluginProcessor.h"
#include <JuceHeader.h>


//==============================================================================
/**
 */
class MelodyEngineAudioProcessorEditor : public juce::AudioProcessorEditor {
public:
  MelodyEngineAudioProcessorEditor(MelodyEngineAudioProcessor &);
  ~MelodyEngineAudioProcessorEditor() override;

  //==============================================================================
  void paint(juce::Graphics &) override;
  void resized() override;

private:
  // This reference is provided as a quick way for your editor to
  // access the processor object that created it.
  MelodyEngineAudioProcessor &audioProcessor;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MelodyEngineAudioProcessorEditor)
};
