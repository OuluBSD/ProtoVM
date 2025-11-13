#include <iostream>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <ctime>

#include "Synthesizer.h"
#include "PortAudioWrapper.h"
#include "WavWriter.h"
#include "Sequencer.h"

int main(int argc, char* argv[]) {
    std::cout << "1970s Analog Synthesizer Emulation" << std::endl;
    
    // Initialize random seed
    std::srand(static_cast<unsigned int>(std::time(0)));
    
    // Create synthesizer
    Synthesizer synth;
    
    // Set up synthesizer parameters
    synth.setWaveform(Waveform::SAWTOOTH);  // Classic analog waveform
    synth.setADSRParams(0.1, 0.2, 0.7, 0.3);  // Fast attack, medium decay, sustaining release
    synth.setPortamentoTime(0.1);  // 100ms glide time
    synth.setPortamentoEnabled(true);
    
    // Create sequencer
    Sequencer seq;
    seq.setBPM(120);  // 120 beats per minute
    seq.setNumNotes(8);  // 8 random notes per sequence
    seq.setOctaveRange(3, 6);  // Octaves 3-6 (C3 to C6)
    seq.start();
    
    // Check command line arguments
    bool useRealtime = true;
    std::string wavFile;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--wav" || arg == "-w") {
            if (i + 1 < argc) {
                wavFile = argv[++i];
                useRealtime = false;
            }
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  --wav <file>  Output to WAV file instead of real-time audio" << std::endl;
            std::cout << "  --help        Show this help message" << std::endl;
            return 0;
        }
    }
    
    if (useRealtime) {
        // Use PortAudio for real-time output
        PortAudioWrapper audio(&synth);
        
        if (!audio.initialize()) {
            std::cerr << "Failed to initialize PortAudio" << std::endl;
            return 1;
        }
        
        std::cout << "Playing synthesizer in real-time..." << std::endl;
        std::cout << "Press Ctrl+C to stop" << std::endl;
        
        if (!audio.start()) {
            std::cerr << "Failed to start audio stream" << std::endl;
            return 1;
        }
        
        // Keep running until user stops
        try {
            while (true) {
                // Get next note from sequencer
                double nextNote = seq.getNextNote();
                if (nextNote > 0) {
                    synth.noteOn(nextNote);
                }
                
                std::this_thread::sleep_for(std::chrono::milliseconds(10));  // Small delay to prevent busy waiting
            }
        } catch (...) {
            // This catches the signal when user presses Ctrl+C
            std::cout << "\nStopping audio..." << std::endl;
        }
        
        audio.stop();
        audio.terminate();
    } else {
        // Output to WAV file
        std::cout << "Generating WAV file: " << wavFile << std::endl;
        
        WavWriter wav;
        if (!wav.open(wavFile)) {
            std::cerr << "Failed to open WAV file for writing: " << wavFile << std::endl;
            return 1;
        }
        
        // Generate 10 seconds of audio
        int sampleRate = 44100;
        int totalSamples = sampleRate * 10;  // 10 seconds
        
        std::cout << "Rendering audio... " << std::flush;
        int lastPercent = 0;
        
        for (int i = 0; i < totalSamples; ++i) {
            // Get next note from sequencer based on sample rate
            if (i % (sampleRate / 2) == 0) {  // Change note every half second
                double nextNote = seq.getNextNote();
                if (nextNote > 0) {
                    synth.noteOn(nextNote);
                }
            }
            
            float sample = synth.getNextSample();
            wav.writeSample(sample);
            
            // Show progress
            int percent = (i * 100) / totalSamples;
            if (percent > lastPercent && percent % 10 == 0) {
                std::cout << percent << "% " << std::flush;
                lastPercent = percent;
            }
        }
        
        std::cout << "100%" << std::endl;
        
        wav.close();
        std::cout << "WAV file generated successfully: " << wavFile << std::endl;
    }
    
    return 0;
}