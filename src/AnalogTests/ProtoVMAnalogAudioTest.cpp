#include "ProtoVM/ProtoVM.h"
#include "ProtoVM/RCOscillator.h"
#include "ProtoVM/AnalogSimulation.h"
#include <portaudio.h>
#include <iostream>
#include <cmath>
#include <thread>
#include <chrono>

// Analog audio oscillator using ProtoVM's analog simulation components
class ProtoVMAnalogAudioOscillator {
public:
    ProtoVMAnalogAudioOscillator(double sample_rate = 44100.0) 
        : sample_rate(sample_rate), current_sample(0.0), time_elapsed(0.0) {
        
        // Create an RC oscillator using ProtoVM analog components
        oscillator = new RCOscillator(1000.0, 1000.0, 1e-6, 5.0); // 1kΩ, 1kΩ, 1μF, 5V
        oscillator->SetName("Audio_Oscillator");
        
        // Initialize analog simulation
        analog_sim = new AnalogSimulation();
        analog_sim->SetTimeStep(1.0 / sample_rate);  // Match audio sample rate
        analog_sim->RegisterAnalogComponent(oscillator);
    }
    
    ~ProtoVMAnalogAudioOscillator() {
        delete analog_sim;
        delete oscillator;
    }
    
    // Generate next sample using analog simulation
    double nextSample() {
        // Run a single simulation step
        if (analog_sim->Tick()) {
            // Get the output voltage from the oscillator
            current_sample = oscillator->GetOutputVoltage() / 5.0; // Normalize to 0-1 range
            
            // Convert voltage to audio range (-1 to 1)
            current_sample = current_sample * 2.0 - 1.0; // Convert 0-1 to -1 to 1
            current_sample *= 0.3; // Reduce amplitude to prevent clipping
        } else {
            // If simulation fails, provide a default value
            current_sample = 0.0;
        }
        
        // Increment time
        time_elapsed += 1.0 / sample_rate;
        
        return current_sample;
    }
    
    RCOscillator* getOscillator() { return oscillator; }

private:
    double sample_rate;
    double current_sample;
    double time_elapsed;
    RCOscillator* oscillator;
    AnalogSimulation* analog_sim;
};

// PortAudio callback function
static int paCallback(const void *inputBuffer, void *outputBuffer,
                      unsigned long framesPerBuffer,
                      const PaStreamCallbackTimeInfo* timeInfo,
                      PaStreamCallbackFlags statusFlags,
                      void *userData) {
    float *out = (float*)outputBuffer;
    ProtoVMAnalogAudioOscillator* oscillator = (ProtoVMAnalogAudioOscillator*)userData;
    
    (void) inputBuffer; // Prevent unused variable warning
    (void) timeInfo;
    (void) statusFlags;
    
    for (unsigned int i = 0; i < framesPerBuffer; i++) {
        float sample = oscillator->nextSample();
        *out++ = sample;  // Left channel
        *out++ = sample;  // Right channel (stereo)
    }
    
    return paContinue;
}

int main() {
    std::cout << "ProtoVM Real-Time Analog Audio Test" << std::endl;
    std::cout << "===================================" << std::endl;
    
    // Initialize PortAudio
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "PortAudio initialization failed: " << Pa_GetErrorText(err) << std::endl;
        return -1;
    }
    
    // Create ProtoVM-based analog oscillator
    ProtoVMAnalogAudioOscillator audioOscillator(44100.0);
    
    // Open audio stream
    PaStream *stream;
    err = Pa_OpenDefaultStream(&stream,
                               0,          // No input channels
                               2,          // Stereo output
                               paFloat32,  // 32-bit floating point output
                               44100,      // Sample rate
                               512,        // Frames per buffer
                               paCallback, // Callback function
                               &audioOscillator); // User data
    if (err != paNoError) {
        std::cerr << "PortAudio stream opening failed: " << Pa_GetErrorText(err) << std::endl;
        Pa_Terminate();
        return -1;
    }
    
    std::cout << "Playing analog oscillator audio for 10 seconds..." << std::endl;
    std::cout << "Using ProtoVM's analog simulation for real-time audio generation" << std::endl;
    std::cout << "Press Ctrl+C to stop early" << std::endl;
    
    // Start the audio stream
    err = Pa_StartStream(stream);
    if (err != paNoError) {
        std::cerr << "PortAudio stream starting failed: " << Pa_GetErrorText(err) << std::endl;
        Pa_CloseStream(stream);
        Pa_Terminate();
        return -1;
    }
    
    // Play for 10 seconds
    std::this_thread::sleep_for(std::chrono::seconds(10));
    
    // Stop and clean up
    err = Pa_StopStream(stream);
    if (err != paNoError) {
        std::cerr << "PortAudio stream stopping failed: " << Pa_GetErrorText(err) << std::endl;
    }
    
    Pa_CloseStream(stream);
    Pa_Terminate();
    
    std::cout << "Analog audio test completed!" << std::endl;
    return 0;
}