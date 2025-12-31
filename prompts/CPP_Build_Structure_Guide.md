# C++ Build Structure Guide

This document explains the CMake + JUCE build system for the House Production Suite. Use this as a reference when working in this codebase.

## Project Overview

```
house_prod_suite/
├── CMakeLists.txt          # Root build config
├── build_plugins.bat       # Windows build script (VS 2026)
├── modules/
│   ├── CMakeLists.txt      # Module registration
│   └── djstih_dsp/         # Shared DSP code module
├── plugins/
│   ├── RhythmEngine/       # Drum/Bass VST3
│   ├── MelodyEngine/       # Polyphonic Sampler VST3
│   └── SampleEngine/       # Sample playback VST3
└── build/                  # Generated build artifacts
```

---

## Key Technologies

| Component | Version | Purpose |
|-----------|---------|---------|
| CMake | 3.22+ | Build system generator |
| JUCE | 8.0.0 | Audio plugin framework (fetched via `FetchContent`) |
| C++ Standard | C++20 | Required by JUCE 8 |
| Ninja | bundled | Fast build executor |
| Visual Studio Build Tools | 2026 (v18) | MSVC compiler |

---

## Root CMakeLists.txt Explained

```cmake
cmake_minimum_required(VERSION 3.22)
project(HouseProductionSuite VERSION 0.0.1)

# C++20 is REQUIRED for JUCE 8
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE is fetched automatically - no manual download needed
include(FetchContent)
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.0
)
FetchContent_MakeAvailable(juce)

# Add project components
add_subdirectory(modules)
add_subdirectory(plugins/RhythmEngine)
add_subdirectory(plugins/MelodyEngine)
```

> [!IMPORTANT]
> JUCE is fetched from GitHub on first configure. This can take a few minutes. Subsequent builds use the cached version in `build/_deps/`.

---

## Adding a New Plugin

1. Create folder: `plugins/YourPlugin/Source/`
2. Create `plugins/YourPlugin/CMakeLists.txt`:

```cmake
juce_add_plugin(YourPlugin
    COMPANY_NAME "DJstih"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    PLUGIN_MANUFACTURER_CODE "DJst"
    PLUGIN_CODE "YrPl"              # 4-char unique code
    FORMATS VST3 Standalone
    PRODUCT_NAME "Your Plugin"
    VST3_CATEGORIES "Instrument"
    COPY_PLUGIN_AFTER_BUILD FALSE
)

target_sources(YourPlugin PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
)

target_link_libraries(YourPlugin PRIVATE
    djstih_dsp                      # Shared DSP module
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_basics
)

target_compile_definitions(YourPlugin PUBLIC
    JUCE_VST3_CAN_REPLACE_VST2=0
)

juce_generate_juce_header(YourPlugin)
```

3. Register in root `CMakeLists.txt`:
```cmake
add_subdirectory(plugins/YourPlugin)
```

---

## The Shared Module: `djstih_dsp`

Located at `modules/djstih_dsp/`, this module contains reusable DSP classes.

**Registration (modules/CMakeLists.txt):**
```cmake
juce_add_module(djstih_dsp)

target_link_libraries(djstih_dsp INTERFACE
    juce::juce_core
    juce::juce_audio_basics
)
```

**Usage in plugins:**
```cmake
target_link_libraries(YourPlugin PRIVATE
    djstih_dsp    # Just add this line
    ...
)
```

---

## Build Commands

### Windows (Visual Studio 2026 Build Tools)

The `build_plugins.bat` script handles environment setup:

```batch
.\build_plugins.bat
```

**Manual build:**
```powershell
# 1. Setup VS environment
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat"

# 2. Configure (first time or after CMake changes)
cmake -S . -B build -G "Ninja"

# 3. Build
cmake --build build --config Release
```

### Output Location

VST3 files are generated at:
```
build/plugins/<PluginName>/<PluginName>_artefacts/Release/VST3/<PluginName>.vst3
```

---

## Common Issues

| Issue | Solution |
|-------|----------|
| `cmake not found` | Use full path or run from VS Developer Command Prompt |
| `JUCE fetch fails` | Check internet connection; delete `build/_deps/juce-*` and retry |
| `C++20 errors` | Ensure VS 2019 16.10+ or VS 2022/2026 is installed |
| `ninja not found` | Use `-DCMAKE_MAKE_PROGRAM=<path>` or install Ninja |

---

## JUCE Key Concepts for Agents

- **AudioProcessor**: The plugin's main class. Handles audio in `processBlock()`.
- **AudioProcessorEditor**: The GUI class.
- **AudioProcessorValueTreeState (APVTS)**: Parameter management. Use this for all user-facing controls.
- **Synthesiser + SynthesiserVoice**: Polyphonic audio synthesis framework.
- **juce_dsp module**: Contains DSP building blocks (`IIR::Filter`, `Gain`, `Oscillator`, etc.).

---

## File Naming Conventions

| File | Purpose |
|------|---------|
| `PluginProcessor.cpp/h` | Audio processing logic |
| `PluginEditor.cpp/h` | GUI implementation |
| `*Component.cpp/h` | Reusable UI components |
| `*Voice.cpp/h` | Synthesizer voice classes |
