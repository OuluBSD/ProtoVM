# CMakeLists.txt for CLAP test plugin

cmake_minimum_required(VERSION 3.10)
project(clap_sine_wave_gen)

set(CMAKE_CXX_STANDARD 11)

# Create the shared library
add_library(clap_sine_wave_gen SHARED sine_wave_gen_fixed.cpp)

# Set the output name without 'lib' prefix
set_target_properties(clap_sine_wave_gen PROPERTIES PREFIX "")

# On Linux, the plugin will be named clap_sine_wave_gen.so
# On macOS, it will be named clap_sine_wave_gen.dylib
# On Windows, it will be named clap_sine_wave_gen.dll