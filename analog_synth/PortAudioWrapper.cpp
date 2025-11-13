#include "PortAudioWrapper.h"

PortAudioWrapper::PortAudioWrapper(Synthesizer* synth) : 
    synthesizer(synth), 
    stream(nullptr), 
    initialized(false), 
    playing(false) {}

PortAudioWrapper::~PortAudioWrapper() {
    if (playing) {
        stop();
    }
    if (initialized) {
        terminate();
    }
}

bool PortAudioWrapper::initialize() {
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        return false;
    }
    
    err = Pa_OpenDefaultStream(
        &stream,
        0,          // No input channels
        channels,   // 1 channel (mono) or 2 (stereo)
        paFloat32,  // 32-bit floating point output
        sampleRate,
        paFramesPerBufferUnspecified,  // Let PortAudio choose
        PortAudioWrapper::audioCallback,
        synthesizer
    );
    
    if (err != paNoError) {
        Pa_Terminate();
        return false;
    }
    
    initialized = true;
    return true;
}

bool PortAudioWrapper::start() {
    if (!initialized) {
        return false;
    }
    
    PaError err = Pa_StartStream(stream);
    if (err != paNoError) {
        return false;
    }
    
    playing = true;
    return true;
}

bool PortAudioWrapper::stop() {
    if (!initialized || !playing) {
        return false;
    }
    
    PaError err = Pa_StopStream(stream);
    if (err != paNoError) {
        return false;
    }
    
    playing = false;
    return true;
}

void PortAudioWrapper::terminate() {
    if (initialized) {
        if (playing) {
            stop();
        }
        Pa_CloseStream(stream);
        Pa_Terminate();
        initialized = false;
    }
}

int PortAudioWrapper::audioCallback(
    const void* inputBuffer,
    void* outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void* userData
) {
    Synthesizer* synth = static_cast<Synthesizer*>(userData);
    float* out = static_cast<float*>(outputBuffer);
    
    for (unsigned int i = 0; i < framesPerBuffer; i++) {
        *out++ = synth->getNextSample();  // Left channel
        // If stereo, we'd also write to the right channel:
        // *out++ = synth->getNextSample();
    }
    
    return paContinue;
}