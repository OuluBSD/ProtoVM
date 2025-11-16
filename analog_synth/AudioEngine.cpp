#include "AudioEngine.h"
#include <iostream>

AudioEngine::AudioEngine(Synthesizer* synth) :
    synthesizer(synth),
    paWrapper(nullptr),
    wavWriter(nullptr),
    config(),
    initialized(false),
    playing(false),
    renderingToFile(false) {}

AudioEngine::~AudioEngine() {
    if (playing) {
        stop();
    }
    if (initialized) {
        terminate();
    }
}

bool AudioEngine::initialize(const AudioConfig& newConfig) {
    if (initialized) {
        std::cerr << "AudioEngine already initialized. Terminate first." << std::endl;
        return false;
    }
    
    config = newConfig;
    initialized = true;
    
    return true;
}

bool AudioEngine::start() {
    if (!initialized) {
        std::cerr << "AudioEngine not initialized. Call initialize() first." << std::endl;
        return false;
    }
    
    if (renderingToFile) {
        // Setup for WAV file rendering
        if (!wavWriter) {
            wavWriter = std::make_unique<WavWriter>();
        }
        
        if (!wavWriter->open(wavFilename, config.sampleRate, config.channels, config.bitsPerSample)) {
            std::cerr << "Failed to open WAV file: " << wavFilename << std::endl;
            return false;
        }
        
        // Generate samples for the duration we want
        // For now, let's generate 10 seconds of audio
        int totalSamples = config.sampleRate * 10;  // 10 seconds
        
        for (int i = 0; i < totalSamples; i++) {
            float sample = synthesizer->getNextSample();
            
            // If stereo, we might want to duplicate or process the sample differently
            // For now just write the same sample to both channels if stereo
            if (config.channels == 1) {
                wavWriter->writeSample(sample);
            } else { // stereo
                wavWriter->writeSample(sample);
                wavWriter->writeSample(sample); // For now use same sample for right channel
            }
        }
        
        wavWriter->close();
        playing = false;
        return true;
    } else {
        // Setup for real-time audio
        if (!paWrapper) {
            paWrapper = std::make_unique<PortAudioWrapper>(synthesizer);
        }
        
        if (!paWrapper->initialize()) {
            std::cerr << "Failed to initialize PortAudio wrapper." << std::endl;
            return false;
        }
        
        if (!paWrapper->start()) {
            std::cerr << "Failed to start PortAudio stream." << std::endl;
            return false;
        }
        
        playing = true;
        return true;
    }
}

bool AudioEngine::stop() {
    if (!initialized || !playing) {
        return true; // Not playing, so consider it stopped
    }
    
    if (renderingToFile) {
        // For file rendering, the start() method processes and completes the file
        // So no specific stop is needed in this implementation
        playing = false;
        return true;
    } else {
        // For real-time audio
        if (paWrapper) {
            paWrapper->stop();
            playing = false;
            return true;
        }
    }
    
    return false;
}

void AudioEngine::terminate() {
    if (playing) {
        stop();
    }
    
    if (paWrapper) {
        paWrapper->terminate();
        paWrapper.reset();
    }
    
    initialized = false;
}

bool AudioEngine::setRealTimeOutput() {
    if (playing) {
        std::cerr << "Cannot change output type while playing." << std::endl;
        return false;
    }
    
    renderingToFile = false;
    return true;
}

bool AudioEngine::setWavFileOutput(const std::string& filename) {
    if (playing) {
        std::cerr << "Cannot change output type while playing." << std::endl;
        return false;
    }
    
    wavFilename = filename;
    renderingToFile = true;
    return true;
}

bool AudioEngine::setConfig(const AudioConfig& newConfig) {
    if (initialized) {
        std::cerr << "Cannot change configuration while initialized." << std::endl;
        return false;
    }
    
    config = newConfig;
    return true;
}

void AudioEngine::processAudioBuffer(float* buffer, int framesPerBuffer) {
    for (int i = 0; i < framesPerBuffer; i++) {
        float sample = synthesizer->getNextSample();
        
        if (config.channels == 1) {
            // Mono
            *buffer++ = sample;
        } else {
            // Stereo - left and right channels
            *buffer++ = sample;  // Left channel
            *buffer++ = sample;  // Right channel (could be different for stereo effects)
        }
    }
}