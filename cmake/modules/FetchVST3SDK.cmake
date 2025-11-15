include(FetchContent)

if(TARGET VST3::sdk)
    return()
endif()

set(VST3_SDK_TAG "v3.7.12_build_20" CACHE STRING "VST3 SDK Git tag")

# Check if VST3 SDK is available locally first
if(EXISTS "${CMAKE_SOURCE_DIR}/external/vst3sdk/CMakeLists.txt")
    message(STATUS "Using local VST3 SDK from external/vst3sdk")
    FetchContent_Declare(
        vst3sdk
        SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/vst3sdk
    )
else()
    message(STATUS "Downloading VST3 SDK from GitHub (requires internet)")
    FetchContent_Declare(
        vst3sdk
        GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk.git
        GIT_TAG ${VST3_SDK_TAG}
        GIT_SHALLOW TRUE
    )
endif()

FetchContent_MakeAvailable(vst3sdk)

add_library(VST3::sdk INTERFACE IMPORTED)
target_include_directories(VST3::sdk INTERFACE "${vst3sdk_SOURCE_DIR}")

