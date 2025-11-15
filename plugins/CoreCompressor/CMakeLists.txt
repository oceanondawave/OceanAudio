juce_add_plugin(CoreCompressor
    PRODUCT_NAME "OceanAudio Core Compressor"
    VERSION ${PROJECT_VERSION}
    COMPANY_NAME "OceanAudio"
    BUNDLE_ID com.oceanaudio.plugins.corecompressor
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE OcaA
    PLUGIN_CODE Cco1
    FORMATS VST3
    VST3_CATEGORIES Fx|Dynamics
)

target_sources(CoreCompressor
    PRIVATE
        src/CoreCompressorProcessor.cpp
        src/CoreCompressorProcessor.h
        src/CoreCompressorEditor.cpp
        src/CoreCompressorEditor.h
)

target_link_libraries(CoreCompressor
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
)

