#include "SequencerGridComponent.h"

SequencerGridComponent::SequencerGridComponent(RhythmEngineAudioProcessor &p)
    : processor(p) {
  processor.getGuiSnapshot(cachedPattern);
  startTimerHz(60); // 60Hz update for playhead
}

SequencerGridComponent::~SequencerGridComponent() {}

void SequencerGridComponent::paint(juce::Graphics &g) {
  auto area = getLocalBounds();

  // Background - Dark grey/black dashboard look
  g.setColour(juce::Colour(0xFF0A0A0F));
  g.fillRoundedRectangle(area.toFloat(), 5.0f);

  // Draw Glass Overlay Effect
  juce::ColourGradient glassGradient(juce::Colours::white.withAlpha(0.05f), 0,
                                     0, juce::Colours::transparentWhite, 0,
                                     area.getHeight(), false);
  g.setGradientFill(glassGradient);
  g.fillRoundedRectangle(area.toFloat(), 5.0f);

  // Use the thread-safe snapshot for painting (Safe Pull)
  processor.getGuiSnapshot(cachedPattern);
  auto &pattern = cachedPattern;

  for (int t = 0; t < RhythmEngine::NUM_TRACKS; ++t) {
    for (int s = 0; s < RhythmEngine::NUM_STEPS; ++s) {
      auto r = getStepBounds(t, s).toFloat();
      bool isActive = pattern.tracks[t].steps[s].active;

      if (isActive) {
        float velocityAlpha = pattern.tracks[t].steps[s].velocity;

        // Neon Purple Glow - brightness based on velocity
        g.setColour(
            juce::Colour(0xFFCC00FF).withAlpha(0.6f * velocityAlpha + 0.2f));
        g.fillRoundedRectangle(r.reduced(1.0f), 2.0f);

        // Inner bright spot
        g.setColour(juce::Colours::white.withAlpha(0.4f * velocityAlpha));
        g.fillRoundedRectangle(r.reduced(3.0f), 1.0f);

        // Outer Glow effect
        g.setColour(juce::Colour(0xFFCC00FF).withAlpha(0.3f * velocityAlpha));
        g.drawRoundedRectangle(r.expanded(1.0f), 2.0f, 2.0f);
      } else {
        // Inactive state - dark outline
        g.setColour(juce::Colours::darkgrey.withAlpha(0.2f));
        g.drawRoundedRectangle(r.reduced(1.0f), 2.0f, 1.0f);
      }
    }
  }

  // Draw Playhead
  int currentStep = processor.currentStepAtomic.load();
  if (currentStep >= 0) {
    float playheadX = 0;
    // Calculate X based on first track bounds as reference
    auto firstStepPos = getStepBounds(0, 0).getX();
    auto lastStepPos = getStepBounds(0, RhythmEngine::NUM_STEPS - 1).getRight();
    float totalWidth = lastStepPos - firstStepPos;

    playheadX = firstStepPos +
                (currentStep / (float)RhythmEngine::NUM_STEPS) * totalWidth;

    g.setColour(juce::Colours::cyan.withAlpha(0.6f));
    g.drawVerticalLine(static_cast<int>(playheadX), area.getY() + 5.0f,
                       area.getBottom() - 5.0f);

    // Add a small glow to playhead
    g.setColour(juce::Colours::cyan.withAlpha(0.2f));
    g.fillRect(playheadX - 1.0f, (float)area.getY() + 5.0f, 3.0f,
               (float)area.getHeight() - 10.0f);
  }
}

void SequencerGridComponent::resized() {}

juce::Rectangle<int> SequencerGridComponent::getStepBounds(int trackIdx,
                                                           int stepIdx) const {
  auto area = getLocalBounds().reduced(5);
  int rowH = area.getHeight() / RhythmEngine::NUM_TRACKS;
  int colW = area.getWidth() / RhythmEngine::NUM_STEPS;

  return juce::Rectangle<int>(area.getX() + stepIdx * colW,
                              area.getY() + trackIdx * rowH, colW, rowH)
      .reduced(2);
}

