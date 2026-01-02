#include "MelodyCanvasComponent.h"
#include "ScaleQuantizer.h"

MelodyCanvasComponent::MelodyCanvasComponent(MelodyEngineAudioProcessor &p)
    : processor(p) {
  processor.getGuiSnapshot(cachedPhrase);
  startTimer(16); // ~60fps
}

MelodyCanvasComponent::~MelodyCanvasComponent() { stopTimer(); }

void MelodyCanvasComponent::paint(juce::Graphics &g) {
  // TE-Style: Pure OLED Black background
  g.fillAll(juce::Colour(0xFF000000));

  // Update snapshot before painting (Safe Pull)
  processor.getGuiSnapshot(cachedPhrase);
  auto &phrase = cachedPhrase;
  auto intervals = quantizer.getScaleIntervals(phrase.scaleName);
  int notesPerOctave = (int)intervals.size();
  int totalRows = NUM_OCTAVES * notesPerOctave;

  float stepWidth = getStepWidth();
  float noteHeight = getNoteHeight();

  // TE-Style: Draw Grid Rows with sharp 1px white lines
  g.setColour(juce::Colours::white.withAlpha(0.1f));
  for (int i = 0; i <= totalRows; ++i) {
    float y = i * noteHeight;
    g.drawHorizontalLine((int)y, 0.0f, (float)getWidth());
  }

  // TE-Style: Draw Grid Columns with bar markers
  for (int i = 0; i <= NUM_STEPS; ++i) {
    float x = i * stepWidth;
    // Brighter lines on bar boundaries
    float alpha = (i % 16 == 0) ? 0.4f : ((i % 4 == 0) ? 0.2f : 0.08f);
    g.setColour(juce::Colours::white.withAlpha(alpha));
    g.drawVerticalLine((int)x, 0.0f, (float)getHeight());
  }

  // TE-Style: Neon Green for Melody notes
  g.setColour(juce::Colour(0xFF39FF14));

  for (int i = 0; i < phrase.events.size(); ++i) {
    auto &event = phrase.events[i];
    if (event.active) {
      // Calculate visual Y position
      // We need to reverse map frequency/pitch to our grid row
      int midiNote = (int)event.pitch;
      int octave = (midiNote / 12) - START_OCTAVE;
      int noteInOctave = midiNote % 12;
      int rootInOctave = phrase.rootNote % 12;

      // Re-center around root
      int relativeNote = (noteInOctave - rootInOctave + 12) % 12;

      // Find scale degree index (which interval is this?)
      int scaleIndex = -1;
      for (int k = 0; k < intervals.size(); ++k) {
        if (intervals[k] == relativeNote) {
          scaleIndex = k;
          break;
        }
      }
      // If note is not in scale (e.g. changed scale after painting), map to
      // nearest or ignore? For visualization, let's map to nearest or default
      // to 0.
      if (scaleIndex == -1)
        scaleIndex = 0;

      // Row 0 is HIGH pitch (top), so we invert
      // visualRow = (NUM_OCTAVES - 1 - octave) * notesPerOctave +
      // (notesPerOctave - 1 - scaleIndex)
      if (octave >= 0 && octave < NUM_OCTAVES) {
        int invertedRow = (NUM_OCTAVES - 1 - octave) * notesPerOctave +
                          (notesPerOctave - 1 - scaleIndex);

        float x = i * stepWidth;
        float y = invertedRow * noteHeight;
        float w = stepWidth * (event.duration / 0.25f); // Duration in 16ths

        g.fillRect(x + 1, y + 1, w - 2, noteHeight - 2);

        // TE-Style: Draw modifier icons on notes
        float iconSize = std::min(noteHeight * 0.4f, 6.0f);
        float iconX = x + w - iconSize - 3;
        float iconY = y + 3;
        g.setColour(juce::Colours::white.withAlpha(0.9f));

        switch (event.modifier) {
        case Melodic::NoteModifier::Ratchet2:
          g.fillEllipse(iconX, iconY, iconSize * 0.4f, iconSize * 0.4f);
          g.fillEllipse(iconX + iconSize * 0.5f, iconY, iconSize * 0.4f,
                        iconSize * 0.4f);
          break;
        case Melodic::NoteModifier::Ratchet4:
          g.fillEllipse(iconX, iconY, iconSize * 0.35f, iconSize * 0.35f);
          g.fillEllipse(iconX + iconSize * 0.4f, iconY, iconSize * 0.35f,
                        iconSize * 0.35f);
          g.fillEllipse(iconX, iconY + iconSize * 0.4f, iconSize * 0.35f,
                        iconSize * 0.35f);
          g.fillEllipse(iconX + iconSize * 0.4f, iconY + iconSize * 0.4f,
                        iconSize * 0.35f, iconSize * 0.35f);
          break;
        case Melodic::NoteModifier::SkipCycle:
          g.drawLine(iconX, iconY + iconSize, iconX + iconSize, iconY, 1.5f);
          break;
        case Melodic::NoteModifier::OnlyFirstCycle:
          g.setFont(juce::Font("Consolas", iconSize * 1.2f, juce::Font::bold));
          g.drawText("1", (int)iconX, (int)iconY, (int)iconSize, (int)iconSize,
                     juce::Justification::centred);
          break;
        default:
          break;
        }
        // Reset color for next note
        g.setColour(juce::Colour(0xFF39FF14));
      }
    }
  }

  // Draw Playhead
  int currentStep = processor.currentStepAtomic.load();
  float playheadX = currentStep * stepWidth;
  g.setColour(juce::Colour(0xFF00F3FF)); // Cyan
  g.drawLine(playheadX, 0, playheadX, (float)getHeight(), 2.0f);
}

