/*
  ==============================================================================

    PluginEditor.h
    Created: 29 Dec 2025
    Author:  DJstih

  ==============================================================================
*/

#pragma once

#include "../../../shared/TELookAndFeel.h"
#include "MelodyCanvasComponent.h"
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
  MelodyCanvasComponent melodyCanvas;

  // Controls
  juce::Slider attackSlider, decaySlider, morphSlider, cutoffSlider, resSlider,
      lfoRateSlider, lfoDepthSlider;
  juce::Label attackLabel, decayLabel, morphLabel, cutoffLabel, resLabel,
      lfoRateLabel, lfoDepthLabel;

  using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
  std::unique_ptr<SliderAttachment> attackAttachment, decayAttachment,
      morphAttachment, cutoffAttachment, resAttachment, lfoRateAttachment,
      lfoDepthAttachment;

  // TE-style LookAndFeel
  TELookAndFeel teLookAndFeel{TELookAndFeel::Accent::Melody};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MelodyEngineAudioProcessorEditor)
};
