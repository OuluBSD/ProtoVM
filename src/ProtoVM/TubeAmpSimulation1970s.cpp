#include "TubeAmpSimulation1970s.h"
#include "Common.h"
#include <algorithm>
#include <cmath>

// TubeAmpSimulation1970s implementation
TubeAmpSimulation1970s::TubeAmpSimulation1970s()
    : gain(80.0)  // Very high gain for 1970s high-gain sound
    , bass(1.2)
    , mid(0.7)    // Often scooped for high-gain 70s sound
    , treble(1.5)
    , presence(1.2)   // More presence for high-gain clarity
    , resonance(1.1)  // More resonance for high-gain response
    , power_level(0.6)  // Moderate power for tight bass
    , input_level(1.0)
    , output_level(1.0)
    , cab_simulation_enabled(true)
    , high_gain_mode(true)  // High-gain master volume setup
    , phase_inverter_gain(1.0)
    , output_transformer_coupling(0.88)  // Different coupling for high-gain OTs
    , input_signal(0.0)
    , output_signal(0.0)
    , power_amp_signal(0.0)
    , sample_rate(44100.0)
    , dt(1.0 / 44100.0)
{
    InitializeAmp();
}

TubeAmpSimulation1970s::~TubeAmpSimulation1970s() {
    // Destructor handled by member destructors
}

void TubeAmpSimulation1970s::InitializeAmp() {
    // Initialize tubes for 1970s American high-gain era simulation
    // 1970s high-gain amps had many preamp stages and master volume controls
    tubes.push_back(std::make_unique<Triode>(100000.0, 100000.0, 1.6e-3));  // Preamp stage 1 (V1)
    tubes.push_back(std::make_unique<Triode>(100000.0, 100000.0, 1.6e-3));  // Preamp stage 2 (V2) 
    tubes.push_back(std::make_unique<Triode>(100000.0, 100000.0, 1.6e-3));  // Preamp stage 3 - extra gain stage
    tubes.push_back(std::make_unique<Triode>(470000.0, 100000.0, 8.0e-4));  // Phase inverter (V4)
    tubes.push_back(std::make_unique<Pentode>(80000.0, 10000.0, 1.8e-3, 0.5, 15.0));  // Power amp left
    tubes.push_back(std::make_unique<Pentode>(80000.0, 10000.0, 1.8e-3, 0.5, 15.0));  // Power amp right
    
    // Set initial gains for each preamp stage (very high for 1970s high-gain sound)
    preamp_stage_gains.push_back(50.0);  // High gain for V1
    preamp_stage_gains.push_back(40.0);  // High gain for V2 
    preamp_stage_gains.push_back(30.0);  // High gain for V3 (extra stage for high-gain)
    preamp_stage_gains.push_back(5.0);   // Lower gain for phase inverter
    
    // Initialize tone stack state
    tone_stack_state[0] = 0.0;  // Bass state
    tone_stack_state[1] = 0.0;  // Mid state  
    tone_stack_state[2] = 0.0;  // Treble state
    
    // Initialize cabinet response for 1970s American high-gain style
    if (cab_simulation_enabled) {
        cabinet_response.resize(64, 0.0);
        cabinet_delay.resize(32, 0.0);
        
        // Set up cabinet response for 1970s high-gain style (e.g., using high-powered speakers)
        for (int i = 0; i < 64; i++) {
            double freq = (double)i * 20000.0 / 64.0;  // 0 to 20kHz
            // Response simulating high-gain guitar speaker setup (e.g., efficient 100W+ cabs)
            if (freq < 80.0) {
                cabinet_response[i] = 0.9;    // Good low-end extension
            } else if (freq < 200.0) {
                cabinet_response[i] = 1.05;   // Slight low-mid emphasis
            } else if (freq < 400.0) {
                cabinet_response[i] = 1.0;    // Flat region
            } else if (freq < 800.0) {
                cabinet_response[i] = 0.7;    // Mid scoop for high-gain clarity
            } else if (freq < 2000.0) {
                cabinet_response[i] = 0.8;    // Dip before presence region
            } else if (freq < 5000.0) {
                cabinet_response[i] = 1.15;   // Presence peak
            } else {
                cabinet_response[i] = 0.8;    // Rolloff at extreme highs
            }
        }
    }
}

bool TubeAmpSimulation1970s::Tick() {
    ProcessSignal();
    return true;
}

void TubeAmpSimulation1970s::ProcessSignal() {
    // Apply input level scaling
    double signal = input_signal * input_level;
    
    // Process through preamp stages (with very high gain for 1970s high-gain sound)
    ProcessPreamp();
    
    // Process through phase inverter
    ProcessPhaseInverter();
    
    // Process through power amp stage (with master volume control for 1970s high-gain)
    ProcessPowerAmp();
    
    // Process through tone stack (which is after phase inverter in most circuits)
    ProcessToneStack();
    
    // Apply cabinet simulation if enabled
    if (cab_simulation_enabled) {
        ProcessCabinetSimulation();
    }
    
    // Apply final output level scaling
    output_signal = power_amp_signal * output_level;
    
    // Apply soft limiting to prevent clipping
    if (output_signal > 5.0) output_signal = 5.0;
    if (output_signal < -5.0) output_signal = -5.0;
}

