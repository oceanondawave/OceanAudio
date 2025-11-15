cmake_minimum_required(VERSION 3.21)

project(OceanAudio
    VERSION 0.1.0
    DESCRIPTION "Low-latency Windows VST host with virtual microphone output"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

include(FetchJUCE)
include(FetchVST3SDK)

add_subdirectory(host)
add_subdirectory(plugins)
# Driver + service scaffolding (requires Windows toolchain)
add_subdirectory(driver)
# Driver and installer directories contain platform-specific projects that will
# be integrated once the WDK toolchain is configured.