void MelodyCanvasComponent::resized() {
  //
}

void MelodyCanvasComponent::timerCallback() { repaint(); }

void MelodyCanvasComponent::mouseDown(const juce::MouseEvent &event) {
  if (event.mods.isRightButtonDown()) {
    // Delete note
    int step = getStepAtX(event.x);
    if (step >= 0 && step < Melodic::NUM_PHRASE_STEPS) {
      // Create Command
      MelodyEngineAudioProcessor::MelodyCommand cmd;
      cmd.type = MelodyEngineAudioProcessor::MelodyCommand::SetEvent;
      cmd.stepIdx = step;
      cmd.eventData = cachedPhrase.events[step];
      cmd.eventData.active = false; // Disable

      processor.queueCommand(cmd);

      // Optimistic update (Local Cache)
      cachedPhrase.events[step].active = false;
    }
  } else {
    // Paint note
    mouseDrag(event);
  }
  repaint();
}

void MelodyCanvasComponent::mouseDrag(const juce::MouseEvent &event) {
  if (event.mods.isRightButtonDown())
    return;

  int step = getStepAtX(event.x);
  int y = event.y;

  if (step >= 0 && step < Melodic::NUM_PHRASE_STEPS) {
    auto &phrase = cachedPhrase; // Use local cache for context
    auto intervals = quantizer.getScaleIntervals(phrase.scaleName);
    int notesPerOctave = (int)intervals.size();

    int row = (int)(y / getNoteHeight());
    row = juce::jlimit(0, (NUM_OCTAVES * notesPerOctave) - 1, row);

    // Reverse row to octave/scale degree
    int invertedRow = (NUM_OCTAVES * notesPerOctave) - 1 - row;
    int octave = invertedRow / notesPerOctave; // 0 to 2 relative to start
    int scaleIndex = invertedRow % notesPerOctave;

    int interval = intervals[scaleIndex];
    int midiNote =
        (START_OCTAVE + octave) * 12 + (phrase.rootNote % 12) + interval;

    auto noteEvent = cachedPhrase.events[step]; // Copy from local cache
    noteEvent.active = true;
    noteEvent.pitch = (float)midiNote;
    noteEvent.velocity = 1.0f;
    noteEvent.duration = 0.25f; // Default length

    // Command
    MelodyEngineAudioProcessor::MelodyCommand cmd;
    cmd.type = MelodyEngineAudioProcessor::MelodyCommand::SetEvent;
    cmd.stepIdx = step;
    cmd.eventData = noteEvent;

    processor.queueCommand(cmd);

    // Optimistic Update (Local Cache)
    cachedPhrase.events[step] = noteEvent;
  }
  repaint();
}

void MelodyCanvasComponent::mouseUp(const juce::MouseEvent &event) {
  // Undo handling could go here
}

float MelodyCanvasComponent::getStepWidth() const {
  return (float)getWidth() / (float)NUM_STEPS;
}

float MelodyCanvasComponent::getNoteHeight() const {
  // Use 7 notes per octave for standard scales layout
  return (float)getHeight() / (float)(NUM_OCTAVES * 7);
}

int MelodyCanvasComponent::getStepAtX(int x) const {
  return (int)(x / getStepWidth());
}
