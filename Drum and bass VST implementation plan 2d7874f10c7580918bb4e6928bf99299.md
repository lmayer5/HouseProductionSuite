# Drum and bass VST implementation plan

Section 1: The Infrastructure Setup (Your Job)
Do not feed this to the AI. The AI cannot interact with your file system or install tools. You must execute this manually to create the "Skeleton" that the AI will flesh out.
Step 1.1: The Directory Structure
Create a new folder named House-Production-Suite. Inside, create exactly this hierarchy. Case sensitivity matters.
/House-Production-Suite
├── CMakeLists.txt              <-- (ROOT BUILD FILE)
├── /modules
│    └── /djstih_dsp            <-- (SHARED CODE)
│         ├── CMakeLists.txt
│         ├── djstih_dsp.h
│         └── djstih_dsp.cpp
└── /plugins
└── /RhythmEngine          <-- (YOUR PLUGIN)
├── CMakeLists.txt
└── /Source
├── PluginProcessor.h
├── PluginProcessor.cpp
├── PluginEditor.h
└── PluginEditor.cpp

Step 1.2: The Root CMake Configuration
Create the CMakeLists.txt in the root folder. This file instructs CMake to download JUCE 8 automatically.
Paste this exact content:
cmake_minimum_required(VERSION 3.22)
project(HouseProductionSuite VERSION 0.0.1)

# 1. Project-wide C++ Standard (C++20 is required for JUCE 8)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 2. Fetch JUCE 8 remotely (No manual download needed)

