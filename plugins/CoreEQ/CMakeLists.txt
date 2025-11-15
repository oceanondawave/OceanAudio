juce_add_plugin(CoreEQ
    PRODUCT_NAME "OceanAudio Core EQ"
    VERSION ${PROJECT_VERSION}
    COMPANY_NAME "OceanAudio"
    BUNDLE_ID com.oceanaudio.plugins.coreeq
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE OcaA
    PLUGIN_CODE Ceq1
    FORMATS VST3
    VST3_CATEGORIES Fx|Eq
)

target_sources(CoreEQ
    PRIVATE
        src/CoreEQProcessor.cpp
        src/CoreEQProcessor.h
        src/CoreEQEditor.cpp
        src/CoreEQEditor.h
)

target_link_libraries(CoreEQ
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
)

