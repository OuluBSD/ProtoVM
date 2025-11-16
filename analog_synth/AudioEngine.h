#ifndef AUDIO_ENGINE_H
#define AUDIO_ENGINE_H

#include "Synthesizer.h"
#include "PortAudioWrapper.h"
#include "WavWriter.h"
#include <memory>

struct AudioConfig {
    int sampleRate = 44100;     // Samples per second (44100, 48000, 96000, etc.)
    int channels = 2;           // 1 for mono, 2 for stereo
    int bitsPerSample = 16;     // 16, 24, or 32 bits
    int bufferSize = 512;       // Buffer size in samples
    
    // Constructor to set default values
    AudioConfig(int sr = 44100, int ch = 2, int bps = 16, int bs = 512) 
        : sampleRate(sr), channels(ch), bitsPerSample(bps), bufferSize(bs) {}
};

class AudioEngine {
public:
    AudioEngine(Synthesizer* synth);
    ~AudioEngine();
    
    // Initialize with specific configuration
    bool initialize(const AudioConfig& config);
    
    // Start audio processing (real-time or file output)
    bool start();
    
    // Stop audio processing
    bool stop();
    
    // Terminate the audio engine
    void terminate();
    
    // Set output to real-time audio (PortAudio)
    bool setRealTimeOutput();
    
    // Set output to WAV file
    bool setWavFileOutput(const std::string& filename);
    
    // Check status
    bool isInitialized() const { return initialized; }
    bool isPlaying() const { return playing; }
    bool isRenderingToFile() const { return renderingToFile; }
    
    // Get current configuration
    const AudioConfig& getConfig() const { return config; }
    
    // Set configuration (only possible when not initialized)
    bool setConfig(const AudioConfig& newConfig);
    
private:
    Synthesizer* synthesizer;
    std::unique_ptr<PortAudioWrapper> paWrapper;
    std::unique_ptr<WavWriter> wavWriter;
    
    AudioConfig config;
    bool initialized;
    bool playing;
    bool renderingToFile;
    std::string wavFilename;
    
    // Internal callback for processing audio data
    void processAudioBuffer(float* buffer, int framesPerBuffer);
};

#endif // AUDIO_ENGINE_H