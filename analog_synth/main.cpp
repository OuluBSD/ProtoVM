#include <iostream>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <memory>

#include "Synthesizer.h"
#include "PortAudioWrapper.h"
#include "WavWriter.h"
#include "Sequencer.h"
#include "PresetManager.h"
#include "AudioEngine.h"
#include "SynthArchitectures.h"
#include "SynthArchAdapter.h"
#include "ExamplePatches.h"

int main(int argc, char* argv[]) {
    std::cout << "1970s Analog Synthesizer Emulation with Multiple Architectures" << std::endl;

    // Initialize random seed
    std::srand(static_cast<unsigned int>(std::time(0)));

    // Check command line arguments to select synthesizer type
    std::string synthType = "subtractive";  // Default
    bool useRealtime = true;
    std::string wavFile;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--wav" || arg == "-w") {
            if (i + 1 < argc) {
                wavFile = argv[++i];
                useRealtime = false;
            }
        } else if (arg == "--synth" || arg == "-s") {
            if (i + 1 < argc) {
                synthType = argv[++i];
            }
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  --wav <file>     Output to WAV file instead of real-time audio" << std::endl;
            std::cout << "  --synth <type>   Synthesizer type: subtractive, fm, wavetable (default: subtractive)" << std::endl;
            std::cout << "  --help           Show this help message" << std::endl;
            return 0;
        }
    }

    // Create the selected synthesizer architecture
    std::unique_ptr<SynthArchitecture> synthArch;
    
    if (synthType == "fm") {
        auto fmSynth = std::make_unique<FMSynth>();
        fmSynth->setModulationIndex(3.0);
        fmSynth->setCarrierADSR(0.1, 0.3, 0.7, 0.5);
        fmSynth->setModulatorADSR(0.2, 0.4, 0.5, 0.6);
        fmSynth->setModulatorFrequency(220.0);  // Set modulator to different frequency
        synthArch = std::move(fmSynth);
        std::cout << "FM Synthesizer initialized" << std::endl;
    } 
    else if (synthType == "wavetable") {
        auto wavetableSynth = std::make_unique<WavetableSynth>();
        wavetableSynth->setADSR(0.05, 0.2, 0.8, 0.3);
        wavetableSynth->setActiveWaveform("sawtooth");  // Default to sawtooth
        synthArch = std::move(wavetableSynth);
        std::cout << "Wavetable Synthesizer initialized" << std::endl;
    }
    else {  // Default to subtractive
        auto subSynth = std::make_unique<SubtractiveSynth>();
        subSynth->setOscCount(2);  // Two oscillators
        subSynth->setOscWaveform(0, Waveform::SAWTOOTH);
        subSynth->setOscWaveform(1, Waveform::SQUARE);
        subSynth->setFilterADSR(0.1, 0.3, 0.7, 0.5);
        subSynth->setAmpADSR(0.05, 0.2, 0.8, 0.3);
        subSynth->setFilterCutoff(0.6);
        subSynth->setFilterResonance(0.3);
        synthArch = std::move(subSynth);
        std::cout << "Subtractive Synthesizer initialized" << std::endl;
    }

    // Set common parameters
    synthArch->setSampleRate(44100);
    synthArch->setVolume(0.5);  // Set master volume to 50%

    // Create sequencer
    Sequencer seq;
    seq.setBPM(120);  // 120 beats per minute
    seq.setNumNotes(8);  // 8 random notes per sequence
    seq.setOctaveRange(3, 6);  // Octaves 3-6 (C3 to C6)
    seq.start();

    // Create the adapter to connect the architecture to the audio system
    SynthArchAdapter adapter(std::move(synthArch));
    
    // Create audio engine with configurable parameters
    AudioEngine audioEngineAdapter(&adapter);
    
    // Configure audio with parameters (sample rate, channels, bit depth)
    AudioConfig audioConfig(44100, 2, 16, 512); // 44.1kHz, stereo, 16-bit, 512 sample buffer
    if (!audioEngineAdapter.initialize(audioConfig)) {
        std::cerr << "Failed to initialize AudioEngine" << std::endl;
        return 1;
    }

    if (useRealtime) {
        // Use AudioEngine for real-time output
        audioEngineAdapter.setRealTimeOutput();

        std::cout << "Playing synthesizer in real-time..." << std::endl;
        std::cout << "Press Ctrl+C to stop" << std::endl;

        if (!audioEngineAdapter.start()) {
            std::cerr << "Failed to start audio stream" << std::endl;
            return 1;
        }

        // Keep running until user stops
        try {
            while (true) {
                // Get next note from sequencer
                double nextNote = seq.getNextNote();
                if (nextNote > 0) {
                    adapter.noteOn(nextNote);
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(10));  // Small delay to prevent busy waiting
            }
        } catch (...) {
            // This catches the signal when user presses Ctrl+C
            std::cout << "\nStopping audio..." << std::endl;
        }

        audioEngineAdapter.stop();
        audioEngineAdapter.terminate();
    } else {
        // Output to WAV file using AudioEngine
        std::cout << "Generating WAV file: " << wavFile << std::endl;
        
        audioEngineAdapter.setWavFileOutput(wavFile);

        if (!audioEngineAdapter.start()) {
            std::cerr << "Failed to generate WAV file: " << wavFile << std::endl;
            return 1;
        }

        std::cout << "WAV file generated successfully: " << wavFile << std::endl;
    }

    return 0;
}