void TubeAmpSimulation1970s::ProcessPreamp() {
    // Apply the first preamp stage (very high gain for 1970s high-gain sound)
    double signal = input_signal * input_level * 1.2;  // High input gain for high-gain style
    
    if (!tubes.empty()) {
        for (int i = 0; i < 3 && i < tubes.size(); i++) {  // Process through first 3 preamp stages
            auto& tube = tubes[i];
            double gain_factor = 0.005;  // Higher factor for high-gain
            
            // Apply signal to grid with gain specific to high-gain design
            double grid_voltage = -1.0 + signal * preamp_stage_gains[i] * gain_factor;
            tube->SetGridVoltage(grid_voltage);
            tube->SetPlateVoltage(275.0);  // Slightly higher voltage for high-gain design
            tube->SetCathodeVoltage(0.0);
            tube->CalculateTubeBehavior();
            
            // Get the amplified signal
            signal = tube->GetPlateCurrent() * 0.15;  // Higher plate current factor for high-gain
            
            // Apply soft limiting to simulate tube saturation (1970s high-gain amps had more stages)
            double saturation_factor = 1.5 + (i * 0.5) + (gain / MAX_GAIN) * 2.0;  // More saturation at higher gains
            signal = tanh(signal * saturation_factor) / saturation_factor;
        }
    }
    
    // Update the input signal for next stages
    input_signal = signal;
}

void TubeAmpSimulation1970s::ProcessPhaseInverter() {
    // In a 1970s high-gain style phase inverter (often different from earlier designs)
    if (tubes.size() > 3) {
        auto& phase_inv_tube = tubes[3];
        double input = input_signal;
        
        // Apply to phase inverter (high-gain designs had more robust phase inverters)
        double grid_voltage = -1.0 + input * phase_inverter_gain * 0.03;  // Different gain factor
        phase_inv_tube->SetGridVoltage(grid_voltage);
        phase_inv_tube->SetPlateVoltage(275.0);  // Higher voltage for high-gain design
        phase_inv_tube->SetCathodeVoltage(0.0);
        phase_inv_tube->CalculateTubeBehavior();
        
        // For a simplified model
        input = phase_inv_tube->GetPlateCurrent() * 0.1;
        input = tanh(input * 1.8) * 0.55;  // Apply high-gain phase inverter characteristics
    }
}

void TubeAmpSimulation1970s::ProcessPowerAmp() {
    // Process through power amp stage (with master volume for 1970s high-gain design)
    if (tubes.size() > 4) {
        // Get both power tubes
        auto& power_tube1 = tubes[4];
        auto& power_tube2 = tubes[5];
        
        // Apply the input to both power tubes (push-pull configuration)
        double input_signal_scaled = input_signal * 0.08;  // Different scaling for high-gain
        
        // Apply to both tubes in push-pull
        double grid1 = -1.0 + input_signal_scaled * 0.5;
        double grid2 = -1.0 - input_signal_scaled * 0.5;  // Inverted
        
        power_tube1->SetGridVoltage(grid1);
        power_tube1->SetPlateVoltage(420.0);  // Higher voltage for high-gain power section
        power_tube1->SetCathodeVoltage(0.0);
        power_tube1->CalculateTubeBehavior();
        
        power_tube2->SetGridVoltage(grid2);
        power_tube2->SetPlateVoltage(420.0);
        power_tube2->SetCathodeVoltage(0.0);
        power_tube2->CalculateTubeBehavior();
        
        // Combine outputs from both tubes
        double current1 = power_tube1->GetPlateCurrent();
        double current2 = power_tube2->GetPlateCurrent();
        
        // Simulate push-pull output with high-gain characteristics
        power_amp_signal = (current1 - current2) * 0.08;  // Different factor for high-gain
        
        // Apply power amp saturation based on power level (1970s high-gain have tight bass)
        double saturation_factor = 2.0 + power_level * 2.0;  // 2.0 to 4.0 range
        power_amp_signal = tanh(power_amp_signal * saturation_factor) / saturation_factor;
        
        // Apply power amp compression (1970s high-gain have tight bass response)
        double compression_factor = 0.85 - power_level * 0.15;  // Tight compression for high-gain
        if (fabs(power_amp_signal) > 0.55) {
            power_amp_signal = power_amp_signal * compression_factor + 
                              (power_amp_signal > 0 ? 0.55 : -0.55) * (1.0 - compression_factor);
        }
        
        // Apply master volume control (characteristic of 1970s high-gain designs)
        if (high_gain_mode) {
            power_amp_signal *= 0.7;  // Master volume reduces power amp output
        }
    }
}

