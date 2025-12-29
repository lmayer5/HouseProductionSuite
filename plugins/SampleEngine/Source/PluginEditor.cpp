#include "PluginEditor.h"
#include "PluginProcessor.h"

SampleEngineAudioProcessorEditor::SampleEngineAudioProcessorEditor(
    SampleEngineAudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p) {
  setSize(400, 300);
}

SampleEngineAudioProcessorEditor::~SampleEngineAudioProcessorEditor() {}

void SampleEngineAudioProcessorEditor::paint(juce::Graphics &g) {
  g.fillAll(juce::Colours::black);
  g.setColour(juce::Colours::white);
  g.setFont(15.0f);
  g.drawFittedText("Sample Engine", getLocalBounds(),
                   juce::Justification::centred, 1);
}

void SampleEngineAudioProcessorEditor::resized() {}
