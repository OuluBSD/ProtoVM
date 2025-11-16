#include "TubeAmpSimulation1990s.h"
#include "Common.h"
#include <algorithm>
#include <cmath>

// TubeAmpSimulation1990s implementation
TubeAmpSimulation1990s::TubeAmpSimulation1990s()
    : gain(45.0)  // Medium-high gain for alternative/grunge sound
    , bass(0.9)
    , mid(1.6)    // Emphasized midrange for alternative rock
    , treble(0.8) // Not too bright for grunge sound
    , presence(0.7)   // Moderate presence
    , resonance(0.8)  // Moderate resonance for alternative sound
    , power_level(0.6)  // Moderate power for grunge saturation
    , input_level(1.0)
    , output_level(1.0)
    , cab_simulation_enabled(true)
    , alt_mode(true)  // Alternative rock mode
    , phase_inverter_gain(1.0)
    , output_transformer_coupling(0.90)  // Balanced coupling for alternative sound
    , input_signal(0.0)
    , output_signal(0.0)
    , power_amp_signal(0.0)
    , sample_rate(44100.0)
    , dt(1.0 / 44100.0)
{
    InitializeAmp();
}

TubeAmpSimulation1990s::~TubeAmpSimulation1990s() {
    // Destructor handled by member destructors
}

void TubeAmpSimulation1990s::InitializeAmp() {
    // Initialize tubes for 1990s alternative rock era simulation
    // 1990s alternative rock amps often had medium gain with strong midrange
    tubes.push_back(std::make_unique<Triode>(100000.0, 100000.0, 1.6e-3));  // Preamp stage 1 (V1)
    tubes.push_back(std::make_unique<Triode>(100000.0, 100000.0, 1.6e-3));  // Preamp stage 2 (V2)
    tubes.push_back(std::make_unique<Triode>(470000.0, 100000.0, 8.0e-4));  // Phase inverter (V3)
    tubes.push_back(std::make_unique<Pentode>(90000.0, 9000.0, 1.7e-3, 0.45, 13.0));  // Power amp left
    tubes.push_back(std::make_unique<Pentode>(90000.0, 9000.0, 1.7e-3, 0.45, 13.0));  // Power amp right
    
    // Set initial gains for each preamp stage (medium-high for alternative sound)
    preamp_stage_gains.push_back(40.0);  // Medium-high gain for V1
    preamp_stage_gains.push_back(25.0);  // Medium gain for V2 
    preamp_stage_gains.push_back(5.0);   // Lower gain for phase inverter
    
    // Initialize tone stack state
    tone_stack_state[0] = 0.0;  // Bass state
    tone_stack_state[1] = 0.0;  // Mid state  
    tone_stack_state[2] = 0.0;  // Treble state
    
    // Initialize cabinet response for 1990s alternative rock style
    if (cab_simulation_enabled) {
        cabinet_response.resize(64, 0.0);
        cabinet_delay.resize(32, 0.0);
        
        // Set up cabinet response for 1990s alternative sound (balanced with mid emphasis)
        for (int i = 0; i < 64; i++) {
            double freq = (double)i * 20000.0 / 64.0;  // 0 to 20kHz
            // Response simulating 1990s alternative rock cab (e.g., Mesa/Boogie, Bogner)
            if (freq < 80.0) {
                cabinet_response[i] = 0.8;    // Controlled low-end (not too boomy)
            } else if (freq < 200.0) {
                cabinet_response[i] = 0.9;    // Tight low-mid
            } else if (freq < 500.0) {
                cabinet_response[i] = 0.95;   // Flat region
            } else if (freq < 1000.0) {
                cabinet_response[i] = 1.15;   // Mid emphasis characteristic of alternative rock
            } else if (freq < 2000.0) {
                cabinet_response[i] = 1.1;    // Sustained mid presence
            } else if (freq < 5000.0) {
                cabinet_response[i] = 0.9;    // Controlled highs
            } else {
                cabinet_response[i] = 0.75;   // Gentle high rolloff
            }
        }
    }
}

bool TubeAmpSimulation1990s::Tick() {
    ProcessSignal();
    return true;
}

