cmake_minimum_required(VERSION 3.22)
project(HouseProductionSuite VERSION 1.0.0)

# 1. Project-wide C++ Standard (C++20 is required for JUCE 8)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 2. Fetch JUCE 8 remotely (No manual download needed)

include(FetchContent)
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.0  # Use specific tag for stability
)
FetchContent_MakeAvailable(juce)

# 3. Add Subdirectories

add_subdirectory(modules) # Changed from modules/djstih_dsp
add_subdirectory(plugins/RhythmEngine)
add_subdirectory(plugins/MelodyEngine)
