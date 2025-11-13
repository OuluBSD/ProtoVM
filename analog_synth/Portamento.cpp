#include "Portamento.h"
#include <cmath>

Portamento::Portamento() : 
    time(0.1), 
    enabled(false), 
    targetFreq(440.0), 
    currentFreq(440.0), 
    stepSize(0.0),
    active(false) {}

void Portamento::setTime(double t) {
    time = t;
    // Update step size based on new time
    if (time > 0) {
        int steps = static_cast<int>(time * sampleRate);
        stepSize = (targetFreq - currentFreq) / steps;
    }
}

void Portamento::setEnabled(bool en) {
    enabled = en;
}

void Portamento::setTargetFrequency(double freq) {
    targetFreq = freq;
    if (enabled) {
        int steps = static_cast<int>(time * sampleRate);
        if (steps > 0) {
            stepSize = (targetFreq - currentFreq) / steps;
            active = true;
        } else {
            currentFreq = targetFreq;  // Immediate change if no time
            active = false;
        }
    } else {
        currentFreq = targetFreq;  // No glide if disabled
        active = false;
    }
}

void Portamento::setCurrentFrequency(double freq) {
    currentFreq = freq;
    // Update step size since current frequency changed
    if (time > 0 && enabled) {
        int steps = static_cast<int>(time * sampleRate);
        stepSize = (targetFreq - currentFreq) / steps;
        active = true;
    }
}

double Portamento::getNextFrequency() {
    if (!enabled || !active) {
        currentFreq = targetFreq;
        active = false;
        return currentFreq;
    }
    
    // Move current frequency towards target
    if (std::abs(currentFreq - targetFreq) < std::abs(stepSize)) {
        // We're close enough to target
        currentFreq = targetFreq;
        active = false;
    } else {
        currentFreq += stepSize;
    }
    
    return currentFreq;
}