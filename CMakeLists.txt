cmake_minimum_required(VERSION 3.22)
project(RASampler VERSION 0.2.0)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.10
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(RASampler
    COMPANY_NAME "Richard Alale"
    BUNDLE_ID "com.richardalale.rasampler"
    PLUGIN_MANUFACTURER_CODE RAlE
    PLUGIN_CODE RASM
    FORMATS VST3 Standalone
    PRODUCT_NAME "RA Sampler"
    DESCRIPTION "Eight-slot polyphonic sample instrument"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE)

juce_generate_juce_header(RASampler)

target_sources(RASampler PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h)

target_compile_features(RASampler PRIVATE cxx_std_17)
target_compile_definitions(RASampler PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    RA_FACTORY_PRESET_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/FactoryPresets\")

target_link_libraries(RASampler PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_audio_formats
    juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags
           juce::juce_recommended_lto_flags
           juce::juce_recommended_warning_flags)
