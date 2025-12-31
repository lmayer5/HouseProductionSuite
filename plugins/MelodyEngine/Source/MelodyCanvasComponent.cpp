#include "MelodyCanvasComponent.h"
#include "ScaleQuantizer.h"

MelodyCanvasComponent::MelodyCanvasComponent(MelodyEngineAudioProcessor &p)
    : processor(p) {
  processor.getGuiSnapshot(cachedPhrase);
  startTimer(16); // ~60fps
}

MelodyCanvasComponent::~MelodyCanvasComponent() { stopTimer(); }

void MelodyCanvasComponent::paint(juce::Graphics &g) {
  g.fillAll(juce::Colour(0xFF0A0A0F)); // Cyberpunk Dark

  // Update snapshot before painting (Safe Pull)
  processor.getGuiSnapshot(cachedPhrase);
  auto &phrase = cachedPhrase;
  ScaleQuantizer quantizer; // Helper for interval lookup
  auto intervals = quantizer.getScaleIntervals(phrase.scaleName);
  int notesPerOctave = (int)intervals.size();
  int totalRows = NUM_OCTAVES * notesPerOctave;

  float stepWidth = getStepWidth();
  float noteHeight = getNoteHeight();

  // Draw Grid Rows (Scale Degrees)
  g.setColour(juce::Colour(0xFF2A2A35));
  for (int i = 0; i <= totalRows; ++i) {
    float y = i * noteHeight;
    g.drawHorizontalLine((int)y, 0.0f, (float)getWidth());
  }

  // Draw Grid Columns (Time Steps)
  for (int i = 0; i <= NUM_STEPS; ++i) {
    float x = i * stepWidth;
    g.setColour((i % 4 == 0) ? juce::Colour(0xFF3A3A45)
                             : juce::Colour(0xFF1A1A25));
    g.drawVerticalLine((int)x, 0.0f, (float)getHeight());
  }

  // Draw Notes
  g.setColour(juce::Colour(0xFFCC00FF)); // Neon Purple

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
    ScaleQuantizer quantizer;
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
  ScaleQuantizer q; // Temp
  // Assuming minor/7 notes for now to calculate height roughly
  // Better to get it dynamically in paint but for helper we strictly need
  // context. Let's assume 7 notes per octave for standard scales
  return (float)getHeight() / (float)(NUM_OCTAVES * 7);
}

int MelodyCanvasComponent::getStepAtX(int x) const {
  return (int)(x / getStepWidth());
}
