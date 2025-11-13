#include "ADSR.h"

ADSR::ADSR() : 
    attackTime(0.1), 
    decayTime(0.2), 
    sustainLevel(0.7), 
    releaseTime(0.3),
    state(ADSRState::IDLE),
    currentLevel(0.0),
    sampleCount(0) {}

void ADSR::setAttack(double attack) {
    attackTime = attack;
}

void ADSR::setDecay(double decay) {
    decayTime = decay;
}

void ADSR::setSustain(double sustain) {
    sustainLevel = sustain;
}

void ADSR::setRelease(double release) {
    releaseTime = release;
}

void ADSR::noteOn() {
    state = ADSRState::ATTACK;
    sampleCount = 0;
}

void ADSR::noteOff() {
    if (state != ADSRState::IDLE && state != ADSRState::RELEASE) {
        state = ADSRState::RELEASE;
        sampleCount = 0;
    }
}

double ADSR::getNextSample() {
    double increment = 0.0;
    int segmentSamples = 0;
    
    switch (state) {
        case ADSRState::ATTACK:
            segmentSamples = static_cast<int>(attackTime * sampleRate);
            if (segmentSamples > 0) {
                increment = 1.0 / segmentSamples;
                currentLevel += increment;
            } else {
                currentLevel = 1.0;
            }
            
            if (currentLevel >= 1.0) {
                currentLevel = 1.0;
                state = ADSRState::DECAY;
                sampleCount = 0;
            }
            break;
            
        case ADSRState::DECAY:
            segmentSamples = static_cast<int>(decayTime * sampleRate);
            if (segmentSamples > 0) {
                increment = (1.0 - sustainLevel) / segmentSamples;
                currentLevel -= increment;
            } else {
                currentLevel = sustainLevel;
            }
            
            if (currentLevel <= sustainLevel) {
                currentLevel = sustainLevel;
                state = ADSRState::SUSTAIN;
            }
            break;
            
        case ADSRState::SUSTAIN:
            // Level stays constant at sustain level
            currentLevel = sustainLevel;
            break;
            
        case ADSRState::RELEASE:
            segmentSamples = static_cast<int>(releaseTime * sampleRate);
            if (segmentSamples > 0) {
                increment = sustainLevel / segmentSamples;
                currentLevel -= increment;
            } else {
                currentLevel = 0.0;
            }
            
            if (currentLevel <= 0.0) {
                currentLevel = 0.0;
                state = ADSRState::IDLE;
            }
            break;
            
        case ADSRState::IDLE:
            currentLevel = 0.0;
            break;
    }
    
    sampleCount++;
    return currentLevel;
}

bool ADSR::isActive() const {
    return state != ADSRState::IDLE;
}