void TubeAmpSimulation1970s::ProcessToneStack() {
    // Simulate the tone stack of a typical 1970s high-gain amp
    // 1970s high-gain designs often had more complex tone stacks
    
    // Apply tone controls by modifying the signal
    // 1970s high-gain amps had very interactive and sometimes complex tone controls
    double bass_factor = (bass - 1.0) * 0.6 + 1.0;  // -0.6 to 1.6 range around 1.0
    double mid_factor = (mid - 1.0) * 0.75 + 1.0;   // -0.75 to 2.5 range - often scooped
    double treble_factor = (treble - 1.0) * 0.6 + 1.0;  // -0.6 to 1.6 range around 1.0
    
    // Apply frequency-dependent tone controls with high-gain characteristics
    double adjusted_signal = power_amp_signal;
    
    // Apply bass (often tighter in high-gain designs)
    adjusted_signal *= (0.8 + 0.6 * bass_factor);
    
    // Apply the characteristic mid scoop of 1970s high-gain amps
    adjusted_signal *= (0.6 + 0.8 * mid_factor);
    
    // Apply treble (often more present for clarity in high-gain)
    adjusted_signal *= (0.75 + 0.5 * treble_factor);
    
    power_amp_signal = adjusted_signal;
    
    // Apply presence control (very important in high-gain designs for clarity)
    if (presence > 0.0) {
        // Add significant high-frequency emphasis for clarity in high-gain
        power_amp_signal *= (1.0 + presence * 0.6);  // More presence than earlier eras
    }
    
    // Apply resonance control (output transformer resonance)
    if (resonance > 0.0) {
        // Add output transformer resonance characteristics
        power_amp_signal *= (1.0 + resonance * 0.3);  // More resonance for high-gain response
    }
}

void TubeAmpSimulation1970s::ProcessCabinetSimulation() {
    // Apply simplified cabinet simulation for 1970s high-gain sound
    if (cab_simulation_enabled && !cabinet_response.empty()) {
        // Apply the frequency response characteristic of 1970s high-gain cabs
        static double lp_state = 0.0;
        static double hp_state = 0.0;
        
        // Apply low-pass filter with high-gain characteristics
        double lp_coeff = 0.12;  // Different from earlier designs
        power_amp_signal = lp_state + lp_coeff * (power_amp_signal - lp_state);
        lp_state = power_amp_signal;
        
        // Apply some additional coloration to simulate the tight bass of 70s cabs
        power_amp_signal = power_amp_signal * 0.8 + 
                          sin(power_amp_signal * 10.0) * 0.04 +  // Harmonic content
                          cos(power_amp_signal * 15.0) * 0.02;   // Complex harmonics
    }
}

double TubeAmpSimulation1970s::CalculateToneStackResponse(double input, double bass, double mid, double treble) {
    // Simplified tone stack calculation for 1970s high-gain style
    double response = input;
    
    // Apply tone controls typical of 1970s high-gain designs (often with mid-scoop)
    response *= (0.6 + 0.8 * bass);
    response *= (0.4 + 1.2 * mid);    // Often scooped for high-gain sound
    response *= (0.7 + 0.6 * treble);
    
    return response;
}

void TubeAmpSimulation1970s::SetGain(double gain) {
    this->gain = std::max(MIN_GAIN, std::min(MAX_GAIN, gain));
    
    // Update preamp stage gains based on overall gain for high-gain characteristics
    if (!preamp_stage_gains.empty()) {
        // 1970s high-gain amps had many gain stages
        preamp_stage_gains[0] = 35.0 + gain * 0.8;  // First stage gets substantial gain
        if (preamp_stage_gains.size() > 1) {
            preamp_stage_gains[1] = 25.0 + gain * 0.5;  // Second stage gets more gain
        }
        if (preamp_stage_gains.size() > 2) {
            preamp_stage_gains[2] = 15.0 + gain * 0.3;  // Third stage for extra gain
        }
    }
}

void TubeAmpSimulation1970s::SetToneControls(double bass, double mid, double treble) {
    this->bass = std::max(MIN_BASS, std::min(MAX_BASS, bass));
    this->mid = std::max(MIN_MID, std::min(MAX_MID, mid));  // Often scooped in 70s high-gain
    this->treble = std::max(MIN_TREBLE, std::min(MAX_TREBLE, treble));
}

void TubeAmpSimulation1970s::SetPresence(double presence) {
    this->presence = std::max(MIN_PRESENCE, std::min(MAX_PRESENCE, presence));
}

void TubeAmpSimulation1970s::SetResonance(double resonance) {
    this->resonance = std::max(MIN_RESONANCE, std::min(MAX_RESONANCE, resonance));
}

void TubeAmpSimulation1970s::SetPowerLevel(double level) {
    this->power_level = std::max(MIN_POWER_LEVEL, std::min(MAX_POWER_LEVEL, level));
}

void TubeAmpSimulation1970s::SetInputLevel(double level) {
    this->input_level = std::max(MIN_LEVEL, std::min(MAX_LEVEL, level));
}

void TubeAmpSimulation1970s::SetOutputLevel(double level) {
    this->output_level = std::max(MIN_LEVEL, std::min(MAX_LEVEL, level));
}

void TubeAmpSimulation1970s::SetCabSimulation(bool enabled) {
    this->cab_simulation_enabled = enabled;
}

void TubeAmpSimulation1970s::SetHighGainMode(bool high_gain) {
    this->high_gain_mode = high_gain;
}