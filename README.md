# House Production Suite

A collection of modern VST3 plugins for House music production, built with JUCE 8 and C++20.

## Plugins

### 🥁 RhythmEngine
A specialized drum synthesizer focusing on tight, punchy kicks and driving basslines.
*   **Kick:** Synthesized sine sweep with fast decay.
*   **Bass:** Sawtooth wave with Lowpass Filter and Drive.
*   **Features:**
    *   Sidechain Ducking (Bass ducks when Kick triggers).
    *   Full ADSR Envelope for Bass.
    *   Intuitive UI with labeled controls.

### 🎹 MelodyEngine
A melodic synthesizer skeleton (Work In Progress).
*   Currently serves as a placeholder for future polyphonic synth development.

## Prerequisites

*   **OS:** Windows 10/11
*   **Build Tools:** Visual Studio 2022 (or newer) with C++ Desktop Development and CMake tools installed.
*   **CMake:** Version 3.22+
*   **Ninja:** Recommended build (included in repo or system path).

## Building

This project uses a custom batch script to automate the CMake configuration and build process using the **Ninja** generator for speed.

1.  **Open Command Prompt** (or VS Developer Command Prompt).
2.  **Run the build script:**
    ```cmd
    build_plugins.bat
    ```

This script will:
*   Configure the project using `CMake` and `Ninja`.
*   Fetch `JUCE 8.0.0` automatically.
*   Build both `RhythmEngine` and `MelodyEngine` VST3s.

## Project Structure

*   `plugins/`: Source code for VST3 plugins.
*   `modules/`: Shared internal JUCE modules (e.g., `djstih_dsp` for shared DSP logic).
*   `scraper/`: Python scripts for sample scraping (separate tool).
*   `CMakeLists.txt`: Root build configuration.

## License

Personal Project.
