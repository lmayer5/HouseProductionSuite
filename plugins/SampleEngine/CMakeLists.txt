# Define the Plugin

juce_add_plugin(SampleEngine
    COMPANY_NAME "DJstih"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE "DJst"
    PLUGIN_CODE "SamE"
    FORMATS VST3 Standalone
    PRODUCT_NAME "Sample Engine"
)

# Source Files

target_sources(SampleEngine PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
)

target_include_directories(SampleEngine PRIVATE
    Source
    ../../libs
)

# Link against JUCE and shared library (if needed later)

target_link_libraries(SampleEngine PRIVATE
    djstih_dsp
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_basics
    juce::juce_audio_formats
)
