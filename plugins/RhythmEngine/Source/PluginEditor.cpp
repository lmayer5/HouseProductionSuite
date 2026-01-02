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

  // Apply TE-style LookAndFeel
  setLookAndFeel(&teLookAndFeel);

  addAndMakeVisible(sequencerGrid);

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

  // TE-style Punch-In FX Buttons
  auto setupFxButton = [&](juce::TextButton &btn, const juce::String &paramId) {
    btn.setClickingTogglesState(true);
    addAndMakeVisible(btn);

    // Manual binding since JUCE doesn't have direct ButtonAttachment for floats
    btn.onClick = [&, paramId]() {
      if (auto *param = audioProcessor.apvts.getRawParameterValue(paramId)) {
        param->store(btn.getToggleState() ? 1.0f : 0.0f);
      }
    };
  };

  setupFxButton(fxStutterBtn, "FX_STUTTER");
  setupFxButton(fxSweepBtn, "FX_SWEEP");
  setupFxButton(fxBitcrushBtn, "FX_BITCRUSH");

  setSize(800, 650); // Taller to fit FX row
}

RhythmEngineAudioProcessorEditor::~RhythmEngineAudioProcessorEditor() {
  setLookAndFeel(nullptr); // Clean up to avoid dangling pointer
}

//==============================================================================
void RhythmEngineAudioProcessorEditor::paint(juce::Graphics &g) {
  // TE-Style: Pure OLED Black background
  g.fillAll(juce::Colour(0xFF000000));

  auto area = getLocalBounds();

  // Header area with TE-style typography
  auto headerArea = area.removeFromTop(50);

  // Title - Sharp white text
  g.setColour(juce::Colours::white);
  g.setFont(juce::Font("Consolas", 22.0f, juce::Font::bold));
  g.drawText("RHYTHM ENGINE", headerArea.reduced(20, 0),
             juce::Justification::left);

  // Version tag - Neon red/orange accent
  g.setColour(juce::Colour(0xFFFF4500));
  g.setFont(juce::Font("Consolas", 12.0f, juce::Font::plain));
  g.drawText("// TE-STEP v2.0", headerArea.reduced(20, 0),
             juce::Justification::right);

  // Thin separator line
  g.setColour(juce::Colours::white.withAlpha(0.2f));
  g.drawHorizontalLine(headerArea.getBottom(), 20.0f,
                       (float)getWidth() - 20.0f);

  // Control area label
  auto knobArea = getLocalBounds().removeFromBottom(200);
  g.setColour(juce::Colours::white.withAlpha(0.6f));
  g.setFont(juce::Font("Consolas", 10.0f, juce::Font::plain));
  g.drawText("DSP PARAMETERS", knobArea.removeFromTop(20).reduced(25, 0),
             juce::Justification::left);
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

  // TE-style FX Button Row at the very bottom
  auto fxRow = getLocalBounds().removeFromBottom(45).reduced(20, 5);
  int fxBtnW = fxRow.getWidth() / 3;
  fxStutterBtn.setBounds(fxRow.removeFromLeft(fxBtnW).reduced(5));
  fxSweepBtn.setBounds(fxRow.removeFromLeft(fxBtnW).reduced(5));
  fxBitcrushBtn.setBounds(fxRow.removeFromLeft(fxBtnW).reduced(5));
}