void SequencerGridComponent::mouseDown(const juce::MouseEvent &e) {
  auto area = getLocalBounds().reduced(5);
  int trackIdx =
      (e.y - area.getY()) / (area.getHeight() / RhythmEngine::NUM_TRACKS);
  int stepIdx =
      (e.x - area.getX()) / (area.getWidth() / RhythmEngine::NUM_STEPS);

  if (trackIdx >= 0 && trackIdx < RhythmEngine::NUM_TRACKS && stepIdx >= 0 &&
      stepIdx < RhythmEngine::NUM_STEPS) {
    auto &pattern = cachedPattern;

    if (e.mods.isShiftDown()) {
      // Just adjust velocity of existing step
      isPainting = true; // Use it as a flag for "adjusting"
    } else {
      // Determine new state based on SNAPSHOT
      bool isNowActive = !cachedPattern.tracks[trackIdx].steps[stepIdx].active;

      // Create Command
      RhythmCommand cmd;
      cmd.type = RhythmCommand::ToggleStep;
      cmd.trackIdx = trackIdx;
      cmd.stepIdx = stepIdx;
      cmd.value = isNowActive ? 1.0f : 0.0f;

      // Send to Audio Thread
      processor.queueCommand(cmd);

      // Optimistic Update (for immediate UI response)
      cachedPattern.tracks[trackIdx].steps[stepIdx].active = isNowActive;
      if (isNowActive)
        cachedPattern.tracks[trackIdx].steps[stepIdx].velocity = 1.0f;

      isPainting = isNowActive; // Track state for drag
      lastMouseStep = stepIdx;
      lastMouseTrack = trackIdx;
    }
    repaint();
  }
}

void SequencerGridComponent::mouseDrag(const juce::MouseEvent &e) {
  auto area = getLocalBounds().reduced(5);
  int trackIdx =
      (e.y - area.getY()) / (area.getHeight() / RhythmEngine::NUM_TRACKS);
  int stepIdx =
      (e.x - area.getX()) / (area.getWidth() / RhythmEngine::NUM_STEPS);

  if (trackIdx >= 0 && trackIdx < RhythmEngine::NUM_TRACKS && stepIdx >= 0 &&
      stepIdx < RhythmEngine::NUM_STEPS) {
    if (e.mods.isShiftDown()) {
      // Adjust velocity based on mouse Y position
      float newVel =
          1.0f -
          (float)(e.y - area.getY() -
                  (trackIdx * (area.getHeight() / RhythmEngine::NUM_TRACKS))) /
              (area.getHeight() / RhythmEngine::NUM_TRACKS);

      // Send Command (Safe)
      RhythmCommand cmd;
      cmd.type = RhythmCommand::UpdateVelocity;
      cmd.trackIdx = trackIdx;
      cmd.stepIdx = stepIdx;
      cmd.value = std::clamp(newVel, 0.0f, 1.0f);
      processor.queueCommand(cmd);

      // Optimistic Update
      cachedPattern.tracks[trackIdx].steps[stepIdx].velocity = cmd.value;

      repaint();
    } else if (trackIdx != lastMouseTrack || stepIdx != lastMouseStep) {
      // Send Command
      RhythmCommand cmd;
      cmd.type = RhythmCommand::ToggleStep;
      cmd.trackIdx = trackIdx;
      cmd.stepIdx = stepIdx;
      cmd.value = isPainting ? 1.0f : 0.0f;
      processor.queueCommand(cmd);

      // Optimistic Update
      cachedPattern.tracks[trackIdx].steps[stepIdx].active = isPainting;
      if (isPainting)
        cachedPattern.tracks[trackIdx].steps[stepIdx].velocity = 1.0f;

      lastMouseStep = stepIdx;
      lastMouseTrack = trackIdx;
    }
  }
  repaint();
}

void SequencerGridComponent::timerCallback() { repaint(); }
