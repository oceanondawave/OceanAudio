juce_add_gui_app(OceanAudioHost
    PRODUCT_NAME "OceanAudio"
    VERSION ${PROJECT_VERSION}
    COMPANY_NAME "OceanAudio"
    BUNDLE_ID com.oceanaudio.host
)

target_sources(OceanAudioHost
    PRIVATE
        src/Main.cpp
        src/Application.cpp
        src/Application.h
        src/MainWindow.cpp
        src/MainWindow.h
        src/AudioEngine.cpp
        src/AudioEngine.h
        src/PluginChain.cpp
        src/PluginChain.h
        src/PluginManager.cpp
        src/PluginManager.h
        src/PluginListComponent.cpp
        src/PluginListComponent.h
        src/PluginChainComponent.cpp
        src/PluginChainComponent.h
        src/PresetManager.cpp
        src/PresetManager.h
        resources/presets/FactoryPresets.json
        src/BridgeClient.cpp
        src/BridgeClient.h
)

target_compile_definitions(OceanAudioHost
    PRIVATE
        JUCE_USE_CURL=0
        JUCE_WEB_BROWSER=0
        JUCE_MODAL_LOOPS_PERMITTED=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_REPORT_APP_USAGE=0
        JUCE_STRICT_REFCOUNTEDPOINTER=1
)

target_link_libraries(OceanAudioHost
    PRIVATE
        juce::juce_gui_extra
        juce::juce_audio_utils
        juce::juce_audio_devices
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_product_unlocking
        VST3::sdk
)

target_include_directories(OceanAudioHost
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/shared/include
)

if(MSVC)
    target_compile_options(OceanAudioHost PRIVATE /W4 /MP /permissive-)
else()
    target_compile_options(OceanAudioHost PRIVATE -Wall -Wextra -Wpedantic -Wshadow -Wconversion)
endif()

