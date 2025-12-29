/*
  ==============================================================================

    FlatLookAndFeel.h
    Created: 28 Dec 2025
    Author:  djstih

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class FlatLookAndFeel : public juce::LookAndFeel_V4 {
public:
  FlatLookAndFeel() {
    setColour(juce::Slider::thumbColourId, juce::Colours::white);
    setColour(juce::Slider::rotarySliderFillColourId,
              juce::Colour(0xff00c8ff)); // Blue accent
    setColour(juce::Slider::rotarySliderOutlineColourId,
              juce::Colour(0xff2d2d2d)); // Dark grey
  }

  void drawRotarySlider(juce::Graphics &g, int x, int y, int width, int height,
                        float sliderPos, float startAngle, float endAngle,
                        juce::Slider &slider) override {
    auto bounds =
        juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height)
            .reduced(10.0f);
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto toCentre = bounds.getCentre();
    auto centreX = toCentre.getX();
    auto centreY = toCentre.getY();
    auto px = centreX;
    auto py = centreY;

    // Draw Background Dial
    g.setColour(findColour(juce::Slider::rotarySliderOutlineColourId));
    g.fillEllipse(centreX - radius, centreY - radius, radius * 2.0f,
                  radius * 2.0f);

    // Draw Value Arc
    juce::Path arcPath;
    auto arcRadius = radius * 0.8f;
    auto angle = startAngle + sliderPos * (endAngle - startAngle);

    // If slider is symmetric (like pan), we might want different drawing, but
    // standard 0-1 is fine here.

    arcPath.addCentredArc(centreX, centreY, arcRadius, arcRadius, 0.0f,
                          startAngle, angle, true);

    g.setColour(findColour(juce::Slider::rotarySliderFillColourId));
    g.strokePath(arcPath,
                 juce::PathStrokeType(4.0f, juce::PathStrokeType::curved,
                                      juce::PathStrokeType::rounded));

    // Draw Indicator / Thumb (optional, sticking to minimal arc + dot)
    // Dot at current angle
    /*
    auto thumbRadius = radius * 0.8f;
    auto thumbX = centreX + thumbRadius * std::cos(angle -
    juce::MathConstants<float>::halfPi); auto thumbY = centreY + thumbRadius *
    std::sin(angle - juce::MathConstants<float>::halfPi);
    */

    // Center text is usually handled by the slider itself if layout allows,
    // but here we just draw simple.
  }
};
