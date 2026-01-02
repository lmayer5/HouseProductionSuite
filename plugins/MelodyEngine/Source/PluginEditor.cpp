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
MelodyEngineAudioProcessorEditor::MelodyEngineAudioProcessorEditor(
    MelodyEngineAudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p), melodyCanvas(p) {

  // Apply TE-style LookAndFeel
  setLookAndFeel(&teLookAndFeel);

  addAndMakeVisible(melodyCanvas);

  // Helper to setup slider
  auto setupSlider = [this](juce::Slider &slider, juce::Label &label,
                            const juce::String &name) {
    addAndMakeVisible(slider);
    addAndMakeVisible(label);
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    label.setText(name, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.attachToComponent(&slider, false);
  };

  setupSlider(attackSlider, attackLabel, "Attack");
  setupSlider(decaySlider, decayLabel, "Decay");
  setupSlider(morphSlider, morphLabel, "Morph");
  setupSlider(cutoffSlider, cutoffLabel, "Cutoff");
  setupSlider(resSlider, resLabel, "Res");
  setupSlider(lfoRateSlider, lfoRateLabel, "LFO Rate");
  setupSlider(lfoDepthSlider, lfoDepthLabel, "LFO Depth");

  // Attachments
  using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
  attackAttachment = std::make_unique<Attachment>(audioProcessor.apvts,
                                                  "ATTACK", attackSlider);
  decayAttachment =
      std::make_unique<Attachment>(audioProcessor.apvts, "DECAY", decaySlider);
  morphAttachment =
      std::make_unique<Attachment>(audioProcessor.apvts, "MORPH", morphSlider);
  cutoffAttachment = std::make_unique<Attachment>(audioProcessor.apvts,
                                                  "CUTOFF", cutoffSlider);
  resAttachment = std::make_unique<Attachment>(audioProcessor.apvts,
                                               "RESONANCE", resSlider);
  lfoRateAttachment = std::make_unique<Attachment>(audioProcessor.apvts,
                                                   "LFO_RATE", lfoRateSlider);
  lfoDepthAttachment = std::make_unique<Attachment>(
      audioProcessor.apvts, "LFO_DEPTH", lfoDepthSlider);

  setSize(800, 600);
  setResizable(true, true);
}

MelodyEngineAudioProcessorEditor::~MelodyEngineAudioProcessorEditor() {
  setLookAndFeel(nullptr); // Clean up to avoid dangling pointer
}

//==============================================================================
void MelodyEngineAudioProcessorEditor::paint(juce::Graphics &g) {
  // TE-Style: Pure OLED Black background
  g.fillAll(juce::Colour(0xFF000000));

  // TE-Style: Header text
  g.setColour(juce::Colours::white);
  g.setFont(juce::Font("Consolas", 14.0f, juce::Font::bold));
  g.drawText("MELODY ENGINE", getLocalBounds().removeFromTop(30).reduced(10, 0),
             juce::Justification::left);

  // Neon green version tag
  g.setColour(juce::Colour(0xFF39FF14));
  g.setFont(juce::Font("Consolas", 10.0f, juce::Font::plain));
  g.drawText("// TE-SYNTH v2.0",
             getLocalBounds().removeFromTop(30).reduced(10, 0),
             juce::Justification::right);
}

void MelodyEngineAudioProcessorEditor::resized() {
  auto area = getLocalBounds();

  // Sidebar for controls (right side)
  auto sidebar = area.removeFromRight(150);

  // Layout sliders in sidebar
  int sliderHeight = 80;
  attackSlider.setBounds(sidebar.removeFromTop(sliderHeight));
  decaySlider.setBounds(sidebar.removeFromTop(sliderHeight));
  morphSlider.setBounds(sidebar.removeFromTop(sliderHeight));
  cutoffSlider.setBounds(sidebar.removeFromTop(sliderHeight));
  resSlider.setBounds(sidebar.removeFromTop(sliderHeight));
  lfoRateSlider.setBounds(sidebar.removeFromTop(sliderHeight));
  lfoDepthSlider.setBounds(sidebar.removeFromTop(sliderHeight));

  // Remaining area for canvas
  melodyCanvas.setBounds(area.reduced(10));
}
