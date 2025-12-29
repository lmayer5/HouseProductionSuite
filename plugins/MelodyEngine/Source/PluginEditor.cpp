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
    : AudioProcessorEditor(&p), audioProcessor(p) {
  // Make sure that before the constructor has finished, you've set the
  // editor's size to whatever you need it to be.
  setSize(400, 300);
}

MelodyEngineAudioProcessorEditor::~MelodyEngineAudioProcessorEditor() {}

//==============================================================================
void MelodyEngineAudioProcessorEditor::paint(juce::Graphics &g) {
  // (Our component is opaque, so we must completely fill the background with a
  // solid colour)
  g.fillAll(
      getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

  g.setColour(juce::Colours::white);
  g.setFont(15.0f);
  g.drawFittedText("Melody Engine", getLocalBounds(),
                   juce::Justification::centred, 1);
}

void MelodyEngineAudioProcessorEditor::resized() {
  // This is generally where you'll want to lay out the positions of any
  // subcomponents in your editor..
}
