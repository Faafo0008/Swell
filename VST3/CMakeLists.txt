cmake_minimum_required(VERSION 3.22)
project(ReverseReverbOneClick VERSION 1.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE is fetched automatically the first time you configure (needs internet once).
include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.12
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(ReverseReverbOneClick
    COMPANY_NAME             "OneClick Audio"
    COMPANY_WEBSITE          ""
    IS_SYNTH                 FALSE
    NEEDS_MIDI_INPUT         FALSE
    NEEDS_MIDI_OUTPUT        FALSE
    IS_MIDI_EFFECT           FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD  FALSE
    PLUGIN_MANUFACTURER_CODE OcAu
    PLUGIN_CODE              Rr1c
    FORMATS                  VST3 Standalone
    PRODUCT_NAME             "Reverse Reverb One-Click"
    VST3_CATEGORIES          "Fx" "Reverb")

juce_generate_juce_header(ReverseReverbOneClick)

target_sources(ReverseReverbOneClick PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/DSP/ReverseReverbEngine.h
    Source/DSP/FreeverbTank.h)

target_compile_definitions(ReverseReverbOneClick PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(ReverseReverbOneClick PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)