void TubeAmpSimulation1990s::ProcessSignal() {
    // Apply input level scaling
    double signal = input_signal * input_level;
    
    // Process through preamp stages (with medium-high gain for alternative sound)
    ProcessPreamp();
    
    // Process through phase inverter
    ProcessPhaseInverter();
    
    // Process through power amp stage (with alternative rock characteristics)
    ProcessPowerAmp();
    
    // Process through tone stack
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

void TubeAmpSimulation1990s::ProcessPreamp() {
    // Apply the first preamp stage (medium-high gain for alternative sound)
    double signal = input_signal * input_level * 1.1;  // Medium input gain
    
    if (!tubes.empty()) {
        for (int i = 0; i < 2 && i < tubes.size(); i++) {  // Process through first 2 preamp stages
            auto& tube = tubes[i];
            double gain_factor = 0.007;  // Medium factor for alternative sound
            
            // Apply signal to grid with gain specific to alternative design
            double grid_voltage = -1.0 + signal * preamp_stage_gains[i] * gain_factor;
            tube->SetGridVoltage(grid_voltage);
            tube->SetPlateVoltage(280.0);  // Medium voltage for alternative design
            tube->SetCathodeVoltage(0.0);
            tube->CalculateTubeBehavior();
            
            // Get the amplified signal
            signal = tube->GetPlateCurrent() * 0.13;  // Medium plate current factor
            
            // Apply soft limiting to simulate tube saturation (alternative focused on grittier sound)
            double saturation_factor = 2.0 + (gain / MAX_GAIN) * 1.8;  // Medium saturation for alternative
            signal = tanh(signal * saturation_factor) / saturation_factor;
        }
    }
    
    // Update the input signal for next stages
    input_signal = signal;
}

void TubeAmpSimulation1990s::ProcessPhaseInverter() {
    // In a 1990s alternative style phase inverter
    if (tubes.size() > 2) {
        auto& phase_inv_tube = tubes[2];
        double input = input_signal;
        
        // Apply to phase inverter (alternative designs had robust phase inverters)
        double grid_voltage = -1.0 + input * phase_inverter_gain * 0.04;  // Alternative gain factor
        phase_inv_tube->SetGridVoltage(grid_voltage);
        phase_inv_tube->SetPlateVoltage(280.0);  // Medium voltage for alternative design
        phase_inv_tube->SetCathodeVoltage(0.0);
        phase_inv_tube->CalculateTubeBehavior();
        
        // For a simplified model
        input = phase_inv_tube->GetPlateCurrent() * 0.1;
        input = tanh(input * 2.0) * 0.5;  // Apply alternative phase inverter characteristics
    }
}

void TubeAmpSimulation1990s::ProcessPowerAmp() {
    // Process through power amp stage (with alternative rock characteristics)
    if (tubes.size() > 3) {
        // Get both power tubes
        auto& power_tube1 = tubes[3];
        auto& power_tube2 = tubes[4];
        
        // Apply the input to both power tubes (push-pull configuration)
        double input_signal_scaled = input_signal * 0.09;  // Alternative scaling
        
        // Apply to both tubes in push-pull
        double grid1 = -1.0 + input_signal_scaled * 0.5;
        double grid2 = -1.0 - input_signal_scaled * 0.5;  // Inverted
        
        power_tube1->SetGridVoltage(grid1);
        power_tube1->SetPlateVoltage(430.0);  // Higher voltage for alternative design
        power_tube1->SetCathodeVoltage(0.0);
        power_tube1->CalculateTubeBehavior();
        
        power_tube2->SetGridVoltage(grid2);
        power_tube2->SetPlateVoltage(430.0);
        power_tube2->SetCathodeVoltage(0.0);
        power_tube2->CalculateTubeBehavior();
        
        // Combine outputs from both tubes
        double current1 = power_tube1->GetPlateCurrent();
        double current2 = power_tube2->GetPlateCurrent();
        
        // Simulate push-pull output with alternative characteristics
        power_amp_signal = (current1 - current2) * 0.075;  // Alternative factor
        
        // Apply power amp saturation based on power level (alternative focused on controlled grit)
        double saturation_factor = 2.2 + power_level * 1.8;  // 2.2 to 4.0 range
        power_amp_signal = tanh(power_amp_signal * saturation_factor) / saturation_factor;
        
        // Apply power amp compression (1990s alternative had controlled, tight response)
        double compression_factor = 0.85 - power_level * 0.1;  // Controlled compression for alternative
        if (fabs(power_amp_signal) > 0.55) {
            power_amp_signal = power_amp_signal * compression_factor + 
                              (power_amp_signal > 0 ? 0.55 : -0.55) * (1.0 - compression_factor);
        }
    }
}

void TubeAmpSimulation1990s::ProcessToneStack() {
    // Simulate the tone stack of a typical 1990s alternative rock amp
    // 1990s alternative designs emphasized midrange for that characteristic sound
    
    // Apply tone controls by modifying the signal
    // 1990s alternative tone stacks emphasized midrange presence
    double bass_factor = (bass - 1.0) * 0.4 + 1.0;  // -0.4 to 1.4 range around 1.0
    double mid_factor = (mid - 1.0) * 0.7 + 1.0;    // -0.7 to 2.4 range - emphasized mids
    double treble_factor = (treble - 1.0) * 0.4 + 1.0;  // -0.4 to 1.4 range - controlled highs
    
    // Apply frequency-dependent tone controls with alternative characteristics
    double adjusted_signal = power_amp_signal;
    
    // Apply tight bass (characteristic of alternative rock)
    adjusted_signal *= (0.8 + 0.5 * bass_factor);
    
    // Apply emphasized midrange (the signature alternative rock sound)
    adjusted_signal *= (0.7 + 0.8 * mid_factor);
    
    // Apply controlled treble (not too bright for alternative sound)
    adjusted_signal *= (0.85 + 0.3 * treble_factor);
    
    power_amp_signal = adjusted_signal;
    
    // Apply presence control (important for alternative rock clarity)
    if (presence > 0.0) {
        // Add presence for clarity in alternative sound
        power_amp_signal *= (1.0 + presence * 0.3);  // Moderate presence
    }
    
    // Apply resonance control (alternative OT resonance)
    if (resonance > 0.0) {
        // Add output transformer resonance characteristics
        power_amp_signal *= (1.0 + resonance * 0.25);  // Moderate resonance for alternative
    }
}

void TubeAmpSimulation1990s::ProcessCabinetSimulation() {
    // Apply simplified cabinet simulation for 1990s alternative rock sound
    if (cab_simulation_enabled && !cabinet_response.empty()) {
        // Apply the frequency response characteristic of 1990s alternative cabs
        static double lp_state = 0.0;
        static double hp_state = 0.0;
        
        // Apply low-pass filter with alternative characteristics
        double lp_coeff = 0.12;  // Alternative response
        power_amp_signal = lp_state + lp_coeff * (power_amp_signal - lp_state);
        lp_state = power_amp_signal;
        
        // Apply some additional coloration for the alternative rock sound
        power_amp_signal = power_amp_signal * 0.85 + 
                          sin(power_amp_signal * 11.0) * 0.035 +  // Alternative harmonics
                          cos(power_amp_signal * 16.0) * 0.02;    // Complex harmonics
    }
}

double TubeAmpSimulation1990s::CalculateToneStackResponse(double input, double bass, double mid, double treble) {
    // Simplified tone stack calculation for 1990s alternative style
    double response = input;
    
    // Apply tone controls typical of 1990s alternative designs (mid emphasis)
    response *= (0.8 + 0.5 * bass);
    response *= (0.6 + 0.9 * mid);    // Mid emphasis for alternative rock
    response *= (0.8 + 0.4 * treble); // Controlled highs
    
    return response;
}

void TubeAmpSimulation1990s::SetGain(double gain) {
    this->gain = std::max(MIN_GAIN, std::min(MAX_GAIN, gain));
    
    // Update preamp stage gains based on overall gain for alternative characteristics
    if (!preamp_stage_gains.empty()) {
        // 1990s alternative amps had medium-high gains
        preamp_stage_gains[0] = 30.0 + gain * 0.4;  // First stage gets moderate gain
        if (preamp_stage_gains.size() > 1) {
            preamp_stage_gains[1] = 18.0 + gain * 0.2;  // Second stage gets less gain
        }
    }
}

void TubeAmpSimulation1990s::SetToneControls(double bass, double mid, double treble) {
    this->bass = std::max(MIN_BASS, std::min(MAX_BASS, bass));
    this->mid = std::max(MIN_MID, std::min(MAX_MID, mid));  // Emphasized midrange
    this->treble = std::max(MIN_TREBLE, std::min(MAX_TREBLE, treble));  // Controlled highs
}

void TubeAmpSimulation1990s::SetPresence(double presence) {
    this->presence = std::max(MIN_PRESENCE, std::min(MAX_PRESENCE, presence));
}

void TubeAmpSimulation1990s::SetResonance(double resonance) {
    this->resonance = std::max(MIN_RESONANCE, std::min(MAX_RESONANCE, resonance));
}

void TubeAmpSimulation1990s::SetPowerLevel(double level) {
    this->power_level = std::max(MIN_POWER_LEVEL, std::min(MAX_POWER_LEVEL, level));
}

void TubeAmpSimulation1990s::SetInputLevel(double level) {
    this->input_level = std::max(MIN_LEVEL, std::min(MAX_LEVEL, level));
}

void TubeAmpSimulation1990s::SetOutputLevel(double level) {
    this->output_level = std::max(MIN_LEVEL, std::min(MAX_LEVEL, level));
}

void TubeAmpSimulation1990s::SetCabSimulation(bool enabled) {
    this->cab_simulation_enabled = enabled;
}

void TubeAmpSimulation1990s::SetAlternativeMode(bool alt_mode) {
    this->alt_mode = alt_mode;
}