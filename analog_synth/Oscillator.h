#ifndef OSCILLATOR_H
#define OSCILLATOR_H

#include <cmath>

enum class Waveform {
    SINE,
    SAWTOOTH,
    SQUARE,
    TRIANGLE
};

class Oscillator {
public:
    Oscillator();
    
    void setFrequency(double frequency);
    void setWaveform(Waveform waveform);
    void setAmplitude(double amplitude);
    
    double getNextSample();
    
    // Phase control
    void setPhase(double phase);
    double getPhase() const { return phase; }
    
private:
    double frequency;
    double amplitude;
    Waveform waveform;
    double phase;
    static constexpr double TWO_PI = 2.0 * M_PI;
};

#endif // OSCILLATOR_H