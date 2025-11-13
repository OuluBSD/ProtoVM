#ifndef PORTAUDIO_WRAPPER_H
#define PORTAUDIO_WRAPPER_H

#include "Synthesizer.h"

extern "C" {
    #include "portaudio.h"
}

class PortAudioWrapper {
public:
    PortAudioWrapper(Synthesizer* synth);
    ~PortAudioWrapper();
    
    bool initialize();
    bool start();
    bool stop();
    void terminate();
    
    bool isInitialized() const { return initialized; }
    bool isPlaying() const { return playing; }
    
private:
    static int audioCallback(
        const void* inputBuffer,
        void* outputBuffer,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags,
        void* userData
    );
    
    Synthesizer* synthesizer;
    PaStream* stream;
    bool initialized;
    bool playing;
    
    static const int sampleRate = 44100;
    static const int channels = 1;  // Mono for now
};

#endif // PORTAUDIO_WRAPPER_H