#include "ProtoVM.h"
#include "RCOscillator.h"
#include "AnalogSimulation.h"
#include <portaudio.h>
#include <thread>
#include <chrono>
#include <iostream>

// Global variables for the audio callback
static RCOscillator* g_analogOscillator = nullptr;
static AnalogSimulation* g_analogSim = nullptr;

// PortAudio callback function for real-time audio
static int paCallback(const void *inputBuffer, void *outputBuffer,
                      unsigned long framesPerBuffer,
                      const PaStreamCallbackTimeInfo* timeInfo,
                      PaStreamCallbackFlags statusFlags,
                      void *userData) {
    float *out = (float*)outputBuffer;
    (void) inputBuffer; // Prevent unused variable warning
    (void) timeInfo;
    (void) statusFlags;

    // Generate audio samples using the analog oscillator
    for (unsigned int i = 0; i < framesPerBuffer; i++) {
        // Run analog simulation tick
        if (g_analogSim) {
            g_analogSim->Tick();
        }
        
        // Get oscillator output
        float sample = 0.0f;
        if (g_analogOscillator) {
            // Normalize the analog value to audio range (-1.0 to 1.0)
            sample = g_analogOscillator->GetOutputVoltage() / 2.5f;  // Scale to ±1.0 range
            sample *= 0.3f;  // Reduce amplitude to prevent clipping
        }
        
        // Output to both left and right channels
        *out++ = sample;  // Left channel
        *out++ = sample;  // Right channel
    }
    
    return paContinue;
}

void SetupAnalogAudioOscillator(Machine& mach) {
    std::cout << "Setting up Analog Audio Oscillator Circuit..." << std::endl;
    
    // Create a PCB for the analog oscillator
    Pcb& pcb = mach.AddPcb();
    // The PCB name is managed internally, so we don't set it directly

    // Create an RC oscillator using analog components using the Pcb.Add method with new default parameters
    RCOscillator& oscillator = pcb.Add<RCOscillator>("AudioOscillator");
    
    // Register with the machine's analog simulation system
    mach.RegisterAnalogComponent(&oscillator);
    
    // Store for the audio callback
    g_analogOscillator = &oscillator;
    
    std::cout << "Analog audio oscillator circuit setup complete!" << std::endl;
    std::cout << "Components:" << std::endl;
    std::cout << "  - RC Oscillator with 1kΩ, 1kΩ, 10nF for faster oscillation" << std::endl;
    std::cout << "  - This will be used for analog audio synthesis" << std::endl;
}

void RunAnalogAudioTest() {
    std::cout << "ProtoVM Analog Audio Oscillator Test" << std::endl;
    std::cout << "===================================" << std::endl;
    
    // Create a machine for the simulation
    Machine mach;
    
    // Create a PCB for the analog oscillator
    Pcb& pcb = mach.AddPcb();

    // Create an RC oscillator using analog components using the Pcb.Add method with default parameters
    RCOscillator& oscillator = pcb.Add<RCOscillator>("AudioOscillator");
    
    // Initialize the machine
    if (!mach.Init()) {
        std::cout << "Failed to initialize the machine" << std::endl;
        return;
    }
    
    std::cout << "Running analog oscillator simulation for 100 ticks..." << std::endl;
    std::cout << "Demonstrating ProtoVM's analog simulation for audio generation" << std::endl;
    std::cout << "RC Oscillator: 1kΩ, 1kΩ, 10nF, 5V supply (faster oscillation)" << std::endl;
    
    // Run the simulation for a few ticks to observe the oscillator behavior
    for (int i = 0; i < 100; i++) {
        // Run analog simulation first
        mach.RunAnalogSimulation();
        
        // Run digital simulation
        if (!mach.Tick()) {
            std::cout << "Simulation halted at tick " << i << std::endl;
            break;
        }
        
        // Print status every 10 ticks
        if (i % 10 == 0) {
            std::cout << "Tick " << i << ": Output voltage = " << oscillator.GetOutputVoltage() << std::endl;
        }
    }
    
    std::cout << "Analog oscillator simulation completed!" << std::endl;
    std::cout << "This demonstrates ProtoVM's analog simulation capabilities," << std::endl;
    std::cout << "which can be extended to generate audio output with PortAudio." << std::endl;
}