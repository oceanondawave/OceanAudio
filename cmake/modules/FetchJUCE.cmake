include(FetchContent)

if(TARGET juce::juce_core)
    return()
endif()

set(JUCE_TAG "8.0.1" CACHE STRING "JUCE Git tag to use")

# Check if JUCE is available locally first
if(EXISTS "${CMAKE_SOURCE_DIR}/external/juce/CMakeLists.txt")
    message(STATUS "Using local JUCE from external/juce")
    FetchContent_Declare(
        juce
        SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/juce
    )
else()
    message(STATUS "Downloading JUCE from GitHub (requires internet)")
    FetchContent_Declare(
        juce
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG ${JUCE_TAG}
        GIT_SHALLOW TRUE
    )
endif()

FetchContent_MakeAvailable(juce)

