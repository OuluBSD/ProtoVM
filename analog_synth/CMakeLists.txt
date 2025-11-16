cmake_minimum_required(VERSION 3.10)
project(analog_synth)

set(CMAKE_CXX_STANDARD 11)

# Find PortAudio
find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)
endif()

# If pkg-config is not available or PortAudio is not found, try find_package
if(NOT PORTAUDIO_FOUND)
    find_package(PortAudio REQUIRED)
endif()

# Add executable
add_executable(analog_synth
    main.cpp
    Oscillator.cpp
    AnalogSynthADSR.cpp
    Portamento.cpp
    Sequencer.cpp
    Synthesizer.cpp
    PortAudioWrapper.cpp
    WavWriter.cpp
    LFO.cpp
    Filter.cpp
    ModulationMatrix.cpp
    PresetManager.cpp
    AudioEngine.cpp
    SynthArchitectures.cpp
    # Note: SynthArchAdapter.cpp is header-only
)

# Link PortAudio
if(PORTAUDIO_FOUND)
    target_include_directories(analog_synth PRIVATE ${PORTAUDIO_INCLUDE_DIRS})
    target_link_libraries(analog_synth ${PORTAUDIO_LIBRARIES})
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)
    target_include_directories(analog_synth PRIVATE ${PORTAUDIO_INCLUDE_DIRS})
    target_link_libraries(analog_synth ${PORTAUDIO_LIBRARIES})
endif()

# Add include directory
target_include_directories(analog_synth PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})