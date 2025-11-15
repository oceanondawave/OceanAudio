if(NOT WIN32)
    message(STATUS "Skipping OceanAudioBridgeService (Windows only)")
    return()
endif()

add_executable(OceanAudioBridgeService
    src/ServiceMain.cpp
    src/BridgeConsumer.cpp
)

target_include_directories(OceanAudioBridgeService
    PRIVATE
        ${CMAKE_SOURCE_DIR}/shared/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(OceanAudioBridgeService
    PRIVATE
        UNICODE
        _UNICODE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
)

target_link_libraries(OceanAudioBridgeService
    PRIVATE
        ws2_32
        advapi32
        user32
        synchronization
        setupapi
        cfgmgr32
)

if(MSVC)
    target_compile_options(OceanAudioBridgeService PRIVATE /W4 /permissive- /MP)
endif()

