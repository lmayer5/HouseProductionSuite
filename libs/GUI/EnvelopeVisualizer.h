/*
  ==============================================================================

    EnvelopeVisualizer.h
    Created: 28 Dec 2025
    Author:  djstih

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class EnvelopeVisualizer : public juce::Component {
public:
  EnvelopeVisualizer() {}

  void setParameters(float a, float d, float s, float r) {
    attack = a;
    decay = d;
    sustain = s;
    release = r;
    repaint();
  }

  void paint(juce::Graphics &g) override {
    auto area = getLocalBounds().toFloat();
    auto w = area.getWidth();
    auto h = area.getHeight();

    g.setColour(juce::Colour(0xff222222));
    g.fillRoundedRectangle(area, 4.0f);

    // Normalize time parameters for visualization
    // Max range assumption: A=1s, D=1s, R=1s.
    // We'll give fixed proportion to S section for display

    // Total "Width" alloc:
    // 25% Attack, 25% Decay, 25% Sustain(Fixed), 25% Release
    // Scaled by their 0-1 param value relative to max?
    // Or just map direct logic.
    // Let's us Flex logic: A+D+R+S = 1.0 width

    float maxTime = 3.0f; // arbitrary max time for display scaling

    // We scale visual widths based on values, but clamp min visual so it's seen
    float aw = (attack / maxTime) * 0.8f + 0.05f;
    float dw = (decay / maxTime) * 0.8f + 0.05f;
    float rw = (release / maxTime) * 0.8f + 0.05f;
    float sw = 0.2f; // Fixed sustain visual width

    // Re-normalize to total width
    float total = aw + dw + rw + sw;
    aw = (aw / total) * w;
    dw = (dw / total) * w;
    sw = (sw / total) * w;
    rw = (rw / total) * w;

    juce::Path p;
    p.startNewSubPath(0.0f, h);

    // Attack (Up to peak)
    p.lineTo(aw, 0.0f);

    // Decay (Down to Sustain Level)
    float sustainY = h * (1.0f - sustain);
    p.lineTo(aw + dw, sustainY);

    // Sustain (Hold)
    p.lineTo(aw + dw + sw, sustainY);

    // Release (Down to zero)
    p.lineTo(aw + dw + sw + rw, h);

    // Draw fill
    g.setColour(juce::Colour(0xff00c8ff).withAlpha(0.2f));
    p.closeSubPath();
    g.fillPath(p);

    // Draw stroke
    g.setColour(juce::Colour(0xff00c8ff));
    g.strokePath(p, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved,
                                         juce::PathStrokeType::rounded));
  }

private:
  float attack = 0.01f;
  float decay = 0.1f;
  float sustain = 1.0f;
  float release = 0.1f;
};
