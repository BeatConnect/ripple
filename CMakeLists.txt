cmake_minimum_required(VERSION 3.22)
project(Ripple VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Dev mode option - enables hot reload from Vite dev server
option(RIPPLE_DEV_MODE "Enable development mode with hot reload" OFF)

# BeatConnect activation option
option(BEATCONNECT_ENABLE_ACTIVATION "Enable BeatConnect activation system" OFF)

# JUCE - use JUCE_PATH if provided (CI), otherwise fetch from GitHub
if(DEFINED JUCE_PATH AND EXISTS "${JUCE_PATH}/CMakeLists.txt")
    message(STATUS "Using JUCE from: ${JUCE_PATH}")
    add_subdirectory("${JUCE_PATH}" "${CMAKE_BINARY_DIR}/JUCE" EXCLUDE_FROM_ALL)
else()
    message(STATUS "Fetching JUCE from GitHub...")
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
    )
    FetchContent_MakeAvailable(JUCE)
endif()

# BeatConnect SDK - MUST be added BEFORE plugin target
if(BEATCONNECT_ENABLE_ACTIVATION AND EXISTS "${CMAKE_SOURCE_DIR}/beatconnect-sdk/sdk/activation/CMakeLists.txt")
    add_subdirectory(beatconnect-sdk/sdk/activation)
    target_link_libraries(beatconnect_activation PRIVATE juce::juce_cryptography)
endif()

juce_add_plugin(${PROJECT_NAME}
    COMPANY_NAME "FlorianPlassAudio"
    COMPANY_WEBSITE "https://florianplass.audio"
    COMPANY_EMAIL "support@florianplass.audio"
    PLUGIN_MANUFACTURER_CODE FpAu
    PLUGIN_CODE Rple
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "Ripple"
    BUNDLE_ID "audio.florianplass.ripple"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE
    NEEDS_WEBVIEW2 TRUE
)

target_sources(${PROJECT_NAME}
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/SpectralProcessor.cpp
        Source/SpectralProcessor.h
)

target_compile_definitions(${PROJECT_NAME}
    PUBLIC
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        $<IF:$<BOOL:${RIPPLE_DEV_MODE}>,RIPPLE_DEV_MODE=1,RIPPLE_DEV_MODE=0>
)

# BeatConnect SDK Integration - project_data.json embedding
if(EXISTS "${CMAKE_SOURCE_DIR}/resources/project_data.json")
    juce_add_binary_data(${PROJECT_NAME}_ProjectData
        HEADER_NAME "ProjectData.h"
        NAMESPACE ProjectData
        SOURCES resources/project_data.json
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_ProjectData)
    target_compile_definitions(${PROJECT_NAME} PUBLIC HAS_PROJECT_DATA=1)
else()
    target_compile_definitions(${PROJECT_NAME} PUBLIC HAS_PROJECT_DATA=0)
endif()

# BeatConnect SDK Integration - activation library linking
if(BEATCONNECT_ENABLE_ACTIVATION AND EXISTS "${CMAKE_SOURCE_DIR}/beatconnect-sdk/sdk/activation/CMakeLists.txt")
    target_link_libraries(${PROJECT_NAME} PRIVATE beatconnect_activation)
    target_compile_definitions(${PROJECT_NAME} PUBLIC BEATCONNECT_ACTIVATION_ENABLED=1)
else()
    target_compile_definitions(${PROJECT_NAME} PUBLIC BEATCONNECT_ACTIVATION_ENABLED=0)
endif()

# Copy WebUI resources after build
add_custom_command(TARGET ${PROJECT_NAME}_Standalone POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/Resources/WebUI"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}_Standalone>/Resources/WebUI"
    COMMENT "Copying WebUI resources to Standalone..."
)

add_custom_command(TARGET ${PROJECT_NAME}_VST3 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/Resources/WebUI"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}_VST3>/../Resources/WebUI"
    COMMENT "Copying WebUI resources to VST3..."
)

if(APPLE)
    add_custom_command(TARGET ${PROJECT_NAME}_AU POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/Resources/WebUI"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}_AU>/../Resources/WebUI"
        COMMENT "Copying WebUI resources to AU..."
    )
endif()

if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME}_VST3 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/Resources/WebUI"
            "$ENV{CommonProgramFiles}/VST3/Ripple.vst3/Contents/Resources/WebUI"
        COMMENT "Copying WebUI resources to installed VST3..."
    )
endif()

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
