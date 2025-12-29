# Add the djstih_dsp module
juce_add_module(djstih_dsp)

# Link dependencies for the module (if needed, though juce_add_module handles interface link)
# But wait, target_link_libraries needs a target. juce_add_module creates one.
target_link_libraries(djstih_dsp INTERFACE
    juce::juce_core
    juce::juce_audio_basics
)

target_include_directories(djstih_dsp INTERFACE .)
