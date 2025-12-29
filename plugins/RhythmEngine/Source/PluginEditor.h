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

class RhythmEngineAudioProcessorEditor : public juce::AudioProcessorEditor {
public:
  RhythmEngineAudioProcessorEditor(RhythmEngineAudioProcessor &);
  ~RhythmEngineAudioProcessorEditor() override;

  void paint(juce::Graphics &) override;
  void resized() override;

private:
  RhythmEngineAudioProcessor &audioProcessor;

  // Sliders
  juce::Slider kickFreqSlider;
  juce::Slider kickDecaySlider;
  juce::Slider bassCutoffSlider;
  juce::Slider bassDriveSlider;
  juce::Slider bassAttackSlider;
  juce::Slider bassDecaySlider;
  juce::Slider sidechainSlider;

  // Labels
  juce::Label kickFreqLabel;
  juce::Label kickDecayLabel;
  juce::Label bassCutoffLabel;
  juce::Label bassDriveLabel;
  juce::Label bassAttackLabel;
  juce::Label bassDecayLabel;
  juce::Label sidechainLabel;

  // Attachments
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      kickFreqAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      kickDecayAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      bassCutoffAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      bassDriveAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      bassAttackAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      bassDecayAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      sidechainAttachment;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RhythmEngineAudioProcessorEditor)
};
