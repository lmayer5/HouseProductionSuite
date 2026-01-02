/*
  ==============================================================================

    TELookAndFeel.h
    Created: 02 Jan 2026
    Author:  DJstih

    Teenage Engineering-inspired Look and Feel for House Production Suite.
    High-contrast OLED aesthetic with minimal, flat vector controls.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

namespace TE {

// TE Color Palette
namespace Colors {
constexpr uint32_t OLEDBlack = 0xFF000000;
constexpr uint32_t SharpWhite = 0xFFFFFFFF;
constexpr uint32_t RhythmAccent = 0xFFFF4500;   // Neon Red/Orange
constexpr uint32_t BassAccent = 0xFF00BFFF;     // Neon Blue
constexpr uint32_t MelodyAccent = 0xFF39FF14;   // Neon Green
constexpr uint32_t GridLine = 0x33FFFFFF;       // 20% white
constexpr uint32_t GridLineBright = 0x66FFFFFF; // 40% white
} // namespace Colors

} // namespace TE

class TELookAndFeel : public juce::LookAndFeel_V4 {
public:
  enum class Accent { Rhythm, Bass, Melody };

  TELookAndFeel(Accent accent = Accent::Rhythm) {
    setAccent(accent);

    // Set default colors
    setColour(juce::ResizableWindow::backgroundColourId,
              juce::Colour(TE::Colors::OLEDBlack));
    setColour(juce::Label::textColourId, juce::Colours::white);
    setColour(juce::TextButton::buttonColourId,
              juce::Colour(TE::Colors::OLEDBlack));
    setColour(juce::TextButton::textColourOffId, juce::Colours::white);
  }

  void setAccent(Accent accent) {
    currentAccent = accent;
    switch (accent) {
    case Accent::Rhythm:
      accentColour = juce::Colour(TE::Colors::RhythmAccent);
      break;
    case Accent::Bass:
      accentColour = juce::Colour(TE::Colors::BassAccent);
      break;
    case Accent::Melody:
      accentColour = juce::Colour(TE::Colors::MelodyAccent);
      break;
    }
  }

  juce::Colour getAccentColour() const { return accentColour; }

  //============================================================================
  // ROTARY SLIDER - Minimal arc with sharp indicator
  //============================================================================
  void drawRotarySlider(juce::Graphics &g, int x, int y, int width, int height,
                        float sliderPosProportional, float rotaryStartAngle,
                        float rotaryEndAngle, juce::Slider &slider) override {
    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto centreX = bounds.getCentreX();
    auto centreY = bounds.getCentreY();
    auto rx = centreX - radius;
    auto ry = centreY - radius;
    auto rw = radius * 2.0f;
    auto angle = rotaryStartAngle +
                 sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    // Background arc (dark)
    juce::Path backgroundArc;
    backgroundArc.addCentredArc(centreX, centreY, radius * 0.8f, radius * 0.8f,
                                0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    g.strokePath(backgroundArc,
                 juce::PathStrokeType(3.0f, juce::PathStrokeType::curved,
                                      juce::PathStrokeType::rounded));

    // Value arc (accent color)
    juce::Path valueArc;
    valueArc.addCentredArc(centreX, centreY, radius * 0.8f, radius * 0.8f, 0.0f,
                           rotaryStartAngle, angle, true);
    g.setColour(accentColour);
    g.strokePath(valueArc,
                 juce::PathStrokeType(3.0f, juce::PathStrokeType::curved,
                                      juce::PathStrokeType::rounded));

    // Center dot
    g.setColour(juce::Colours::white);
    g.fillEllipse(centreX - 3.0f, centreY - 3.0f, 6.0f, 6.0f);

    // Pointer line
    juce::Path pointer;
    auto pointerLength = radius * 0.6f;
    auto pointerThickness = 2.0f;
    pointer.addRoundedRectangle(-pointerThickness * 0.5f, -pointerLength,
                                pointerThickness, pointerLength * 0.6f, 1.0f);
    pointer.applyTransform(
        juce::AffineTransform::rotation(angle).translated(centreX, centreY));
    g.setColour(juce::Colours::white);
    g.fillPath(pointer);
  }

  //============================================================================
  // TEXT BUTTON - Flat rectangle with 1px border
  //============================================================================
  void drawButtonBackground(juce::Graphics &g, juce::Button &button,
                            const juce::Colour &backgroundColour,
                            bool shouldDrawButtonAsHighlighted,
                            bool shouldDrawButtonAsDown) override {
    auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);

    if (button.getToggleState() || shouldDrawButtonAsDown) {
      // Active/pressed state - filled with accent
      g.setColour(accentColour);
      g.fillRect(bounds);
    } else if (shouldDrawButtonAsHighlighted) {
      // Hover state - subtle fill
      g.setColour(juce::Colours::white.withAlpha(0.1f));
      g.fillRect(bounds);
    }

    // Always draw border
    g.setColour(button.getToggleState() ? accentColour
                                        : juce::Colours::white.withAlpha(0.4f));
    g.drawRect(bounds, 1.0f);
  }

  void drawButtonText(juce::Graphics &g, juce::TextButton &button,
                      bool shouldDrawButtonAsHighlighted,
                      bool shouldDrawButtonAsDown) override {
    auto font = juce::Font("Consolas", 12.0f, juce::Font::bold);
    g.setFont(font);

    auto textColour = button.getToggleState() || shouldDrawButtonAsDown
                          ? juce::Colour(TE::Colors::OLEDBlack)
                          : juce::Colours::white;
    g.setColour(textColour);

    g.drawText(button.getButtonText(), button.getLocalBounds(),
               juce::Justification::centred);
  }

  //============================================================================
  // LABEL - Monospace white text
  //============================================================================
  juce::Font getLabelFont(juce::Label &label) override {
    return juce::Font("Consolas", 11.0f, juce::Font::plain);
  }

  //============================================================================
  // LINEAR SLIDER - Minimal track with accent fill
  //============================================================================
  void drawLinearSlider(juce::Graphics &g, int x, int y, int width, int height,
                        float sliderPos, float minSliderPos, float maxSliderPos,
                        const juce::Slider::SliderStyle style,
                        juce::Slider &slider) override {
    auto trackWidth = 4.0f;

    if (style == juce::Slider::LinearHorizontal ||
        style == juce::Slider::LinearBar) {
      auto trackY = y + height * 0.5f - trackWidth * 0.5f;

      // Background track
      g.setColour(juce::Colours::white.withAlpha(0.1f));
      g.fillRect(
          juce::Rectangle<float>((float)x, trackY, (float)width, trackWidth));

      // Value track
      g.setColour(accentColour);
      g.fillRect(
          juce::Rectangle<float>((float)x, trackY, sliderPos - x, trackWidth));

      // Thumb
      g.setColour(juce::Colours::white);
      g.fillRect(sliderPos - 2.0f, trackY - 4.0f, 4.0f, trackWidth + 8.0f);
    } else {
      // Vertical
      auto trackX = x + width * 0.5f - trackWidth * 0.5f;

      // Background track
      g.setColour(juce::Colours::white.withAlpha(0.1f));
      g.fillRect(
          juce::Rectangle<float>(trackX, (float)y, trackWidth, (float)height));

      // Value track
      g.setColour(accentColour);
      g.fillRect(juce::Rectangle<float>(trackX, sliderPos, trackWidth,
                                        y + height - sliderPos));

      // Thumb
      g.setColour(juce::Colours::white);
      g.fillRect(trackX - 4.0f, sliderPos - 2.0f, trackWidth + 8.0f, 4.0f);
    }
  }

private:
  Accent currentAccent = Accent::Rhythm;
  juce::Colour accentColour{TE::Colors::RhythmAccent};
};
