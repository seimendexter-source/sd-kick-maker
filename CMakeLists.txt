cmake_minimum_required(VERSION 3.22)

project(SDKickMaker VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE: lokaler Ordner "JUCE" wenn vorhanden, sonst automatisch von GitHub laden
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

if(APPLE)
    set(SD_FORMATS AU VST3)
else()
    set(SD_FORMATS VST3)
endif()

juce_add_plugin(SDKickMaker
    COMPANY_NAME "Seimen Dexter"
    COMPANY_WEBSITE "https://dexit-techno.de"
    BUNDLE_ID "de.seimendexter.sdkickmaker"
    PLUGIN_MANUFACTURER_CODE SDxt
    PLUGIN_CODE SdKk
    PRODUCT_NAME "SD KICK MAKER"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    VST3_CATEGORIES Instrument Drum
    FORMATS ${SD_FORMATS})

# --- Ressourcen: Schriften (Google Fonts / OFL) laden, Logo aus Textdatei entpacken ---
set(SD_RES_DIR "${CMAKE_CURRENT_BINARY_DIR}/sd_resources")
file(MAKE_DIRECTORY "${SD_RES_DIR}")

if(NOT EXISTS "${SD_RES_DIR}/Anton-Regular.ttf")
    file(DOWNLOAD "https://raw.githubusercontent.com/google/fonts/main/ofl/anton/Anton-Regular.ttf"
         "${SD_RES_DIR}/Anton-Regular.ttf" STATUS st1 TLS_VERIFY ON)
endif()
if(NOT EXISTS "${SD_RES_DIR}/Montserrat-SemiBold.ttf")
    file(DOWNLOAD "https://raw.githubusercontent.com/JulietaUla/Montserrat/v7.222/fonts/ttf/Montserrat-SemiBold.ttf"
         "${SD_RES_DIR}/Montserrat-SemiBold.ttf" STATUS st2 TLS_VERIFY ON)
endif()

find_package(Python3 COMPONENTS Interpreter REQUIRED)
execute_process(COMMAND "${Python3_EXECUTABLE}" -c
    "import base64,sys; open(sys.argv[2],'wb').write(base64.b64decode(open(sys.argv[1]).read()))"
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/sd_logo.png.b64" "${SD_RES_DIR}/sd_logo.png"
    COMMAND_ERROR_IS_FATAL ANY)

juce_add_binary_data(SDKickMakerData SOURCES
    "${SD_RES_DIR}/sd_logo.png"
    "${SD_RES_DIR}/Anton-Regular.ttf"
    "${SD_RES_DIR}/Montserrat-SemiBold.ttf")

target_sources(SDKickMaker PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/KickEngine.cpp)

target_compile_definitions(SDKickMaker PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(SDKickMaker
    PRIVATE
        SDKickMakerData
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

option(SD_BUILD_TESTS "Test-Programm bauen" OFF)
if(SD_BUILD_TESTS)
    juce_add_console_app(SDKickTest PRODUCT_NAME "SDKickTest")
    target_sources(SDKickTest PRIVATE tests/render_test.cpp Source/KickEngine.cpp)
    target_compile_definitions(SDKickTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(SDKickTest PRIVATE juce::juce_audio_formats juce::juce_dsp juce::juce_gui_basics juce::juce_recommended_config_flags)
endif()
if(SD_BUILD_TESTS)
    juce_add_gui_app(SDHostTest PRODUCT_NAME "SDHostTest")
    target_sources(SDHostTest PRIVATE tests/host_test.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp Source/KickEngine.cpp)
    target_compile_definitions(SDHostTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_STANDALONE_APPLICATION=1 JUCE_MODAL_LOOPS_PERMITTED=1)
    target_link_libraries(SDHostTest PRIVATE SDKickMakerData juce::juce_audio_utils juce::juce_dsp juce::juce_recommended_config_flags)
endif()
if(SD_BUILD_TESTS)
    juce_add_console_app(SDRumbleCheck PRODUCT_NAME "SDRumbleCheck")
    target_sources(SDRumbleCheck PRIVATE tests/rumble_check.cpp Source/KickEngine.cpp)
    target_compile_definitions(SDRumbleCheck PRIVATE JUCE_USE_CURL=0 JUCE_WEB_BROWSER=0)
    target_link_libraries(SDRumbleCheck PRIVATE juce::juce_dsp juce::juce_audio_basics juce::juce_recommended_config_flags)
endif()
