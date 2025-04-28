cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(Ir_Generator VERSION 0.1.0)

# Configurer le chemin vers JUCE
add_subdirectory("C:/Users/mikez/Desktop/JUCE" JUCE)

# Configurer le chemin vers Qt6
set(Qt6_DIR "C:/Qt/6.9.0/msvc2022_64/lib/cmake/Qt6")

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Multimedia Network)
if (NOT Qt6_FOUND)
    find_package(Qt5 REQUIRED COMPONENTS Core Widgets Multimedia Network)
endif()

# Options de compilation pour MSVC
if(MSVC)
    add_compile_options(/Zc:__cplusplus /permissive-)
    add_compile_options(/MD)
endif()

# Définition du plugin
juce_add_plugin(Ir_Generator
    VERSION ${PROJECT_VERSION}
    COMPANY_NAME "TangoFlux"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Tfxg
    PLUGIN_CODE GenR
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "IR Generator")

# Générer les fichiers d'en-tête JUCE
juce_generate_juce_header(Ir_Generator)

# Fichiers source
target_sources(Ir_Generator
    PRIVATE
        src/PluginProcessor.cpp
        src/PluginEditor.cpp
        src/IRGeneratorPanel.cpp)

# Définitions de compilation
target_compile_definitions(Ir_Generator
    PUBLIC
        JUCE_ALSA=1
        JUCE_DIRECTSOUND=1
        JUCE_DISABLE_CAUTIOUS_PARAMETER_ID_CHECKING=1
        JUCE_USE_OGGVORBIS=1
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

# Inclure les répertoires de Qt
target_include_directories(Ir_Generator
    PRIVATE
        ${Qt6Core_INCLUDE_DIRS}
        ${Qt6Network_INCLUDE_DIRS}
        # Inclure directement les répertoires d'en-têtes Qt
        "C:/Qt/Qt6.9.0/6.9.0/msvc2022_64/include"
        "C:/Qt/Qt6.9.0/6.9.0/msvc2022_64/include/QtCore"
        "C:/Qt/Qt6.9.0/6.9.0/msvc2022_64/include/QtNetwork")

# Bibliothèques liées
target_link_libraries(Ir_Generator
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        Qt6::Core
        Qt6::Network
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Configuration des dossiers d'installation
set_target_properties(Ir_Generator PROPERTIES
    JUCE_VST3_BINARY_LOCATION "${CMAKE_BINARY_DIR}/VST3"
    JUCE_AU_BINARY_LOCATION "${CMAKE_BINARY_DIR}/AU"
    JUCE_STANDALONE_BINARY_LOCATION "${CMAKE_BINARY_DIR}/Standalone")

# Installation
install(TARGETS Ir_Generator
    LIBRARY DESTINATION "plugins"
    ARCHIVE DESTINATION "plugins"
    RUNTIME DESTINATION "plugins")

# Créer un dossier IRs pour les fichiers de réponse impulsionnelle
install(DIRECTORY DESTINATION "IRs")