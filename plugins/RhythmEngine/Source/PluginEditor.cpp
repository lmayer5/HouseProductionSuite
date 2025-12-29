/*
  ==============================================================================

    PluginEditor.cpp
    Created: 29 Dec 2025
    Author:  DJstih

  ==============================================================================
*/

#include "PluginEditor.h"
#include "PluginProcessor.h"

//==============================================================================
RhythmEngineAudioProcessorEditor::RhythmEngineAudioProcessorEditor(
    RhythmEngineAudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p) {

  // Colors and Look
  getLookAndFeel().setColour(juce::Slider::thumbColourId, juce::Colours::cyan);
  getLookAndFeel().setColour(juce::Slider::rotarySliderFillColourId,
                             juce::Colours::cyan.withAlpha(0.5f));
  getLookAndFeel().setColour(juce::Slider::rotarySliderOutlineColourId,
                             juce::Colours::black);

  auto setupControl =
      [&](juce::Slider &slider, juce::Label &label,
          std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
              &attachment,
          const juce::String &paramId, const juce::String &labelText) {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
        addAndMakeVisible(slider);

        label.setText(labelText, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.attachToComponent(&slider, false); // Attach to top
        // We will ensure enough vertical space so this doesn't overlap
        // But actually, explicit placement is often safer. Let's stick to
        // attach for now but add Padding in resized.
        addAndMakeVisible(label);

        attachment = std::make_unique<
            juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.apvts, paramId, slider);
      };

  setupControl(kickFreqSlider, kickFreqLabel, kickFreqAttachment, "KICK_FREQ",
               "Kick Freq");
  setupControl(kickDecaySlider, kickDecayLabel, kickDecayAttachment,
               "KICK_DECAY", "Kick Decay");

  setupControl(bassCutoffSlider, bassCutoffLabel, bassCutoffAttachment,
               "BASS_CUTOFF", "Cutoff");
  setupControl(bassDriveSlider, bassDriveLabel, bassDriveAttachment,
               "BASS_DRIVE", "Drive");
  setupControl(bassAttackSlider, bassAttackLabel, bassAttackAttachment,
               "BASS_ATTACK", "Attack");
  setupControl(bassDecaySlider, bassDecayLabel, bassDecayAttachment,
               "BASS_DECAY", "Decay");
  setupControl(sidechainSlider, sidechainLabel, sidechainAttachment,
               "SIDECHAIN_AMT", "Sidechain");

  setSize(500, 400); // Taller UI
}

RhythmEngineAudioProcessorEditor::~RhythmEngineAudioProcessorEditor() {}

//==============================================================================
void RhythmEngineAudioProcessorEditor::paint(juce::Graphics &g) {
  g.fillAll(juce::Colours::black); // Clean black background

  auto area = getLocalBounds();

  // Header
  auto headerArea = area.removeFromTop(40);
  g.setColour(juce::Colours::white);
  g.setFont(20.0f);
  g.drawText("RHYTHM ENGINE", headerArea, juce::Justification::centred);

  // Backgrounds for sections
  auto kickArea = area.removeFromTop(130).reduced(10);
  auto bassArea = area.removeFromTop(180).reduced(10); // More height for bass

  g.setColour(juce::Colours::darkgrey.withAlpha(0.3f));
  g.fillRoundedRectangle(kickArea.toFloat(), 10.0f);
  g.fillRoundedRectangle(bassArea.toFloat(), 10.0f);

  // Section Titles
  g.setColour(juce::Colours::cyan);
  g.setFont(14.0f);
  g.drawText("KICK", kickArea.removeFromTop(20),
             juce::Justification::centredTop);
  g.drawText("BASS", bassArea.removeFromTop(20),
             juce::Justification::centredTop);
}

void RhythmEngineAudioProcessorEditor::resized() {
  auto area = getLocalBounds();
  auto headerH = 40;
  area.removeFromTop(headerH);

  // Kick Section
  auto kickRow = area.removeFromTop(130).reduced(10);
  kickRow.removeFromTop(20); // Title space
  // Push down slightly to allow for attached labels
  kickRow.removeFromTop(20);

  int knobW = 90;
  // Center the 2 kick knobs
  auto kickCenter = kickRow.reduced((kickRow.getWidth() - (knobW * 2)) / 2, 0);
  kickFreqSlider.setBounds(kickCenter.removeFromLeft(knobW));
  kickCenter.removeFromLeft(10); // gap
  kickDecaySlider.setBounds(kickCenter.removeFromLeft(knobW));

  // Bass Section
  auto bassRow = area.removeFromTop(180).reduced(10);
  bassRow.removeFromTop(20); // Title space
  bassRow.removeFromTop(20); // Label clearance

  // 5 Knobs
  // We can just flex them or split row
  int bassKnobW = bassRow.getWidth() / 5;

  bassCutoffSlider.setBounds(bassRow.removeFromLeft(bassKnobW).reduced(2));
  bassDriveSlider.setBounds(bassRow.removeFromLeft(bassKnobW).reduced(2));
  bassAttackSlider.setBounds(bassRow.removeFromLeft(bassKnobW).reduced(2));
  bassDecaySlider.setBounds(bassRow.removeFromLeft(bassKnobW).reduced(2));
  sidechainSlider.setBounds(bassRow.removeFromLeft(bassKnobW).reduced(2));
}
