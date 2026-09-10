# CMakeLists.txt for LADSPA test plugin

cmake_minimum_required(VERSION 3.10)
project(ladspa_sine_wave_gen)

set(CMAKE_C_STANDARD 99)

# Create the shared library
add_library(sine_wave_gen SHARED sine_wave_gen.c)

# Set the output name without 'lib' prefix
set_target_properties(sine_wave_gen PROPERTIES PREFIX "")

# On Linux, the plugin will be named sine_wave_gen.so
# On macOS, it will be named sine_wave_gen.dylib
# On Windows, it will be named sine_wave_gen.dll