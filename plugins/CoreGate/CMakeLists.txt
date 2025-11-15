juce_add_plugin(CoreGate
    PRODUCT_NAME "OceanAudio Core Gate"
    VERSION ${PROJECT_VERSION}
    COMPANY_NAME "OceanAudio"
    BUNDLE_ID com.oceanaudio.plugins.coregate
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE OcaA
    PLUGIN_CODE Cgt1
    FORMATS VST3
    VST3_CATEGORIES Fx|Dynamics
)

target_sources(CoreGate
    PRIVATE
        src/CoreGateProcessor.cpp
        src/CoreGateProcessor.h
        src/CoreGateEditor.cpp
        src/CoreGateEditor.h
)

target_link_libraries(CoreGate
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
)

