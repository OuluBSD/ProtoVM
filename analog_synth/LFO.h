#ifndef LFO_H
#define LFO_H

#include "Oscillator.h"

class LFO : public Oscillator {
public:
    LFO();

    void setRate(double rate);  // Rate in Hz (typically 0.1 to 20 Hz)
    double getRate() const { return frequency; }

    // Set depth of modulation (0.0 to 1.0)
    void setDepth(double depth) { this->depth = depth; }
    double getDepth() const { return depth; }

    // Set waveform for the LFO
    using Oscillator::setWaveform;

    // Get the next sample with depth applied
    double getNextSample();

    // Set sample rate
    void setSampleRate(int rate);

    // Getters
    double getDepth() const { return depth; }

private:
    double depth;      // Modulation depth (0.0 to 1.0)
    static const int defaultSampleRate = 44100;
};

#endif // LFO_H