/*
  ==============================================================================

    Parameters.h
    Created: 28 Dec 2025
    Author:  djstih

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

namespace Parameters {
// Kick ID
static const juce::String kickPitch = "kick_pitch";
static const juce::String kickDecay = "kick_decay";
static const juce::String kickClick = "kick_click";

// Bass ID
static const juce::String bassCutoff = "bass_cutoff";
static const juce::String bassRes = "bass_res";
static const juce::String bassEnvAmt = "bass_env_amount";

// Global
static const juce::String sidechainDepth = "sidechain_depth";

inline juce::AudioProcessorValueTreeState::ParameterLayout
createParameterLayout() {
  std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

  // Kick
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      kickPitch, "Kick Pitch", 20.0f, 100.0f, 50.0f));

  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      kickDecay, "Kick Decay", 0.01f, 1.0f, 0.3f));

  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      kickClick, "Kick Click", 0.0f, 1.0f, 0.5f));

  // Bass
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      bassCutoff, "Bass Cutoff",
      juce::NormalisableRange<float>(50.0f, 2000.0f, 1.0f, 0.5f),
      500.0f)); // Skewed 0.5 for filter sweep feel

  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      bassRes, "Bass Res", 0.1f, 10.0f, 1.0f));

  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      bassEnvAmt, "Bass Env Amt", 0.0f, 1.0f, 0.5f));

  // Sidechain
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      sidechainDepth, "Sidechain Depth", 0.0f, 1.0f, 0.8f));

  return {params.begin(), params.end()};
}
} // namespace Parameters
