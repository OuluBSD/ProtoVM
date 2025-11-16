#include "LFO.h"

LFO::LFO() : depth(1.0) {
    // By default, set a slow LFO rate
    setRate(2.0);  // 2 Hz
}

void LFO::setRate(double rate) {
    setFrequency(rate);
}

double LFO::getNextSample() {
    // Get the raw oscillator sample (-1 to 1)
    double sample = Oscillator::getNextSample();
    
    // Apply depth scaling (convert from -depth to +depth)
    return sample * depth;
}

void LFO::setSampleRate(int rate) {
    // This would update the sample rate for calculations if needed
    // Currently, Oscillator handles this internally
}