#ifndef ADSR_H
#define ADSR_H

enum class ADSRState {
    IDLE,
    ATTACK,
    DECAY,
    SUSTAIN,
    RELEASE
};

class ADSR {
public:
    ADSR();
    
    // Set parameters in seconds
    void setAttack(double attack);
    void setDecay(double decay);
    void setSustain(double sustain);  // Level from 0.0 to 1.0
    void setRelease(double release);
    
    // Trigger the envelope
    void noteOn();
    void noteOff();
    
    // Get the next sample of the envelope
    double getNextSample();
    
    // Check if the envelope is still active
    bool isActive() const;
    
private:
    double attackTime;      // Time in seconds
    double decayTime;       // Time in seconds
    double sustainLevel;    // Level from 0.0 to 1.0
    double releaseTime;     // Time in seconds
    
    ADSRState state;
    double currentLevel;
    int sampleCount;
    
    // Sample rate - should be configurable
    static const int sampleRate = 44100;
};

#endif // ADSR_H