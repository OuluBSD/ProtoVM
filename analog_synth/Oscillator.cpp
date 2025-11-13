#include "Oscillator.h"

Oscillator::Oscillator() : frequency(440.0), amplitude(1.0), waveform(Waveform::SINE), phase(0.0) {}

void Oscillator::setFrequency(double freq) {
    frequency = freq;
}

void Oscillator::setWaveform(Waveform wf) {
    waveform = wf;
}

void Oscillator::setAmplitude(double amp) {
    amplitude = amp;
}

double Oscillator::getNextSample() {
    double sample = 0.0;
    double sampleRate = 44100.0; // Default sample rate, should be configurable
    
    switch (waveform) {
        case Waveform::SINE:
            sample = amplitude * sin(phase);
            break;
            
        case Waveform::SAWTOOTH:
            // Sawtooth from -1 to 1
            sample = amplitude * (2.0 * (phase / TWO_PI) - 1.0);
            break;
            
        case Waveform::SQUARE:
            sample = amplitude * (sin(phase) >= 0 ? 1.0 : -1.0);
            break;
            
        case Waveform::TRIANGLE:
            // Triangle wave
            sample = amplitude * (2.0 * fabs(2.0 * (phase / TWO_PI) - 1.0) - 1.0);
            break;
    }
    
    // Update phase for next sample
    phase += (TWO_PI * frequency) / sampleRate;
    if (phase >= TWO_PI) {
        phase -= TWO_PI;
    }
    
    return sample;
}

void Oscillator::setPhase(double p) {
    phase = p;
    // Normalize phase to [0, 2*PI)
    while (phase >= TWO_PI) phase -= TWO_PI;
    while (phase < 0) phase += TWO_PI;
}