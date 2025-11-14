#include "ADSR.h"
#include <algorithm>

ADSR::ADSR(double attack, double decay, double sustain, double release)
    : attack_time(std::max(MIN_TIME, std::min(MAX_TIME, attack)))
    , decay_time(std::max(MIN_TIME, std::min(MAX_TIME, decay)))
    , sustain_level(std::max(MIN_LEVEL, std::min(MAX_LEVEL, sustain)))
    , release_time(std::max(MIN_TIME, std::min(MAX_TIME, release)))
    , state(ADSRState::IDLE)
    , output(0.0)
    , samples_in_current_phase(0)
    , total_samples_for_phase(0)
    , phase_increment(0.0)
    , target_level(0.0)
    , sample_rate(44100)
{
}

bool ADSR::Tick() {
    // Calculate next output based on current state
    switch (state) {
        case ADSRState::IDLE:
            output = 0.0;
            break;
            
        case ADSRState::ATTACK:
            {
                // Increment output during attack phase
                output += phase_increment;
                samples_in_current_phase++;
                
                // Check if attack phase is complete
                if (samples_in_current_phase >= total_samples_for_phase || output >= 1.0) {
                    output = 1.0;
                    state = ADSRState::DECAY;
                    samples_in_current_phase = 0;
                    
                    // Calculate decay phase parameters
                    total_samples_for_phase = static_cast<int>(decay_time * sample_rate);
                    if (total_samples_for_phase > 0) {
                        phase_increment = (sustain_level - 1.0) / total_samples_for_phase;
                    } else {
                        phase_increment = 0.0;
                    }
                    target_level = sustain_level;
                }
            }
            break;
            
        case ADSRState::DECAY:
            {
                // Decrease output during decay phase
                output += phase_increment;
                samples_in_current_phase++;
                
                // Ensure output doesn't go below sustain level
                if (samples_in_current_phase >= total_samples_for_phase || output <= sustain_level) {
                    output = sustain_level;
                    state = ADSRState::SUSTAIN;
                    samples_in_current_phase = 0;
                    phase_increment = 0.0;  // No change during sustain
                    target_level = sustain_level;
                }
            }
            break;
            
        case ADSRState::SUSTAIN:
            // Output remains at sustain level until note is released
            output = sustain_level;
            break;
            
        case ADSRState::RELEASE:
            {
                // Decrease output during release phase
                output += phase_increment;
                samples_in_current_phase++;
                
                // Check if release phase is complete
                if (samples_in_current_phase >= total_samples_for_phase || output <= 0.0) {
                    output = 0.0;
                    state = ADSRState::IDLE;
                    samples_in_current_phase = 0;
                    phase_increment = 0.0;
                }
            }
            break;
    }
    
    return true;
}

void ADSR::NoteOn() {
    state = ADSRState::ATTACK;
    samples_in_current_phase = 0;
    output = 0.0;  // Start at 0
    
    // Calculate attack phase parameters
    total_samples_for_phase = static_cast<int>(attack_time * sample_rate);
    if (total_samples_for_phase > 0) {
        phase_increment = (1.0 - output) / total_samples_for_phase;
    } else {
        phase_increment = 0.0;
    }
    target_level = 1.0;
}

void ADSR::NoteOff() {
    if (state != ADSRState::IDLE && state != ADSRState::RELEASE) {
        state = ADSRState::RELEASE;
        samples_in_current_phase = 0;
        
        // Calculate release phase parameters
        total_samples_for_phase = static_cast<int>(release_time * sample_rate);
        if (total_samples_for_phase > 0) {
            phase_increment = (0.0 - output) / total_samples_for_phase;
        } else {
            phase_increment = 0.0;
        }
    }
}

void ADSR::SetAttack(double attack) {
    this->attack_time = std::max(MIN_TIME, std::min(MAX_TIME, attack));
}

void ADSR::SetDecay(double decay) {
    this->decay_time = std::max(MIN_TIME, std::min(MAX_TIME, decay));
}

void ADSR::SetSustain(double sustain) {
    this->sustain_level = std::max(MIN_LEVEL, std::min(MAX_LEVEL, sustain));
}

void ADSR::SetRelease(double release) {
    this->release_time = std::max(MIN_TIME, std::min(MAX_TIME, release));
}

void ADSR::SetSampleRate(int rate) {
    this->sample_rate = rate;
}