# Define the Plugin

juce_add_plugin(RhythmEngine
    COMPANY_NAME "DJstih"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    PLUGIN_MANUFACTURER_CODE "DJst"
    PLUGIN_CODE "Rhyt"
    FORMATS VST3 Standalone
    PRODUCT_NAME "Rhythm Engine"
    VST3_CATEGORIES "Instrument|Drum"
    COPY_PLUGIN_AFTER_BUILD FALSE
)

target_link_libraries(RhythmEngine PRIVATE
    djstih_dsp
    juce::juce_audio_utils
    juce::juce_audio_devices
    juce::juce_dsp
    juce::juce_gui_basics
)

target_compile_definitions(RhythmEngine PUBLIC
    JUCE_VST3_CAN_REPLACE_VST2=0
)

# Source Files

target_sources(RhythmEngine PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
)

# Link against JUCE and YOUR shared library


# Generate JuceHeader.h
juce_generate_juce_header(RhythmEngine)
