#include "SequencerGridComponent.h"

SequencerGridComponent::SequencerGridComponent(RhythmEngineAudioProcessor &p)
    : processor(p) {
  processor.getGuiSnapshot(cachedPattern);
  startTimerHz(60); // 60Hz update for playhead
}

SequencerGridComponent::~SequencerGridComponent() {}

void SequencerGridComponent::paint(juce::Graphics &g) {
  auto area = getLocalBounds();

  // TE-Style: Pure OLED Black background
  g.fillAll(juce::Colour(0xFF000000));

  // Use the thread-safe snapshot for painting (Safe Pull)
  processor.getGuiSnapshot(cachedPattern);
  auto &pattern = cachedPattern;

  // TE Color Palette - per-track neon accents
  const std::array<juce::Colour, RhythmEngine::NUM_TRACKS> trackColors = {
      juce::Colour(0xFFFF4500), // Kick - Neon Red/Orange
      juce::Colour(0xFF00BFFF), // Bass - Neon Blue
      juce::Colour(0xFF00FFFF), // Hat - Neon Cyan
      juce::Colour(0xFFFFFFFF)  // Clap - Pure White
  };

  // TE-Style: Check FX state for visual feedback
  bool fxActive = false;
  float stutterVal = 0.0f, sweepVal = 0.0f, crushVal = 0.0f;
  if (auto *p = processor.apvts.getRawParameterValue("FX_STUTTER"))
    stutterVal = p->load();
  if (auto *p = processor.apvts.getRawParameterValue("FX_SWEEP"))
    sweepVal = p->load();
  if (auto *p = processor.apvts.getRawParameterValue("FX_BITCRUSH"))
    crushVal = p->load();
  fxActive = (stutterVal > 0.5f) || (sweepVal > 0.01f) || (crushVal > 0.5f);

  // TE-Style: Draw FX active border/glow when effects are engaged
  if (fxActive) {
    g.setColour(juce::Colour(0xFFFF4500).withAlpha(0.15f));
    g.fillAll();
    g.setColour(juce::Colour(0xFFFF4500).withAlpha(0.8f));
    g.drawRect(area, 3);
  }

  for (int t = 0; t < RhythmEngine::NUM_TRACKS; ++t) {
    for (int s = 0; s < RhythmEngine::NUM_STEPS; ++s) {
      auto r = getStepBounds(t, s).toFloat();
      bool isActive = pattern.tracks[t].steps[s].active;
      auto modifier = pattern.tracks[t].steps[s].modifier;

      if (isActive) {
        float velocityAlpha = pattern.tracks[t].steps[s].velocity;
        auto trackColor = trackColors[t];

        // TE-Style: Solid fill with velocity-based brightness
        g.setColour(trackColor.withAlpha(0.3f + 0.7f * velocityAlpha));
        g.fillRect(r.reduced(1.0f));

        // Draw modifier icons if present
        if (modifier != RhythmEngine::StepModifier::None) {
          g.setColour(juce::Colours::white.withAlpha(0.9f));
          auto iconArea = r.reduced(4.0f);

          switch (modifier) {
          case RhythmEngine::StepModifier::Ratchet2:
            // Draw "2x" symbol - two dots
            g.fillEllipse(iconArea.getX() + 2, iconArea.getY() + 2, 4, 4);
            g.fillEllipse(iconArea.getX() + 8, iconArea.getY() + 2, 4, 4);
            break;
          case RhythmEngine::StepModifier::Ratchet4:
            // Draw "4x" symbol - four dots in grid
            g.fillEllipse(iconArea.getX() + 2, iconArea.getY() + 2, 3, 3);
            g.fillEllipse(iconArea.getX() + 7, iconArea.getY() + 2, 3, 3);
            g.fillEllipse(iconArea.getX() + 2, iconArea.getY() + 7, 3, 3);
            g.fillEllipse(iconArea.getX() + 7, iconArea.getY() + 7, 3, 3);
            break;
          case RhythmEngine::StepModifier::SkipCycle:
            // Draw skip symbol - diagonal line
            g.drawLine(iconArea.getX(), iconArea.getBottom(),
                       iconArea.getRight(), iconArea.getY(), 2.0f);
            break;
          case RhythmEngine::StepModifier::OnlyFirstCycle:
            // Draw "1" symbol
            g.setFont(10.0f);
            g.drawText("1", iconArea, juce::Justification::centred);
            break;
          case RhythmEngine::StepModifier::Glide:
            // Draw ramp symbol
            g.drawLine(iconArea.getX(), iconArea.getBottom(),
                       iconArea.getRight(), iconArea.getY(), 1.5f);
            break;
          default:
            break;
          }
        }
      }

      // TE-Style: Sharp 1px white outline for all steps
      g.setColour(juce::Colours::white.withAlpha(isActive ? 0.8f : 0.15f));
      g.drawRect(r.reduced(0.5f), 1.0f);
    }
  }

  // Draw Playhead - TE-Style: Bright cyan line
  int currentStep = processor.currentStepAtomic.load();
  if (currentStep >= 0) {
    auto firstStepPos = getStepBounds(0, 0).getX();
    auto lastStepPos = getStepBounds(0, RhythmEngine::NUM_STEPS - 1).getRight();
    float totalWidth = lastStepPos - firstStepPos;

    float playheadX =
        firstStepPos +
        (currentStep / (float)RhythmEngine::NUM_STEPS) * totalWidth;

    // TE-Style: Clean bright playhead
    g.setColour(juce::Colour(0xFF00F3FF)); // Bright cyan
    g.fillRect(playheadX - 1.0f, (float)area.getY() + 2.0f, 2.0f,
               (float)area.getHeight() - 4.0f);
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
