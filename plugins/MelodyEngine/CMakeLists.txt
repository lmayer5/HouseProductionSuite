# Define the Plugin

juce_add_plugin(MelodyEngine
    COMPANY_NAME "DJstih"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    PLUGIN_MANUFACTURER_CODE "DJst"
    PLUGIN_CODE "Melo"
    FORMATS VST3 Standalone
    PRODUCT_NAME "Melody Engine"
    VST3_CATEGORIES "Instrument|Synth"
    COPY_PLUGIN_AFTER_BUILD FALSE
)

target_link_libraries(MelodyEngine PRIVATE
    djstih_dsp
    juce::juce_audio_utils
    juce::juce_audio_devices
    juce::juce_dsp
    juce::juce_gui_basics
)

target_compile_definitions(MelodyEngine PUBLIC
    JUCE_VST3_CAN_REPLACE_VST2=0
)

# Source Files

target_sources(MelodyEngine PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/MelodyStructures.h
    Source/ScaleQuantizer.h
    Source/ScaleQuantizer.cpp
    Source/WavetableOscillator.h
    Source/WavetableOscillator.cpp
    Source/WavetableSynth.h
    Source/WavetableSynth.cpp
    Source/MelodyCanvasComponent.h
    Source/MelodyCanvasComponent.cpp
)

# Link against JUCE and YOUR shared library


# Generate JuceHeader.h
juce_generate_juce_header(MelodyEngine)