include(FetchContent)
FetchContent_Declare(
juce
GIT_REPOSITORY [https://github.com/juce-framework/JUCE.git](https://github.com/juce-framework/JUCE.git)
GIT_TAG        8.0.0  # Use specific tag for stability
)
FetchContent_MakeAvailable(juce)

# 3. Add Subdirectories

add_subdirectory(modules/djstih_dsp)
add_subdirectory(plugins/RhythmEngine)

Step 1.3: The Shared Library CMake
Create modules/djstih_dsp/CMakeLists.txt. This defines your math library.

# Define the library

juce_add_module(djstih_dsp)

# Link basic JUCE modules (Core, Audio Basics)

target_link_libraries(djstih_dsp PRIVATE
juce::juce_core
juce::juce_audio_basics
)

Step 1.4: The Plugin CMake
Create plugins/RhythmEngine/CMakeLists.txt. This defines the VST3 target.

# Define the Plugin

juce_add_plugin(RhythmEngine
COMPANY_NAME "DJstih"
IS_SYNTH TRUE
NEEDS_MIDI_INPUT TRUE
NEEDS_MIDI_OUTPUT FALSE
IS_MIDI_EFFECT FALSE
COPY_PLUGIN_AFTER_BUILD TRUE
PLUGIN_MANUFACTURER_CODE "DJst"
PLUGIN_CODE "Rhyt"
FORMATS VST3 Standalone
PRODUCT_NAME "Rhythm Engine"
)

# Source Files

target_sources(RhythmEngine PRIVATE
Source/PluginProcessor.cpp
Source/PluginProcessor.h
Source/PluginEditor.cpp
Source/PluginEditor.h
)

# Link against JUCE and YOUR shared library

target_link_libraries(RhythmEngine PRIVATE
djstih_dsp        # <--- Your shared code
juce::juce_audio_utils
juce::juce_dsp
juce::juce_gui_basics
)

Step 1.5: Verify the Build

- Open Visual Studio 2022 (or Terminal).
- Open the House-Production-Suite folder.
- Let CMake configure. It will take 2-3 minutes to download JUCE.
- Stop. Do not proceed until you see "CMake generation finished" with 0 errors.
Section 2: The Implementation Prompts (AI Agent)
Once the files exist (even if empty), feed these prompts to your coding agent (Cursor/Antigravity) sequentially. Do not copy-paste all at once.
Prompt 1: The Mathematical Core (djstih_dsp)
Goal: Build the DSP objects. We treat this like building the components on a breadboard.

> Prompt:
"I need to implement the shared DSP library in modules/djstih_dsp.
Task:
Write the header djstih_dsp.h and implementation djstih_dsp.cpp to include three classes. Use namespace djstih.
> 
> 1. Class Phasor:
> - Role: A phase accumulator for oscillators.
> - Member: double phase (0.0 to 1.0).
> - Method: float process(float frequency, double sampleRate) which increments the phase and wraps it.
> - Constraint: Use std::fmod or conditional subtraction for wrapping.
> 1. Class AdsrEnvelope:
> - Role: Fast, per-sample envelope generation for drums.
> - State Machine: IDLE, ATTACK, DECAY, HOLD. (Ignore Sustain/Release for now, we want "One-Shot" drum behavior).
> - Method: float getNextSample() returns the current envelope value (0.0 to 1.0).
> - Method: trigger() resets state to ATTACK.
> - Math: Use exponential decay logic: output *= coeff.
> 1. Class ClipFunctions:
> - Role: Static utility for saturation.
> - Method: static float softClip(float input) using the formula x / (1 + |x|).
> Style: Modern C++20. Mark methods noexcept where appropriate."

Prompt 2: The Plugin State Management (RhythmEngine)
Goal: Define the "Pins" of the chip. The parameters.

> Prompt:
"Now we move to plugins/RhythmEngine/Source/PluginProcessor.h.
Task:
Update the RhythmEngineAudioProcessor class.
> 
> 1. Add the Parameter Layout:
> - Implement a private method juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout().
> - Parameters to add:
>     - KICK_FREQ (Float, 40Hz to 150Hz).
>     - KICK_DECAY (Float, 0.1s to 1.0s).
>     - BASS_CUTOFF (Float, 20Hz to 2000Hz, skew factor 0.5 for logarithmic feel).
>     - BASS_DRIVE (Float, 0.0 to 1.0).
>     - SIDECHAIN_AMT (Float, 0.0 to 1.0).
> 1. Add the State Member:
> - Add juce::AudioProcessorValueTreeState apvts; as a public member.
> - Initialize it in the constructor list: apvts(*this, nullptr, "Parameters", createParameterLayout()).
> 1. Clean up:
> - Ensure acceptsMidi() returns true."

Prompt 3: The Audio Engine (processBlock)
Goal: Wire the components together in the ISR.

> Prompt:
"Implement the audio processing logic in plugins/RhythmEngine/Source/PluginProcessor.cpp.
Task:
Implement processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages).
Logic Requirements:
> 
> - Preparation: Clear the buffer. Get parameter values using apvts.getRawParameterValue().
> - MIDI Loop: Iterate midiMessages.
>     - If Note Number == 36 (C1): Trigger kickEnvelope.
>     - If Note Number >= 48 (C2): Update bassPhasor frequency.
> - Sample Loop (The DSP):
>     - Loop through buffer.getNumSamples().
>     - Kick Path: kickOsc (Sine) * kickEnv.
>     - Bass Path: bassOsc (Saw) * bassFilter.
>     - Sidechain: Multiply Bass Output by (1.0f - (kickEnvVal * sidechainAmt)).
>     - Sum: Add Kick + Bass to the output channels.
>     Critical Constraints:
> - NO memory allocations (new, malloc) inside the loop.
> - NO locking (std::mutex).
> - Use the classes from djstih::dsp created in step 1."

Prompt 4: The GUI (PluginEditor)
Goal: Add the interface.

> Prompt:
"Finally, create the UI in plugins/RhythmEngine/Source/PluginEditor.cpp.
Task:
> 
> - Add juce::Slider members for Kick Freq, Bass Cutoff, and Sidechain.
> - Add std::unique_ptrjuce::AudioProcessorValueTreeState::SliderAttachment members for each slider.
> - Constructor:
>     - Initialize the attachments passing audioProcessor.apvts.
>     - Make sliders visible.
> - Resized:
>     - Use juce::FlexBox to layout the sliders in a simple row or grid.
>     - Do NOT use absolute coordinates like setBounds(10, 10, 100, 100).
> - Paint:
>     - Fill the background with a dark grey color."

Next Step
Once you execute Section 1, ask the AI Prompt 1.
Would you like me to explain how to verify if the VST3 is actually outputting sound once the AI finishes writing the code?