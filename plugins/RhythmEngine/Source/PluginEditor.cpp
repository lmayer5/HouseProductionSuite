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
    : AudioProcessorEditor(&p), audioProcessor(p), sequencerGrid(p) {

  addAndMakeVisible(sequencerGrid);

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

  setSize(800, 600); // Larger Dashboard UI
}

RhythmEngineAudioProcessorEditor::~RhythmEngineAudioProcessorEditor() {}

//==============================================================================
void RhythmEngineAudioProcessorEditor::paint(juce::Graphics &g) {
  // Main background - deep space grey
  g.fillAll(juce::Colour(0xFF0F0F14));

  auto area = getLocalBounds();

  // Header Title
  auto headerArea = area.removeFromTop(50);
  g.setColour(juce::Colours::white);
  g.setFont(juce::Font("Outfit", 24.0f, juce::Font::bold));
  g.drawText("RHYTHM ENGINE", headerArea.reduced(20, 0),
             juce::Justification::left);

  g.setColour(juce::Colour(0xFF00FFFF).withAlpha(0.5f));
  g.setFont(14.0f);
  g.drawText("// STEP SEQUENCER v1.0", headerArea.reduced(20, 0),
             juce::Justification::right);

  // Background for knob row
  auto knobArea = getLocalBounds().removeFromBottom(200).reduced(10);
  g.setColour(juce::Colours::black.withAlpha(0.3f));
  g.fillRoundedRectangle(knobArea.toFloat(), 10.0f);
  g.setColour(juce::Colours::white.withAlpha(0.1f));
  g.drawRoundedRectangle(knobArea.toFloat(), 10.0f, 1.0f);
}

void RhythmEngineAudioProcessorEditor::resized() {
  auto area = getLocalBounds();

  // Header
  area.removeFromTop(50);

  // Grid in the middle
  auto gridArea = area.removeFromTop(350).reduced(20);
  sequencerGrid.setBounds(gridArea);

  // Knob Row at the bottom
  auto knobRow = area.reduced(20, 10);
  int knobW = knobRow.getWidth() / 7;

  kickFreqSlider.setBounds(knobRow.removeFromLeft(knobW).reduced(5));
  kickDecaySlider.setBounds(knobRow.removeFromLeft(knobW).reduced(5));
  bassCutoffSlider.setBounds(knobRow.removeFromLeft(knobW).reduced(5));
  bassDriveSlider.setBounds(knobRow.removeFromLeft(knobW).reduced(5));
  bassAttackSlider.setBounds(knobRow.removeFromLeft(knobW).reduced(5));
  bassDecaySlider.setBounds(knobRow.removeFromLeft(knobW).reduced(5));
  sidechainSlider.setBounds(knobRow.removeFromLeft(knobW).reduced(5));
}
