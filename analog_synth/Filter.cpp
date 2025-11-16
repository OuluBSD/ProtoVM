#include "Filter.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Filter::Filter() : 
    a0(0), a1(0), a2(0), b1(0), b2(0),
    x1(0), x2(0), y1(0), y2(0),
    cutoff(0.5), resonance(0.5), filterType(0), sampleRate(44100) {
    updateCoefficients();
}

void Filter::setCutoff(double cutoff) {
    this->cutoff = cutoff;
    updateCoefficients();
}

void Filter::setResonance(double resonance) {
    this->resonance = resonance;
    updateCoefficients();
}

void Filter::setType(int type) {
    this->filterType = type;
    updateCoefficients();
}

void Filter::setSampleRate(int rate) {
    this->sampleRate = rate;
    updateCoefficients();
}

double Filter::processSample(double input) {
    // Apply the biquad filter formula
    double output = a0 * input + a1 * x1 + a2 * x2 - b1 * y1 - b2 * y2;
    
    // Update history
    x2 = x1;
    x1 = input;
    y2 = y1;
    y1 = output;
    
    return output;
}

void Filter::reset() {
    x1 = x2 = y1 = y2 = 0.0;
}

void Filter::updateCoefficients() {
    // Calculate coefficients for the selected filter type
    // Using the RBJ biquad filter formulas
    
    double w0 = 2 * M_PI * cutoff * sampleRate / 2;  // Convert normalized cutoff to radians per sample
    if (w0 >= M_PI) w0 = M_PI - 0.001; // Limit to Nyquist frequency
    
    double A = 1.0; // For now, no amplification
    double alpha = sin(w0) / (2 * resonance);
    double cos_w0 = cos(w0);
    
    double b0, b1, b2; // numerator coefficients
    double a0, a1, a2; // denominator coefficients
    
    switch (filterType) {
        case 0: // Lowpass
            b0 = (1 - cos_w0) / 2;
            b1 = 1 - cos_w0;
            b2 = (1 - cos_w0) / 2;
            a0 = 1 + alpha;
            a1 = -2 * cos_w0;
            a2 = 1 - alpha;
            break;
            
        case 1: // Highpass
            b0 = (1 + cos_w0) / 2;
            b1 = -(1 + cos_w0);
            b2 = (1 + cos_w0) / 2;
            a0 = 1 + alpha;
            a1 = -2 * cos_w0;
            a2 = 1 - alpha;
            break;
            
        case 2: // Bandpass
            b0 = alpha;
            b1 = 0;
            b2 = -alpha;
            a0 = 1 + alpha;
            a1 = -2 * cos_w0;
            a2 = 1 - alpha;
            break;
            
        case 3: // Notch
            b0 = 1;
            b1 = -2 * cos_w0;
            b2 = 1;
            a0 = 1 + alpha;
            a1 = -2 * cos_w0;
            a2 = 1 - alpha;
            break;
            
        default: // Default to lowpass
            b0 = (1 - cos_w0) / 2;
            b1 = 1 - cos_w0;
            b2 = (1 - cos_w0) / 2;
            a0 = 1 + alpha;
            a1 = -2 * cos_w0;
            a2 = 1 - alpha;
            break;
    }
    
    // Normalize coefficients
    this->a0 = b0 / a0;
    this->a1 = b1 / a0;
    this->a2 = b2 / a0;
    this->b1 = a1 / a0;
    this->b2 = a2 / a0;
}