#include <portaudio.h>
#include <iostream>
#include <cmath>
#include <thread>
#include <chrono>
#include <vector>

// Simple analog oscillator implementation for audio
class AnalogAudioOscillator {
public:
    AnalogAudioOscillator(double freq = 440.0, double sample_rate = 44100.0) 
        : frequency(freq), sample_rate(sample_rate), phase(0.0) {}
    
    void setFrequency(double freq) { frequency = freq; }
    double getFrequency() const { return frequency; }
    
    // Generate next sample using analog principles (RC charging/discharging)
    double nextSample() {
        // Simple sine wave for now, but could be replaced with analog simulation
        double value = sin(phase);
        
        // Update phase for next sample
        phase += 2.0 * M_PI * frequency / sample_rate;
        if (phase > 2.0 * M_PI) {
            phase -= 2.0 * M_PI;
        }
        
        return value * 0.3; // Reduce amplitude to prevent clipping
    }

private:
    double frequency;
    double sample_rate;
    double phase;
};

// PortAudio callback function
static int paCallback(const void *inputBuffer, void *outputBuffer,
                      unsigned long framesPerBuffer,
                      const PaStreamCallbackTimeInfo* timeInfo,
                      PaStreamCallbackFlags statusFlags,
                      void *userData) {
    float *out = (float*)outputBuffer;
    AnalogAudioOscillator* oscillator = (AnalogAudioOscillator*)userData;
    
    (void) inputBuffer; // Prevent unused variable warning
    
    for (unsigned int i = 0; i < framesPerBuffer; i++) {
        float sample = oscillator->nextSample();
        *out++ = sample;  // Left channel
        *out++ = sample;  // Right channel (stereo)
    }
    
    return paContinue;
}

int main() {
    std::cout << "ProtoVM Analog Audio Oscillator Test" << std::endl;
    std::cout << "=====================================" << std::endl;
    
    // Initialize PortAudio
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "PortAudio initialization failed: " << Pa_GetErrorText(err) << std::endl;
        return -1;
    }
    
    // Create oscillator
    AnalogAudioOscillator oscillator(440.0); // A4 note
    
    // Open audio stream
    PaStream *stream;
    err = Pa_OpenDefaultStream(&stream,
                               0,          // No input channels
                               2,          // Stereo output
                               paFloat32,  // 32-bit floating point output
                               44100,      // Sample rate
                               256,        // Frames per buffer
                               paCallback, // Callback function
                               &oscillator); // User data
    if (err != paNoError) {
        std::cerr << "PortAudio stream opening failed: " << Pa_GetErrorText(err) << std::endl;
        Pa_Terminate();
        return -1;
    }
    
    // Start the audio stream
    err = Pa_StartStream(stream);
    if (err != paNoError) {
        std::cerr << "PortAudio stream starting failed: " << Pa_GetErrorText(err) << std::endl;
        Pa_CloseStream(stream);
        Pa_Terminate();
        return -1;
    }
    
    std::cout << "Playing 440Hz tone for 10 seconds..." << std::endl;
    std::cout << "Press Ctrl+C to stop early" << std::endl;
    
    // Play for 10 seconds
    std::this_thread::sleep_for(std::chrono::seconds(10));
    
    // Stop and clean up
    err = Pa_StopStream(stream);
    if (err != paNoError) {
        std::cerr << "PortAudio stream stopping failed: " << Pa_GetErrorText(err) << std::endl;
    }
    
    Pa_CloseStream(stream);
    Pa_Terminate();
    
    std::cout << "Test completed!" << std::endl;
    return 0;
}