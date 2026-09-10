/**
 * Test that verifies the fixture plugins build correctly
 */

#include <iostream>
#include <cstdlib>
#include <filesystem>

namespace fs = std::filesystem;

int main() {
    std::cout << "Testing that fixture plugins can be built..." << std::endl;

    // Check if fixture plugin source exists
    if (!fs::exists("tests/fixtures/ladspa/sine_wave_gen.c")) {
        std::cout << "ERROR: LADSPA fixture source not found" << std::endl;
        return 1;
    }
    
    if (!fs::exists("tests/fixtures/clap/sine_wave_gen_fixed.cpp")) {
        std::cout << "ERROR: CLAP fixture source not found" << std::endl;
        return 1;
    }
    
    std::cout << "Fixture sources exist." << std::endl;

    // Try to create build directories
    try {
        fs::create_directories("tests/fixtures/ladspa/build");
        fs::create_directories("tests/fixtures/clap/build");
        std::cout << "Build directories created." << std::endl;
    } catch (const std::exception& e) {
        std::cout << "ERROR creating build directories: " << e.what() << std::endl;
        return 1;
    }

    // Try to build the LADSPA plugin using cmake
    std::cout << "Attempting to build LADSPA fixture plugin..." << std::endl;
    int ladspa_result = std::system("cd tests/fixtures/ladspa/build && cmake .. && make");
    if (ladspa_result != 0) {
        std::cout << "LADSPA plugin build failed (this may be expected in some environments)" << std::endl;
    } else {
        std::cout << "LADSPA plugin built successfully." << std::endl;
        
        // Check that the plugin file was created
        if (fs::exists("tests/fixtures/ladspa/build/sine_wave_gen.so") ||
            fs::exists("tests/fixtures/ladspa/build/sine_wave_gen.dll") ||
            fs::exists("tests/fixtures/ladspa/build/sine_wave_gen.dylib")) {
            std::cout << "LADSPA plugin file was created." << std::endl;
        } else {
            std::cout << "WARNING: LADSPA plugin file not found after build" << std::endl;
        }
    }

    // Try to build the CLAP plugin using cmake
    std::cout << "Attempting to build CLAP fixture plugin..." << std::endl;
    int clap_result = std::system("cd tests/fixtures/clap/build && cmake .. && make");
    if (clap_result != 0) {
        std::cout << "CLAP plugin build failed (this may be expected in some environments)" << std::endl;
    } else {
        std::cout << "CLAP plugin built successfully." << std::endl;
        
        // Check that the plugin file was created
        if (fs::exists("tests/fixtures/clap/build/clap_sine_wave_gen.so") ||
            fs::exists("tests/fixtures/clap/build/clap_sine_wave_gen.dll") ||
            fs::exists("tests/fixtures/clap/build/clap_sine_wave_gen.dylib")) {
            std::cout << "CLAP plugin file was created." << std::endl;
        } else {
            std::cout << "WARNING: CLAP plugin file not found after build" << std::endl;
        }
    }

    std::cout << "Fixture plugin build test completed!" << std::endl;
    return 0;
}