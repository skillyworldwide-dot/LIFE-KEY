cmake_minimum_required(VERSION 3.22)

project(ScaleLock VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Expect JUCE to be cloned as a sibling folder named "JUCE"
# (see README.md for the exact clone command)
add_subdirectory(JUCE)

juce_add_plugin(ScaleLock
    COMPANY_NAME "Jay Skilly"
    PLUGIN_MANUFACTURER_CODE Jsky
    PLUGIN_CODE Lfky
    FORMATS VST3 AU
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    PRODUCT_NAME "Life Key"
    COMPANY_COPYRIGHT "Jay Skilly"
    VST3_CATEGORIES "Instrument" "Synth"
    AU_MAIN_TYPE "kAudioUnitType_MusicDevice")

target_sources(ScaleLock PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(ScaleLock PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_PLUGINHOST_VST3=1
    JUCE_PLUGINHOST_AU=1)

target_link_libraries(ScaleLock PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_audio_processors)

juce_generate_juce_header(ScaleLock)
