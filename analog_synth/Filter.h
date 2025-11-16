#ifndef FILTER_H
#define FILTER_H

class Filter {
public:
    Filter();
    
    // Set filter parameters
    void setCutoff(double cutoff);      // Normalized frequency (0.0 to 1.0)
    void setResonance(double resonance); // Q factor or resonance (0.1 to 10.0)
    void setType(int type);             // 0=lowpass, 1=hipass, 2=bandpass, 3=notch
    
    // Process a single sample
    double processSample(double input);
    
    // Set sample rate for internal calculations
    void setSampleRate(int rate);
    
    // Reset the filter state
    void reset();
    
    // Getter methods
    double getCutoff() const { return cutoff; }
    double getResonance() const { return resonance; }
    int getType() const { return filterType; }
    
private:
    // Filter coefficients
    double a0, a1, a2, b1, b2;
    
    // Input history (for IIR filter)
    double x1, x2, y1, y2;
    
    // Filter parameters
    double cutoff, resonance;
    int filterType;
    
    // Sample rate
    int sampleRate;
    
    // Update filter coefficients based on parameters
    void updateCoefficients();
};

#endif // FILTER_H