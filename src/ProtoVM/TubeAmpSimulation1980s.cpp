#include "TubeAmpSimulation1980s.h"
#include "Common.h"
#include <algorithm>
#include <cmath>

// TubeAmpSimulation1980s implementation
TubeAmpSimulation1980s::TubeAmpSimulation1980s()
    : gain(25.0)  // Moderate gain for high headroom
    , bass(1.1)
    , mid(1.0)    // More focused midrange for 80s sound
    , treble(1.3) // Extended, clean highs
    , presence(0.9)   // Clear highs for definition
    , resonance(0.6)  // Controlled resonance for tight response
    , power_level(0.4)  // Lower power for more headroom
    , input_level(1.0)
    , output_level(1.0)
    , cab_simulation_enabled(true)
    , high_headroom_mode(true)  // High-headroom operation
    , phase_inverter_gain(1.0)
    , output_transformer_coupling(0.92)  // Clean coupling for 80s design
    , input_signal(0.0)
    , output_signal(0.0)
    , power_amp_signal(0.0)
    , sample_rate(44100.0)
    , dt(1.0 / 44100.0)
{
    InitializeAmp();
}

TubeAmpSimulation1980s::~TubeAmpSimulation1980s() {
    // Destructor handled by member destructors
}

void TubeAmpSimulation1980s::InitializeAmp() {
    // Initialize tubes for 1980s high-headroom era simulation
    // 1980s amps focused on clean headroom and tight bass response
    tubes.push_back(std::make_unique<Triode>(100000.0, 100000.0, 1.6e-3));  // Preamp stage 1 (V1)
    tubes.push_back(std::make_unique<Triode>(100000.0, 100000.0, 1.6e-3));  // Preamp stage 2 (V2)
    tubes.push_back(std::make_unique<Triode>(470000.0, 100000.0, 8.0e-4));  // Phase inverter (V3)
    tubes.push_back(std::make_unique<Pentode>(100000.0, 8000.0, 1.6e-3, 0.4, 12.0));  // Power amp left
    tubes.push_back(std::make_unique<Pentode>(100000.0, 8000.0, 1.6e-3, 0.4, 12.0));  // Power amp right
    
    // Set initial gains for each preamp stage (moderate for headroom)
    preamp_stage_gains.push_back(35.0);  // Moderate gain for V1
    preamp_stage_gains.push_back(20.0);  // Moderate gain for V2 
    preamp_stage_gains.push_back(5.0);   // Lower gain for phase inverter
    
    // Initialize tone stack state
    tone_stack_state[0] = 0.0;  // Bass state
    tone_stack_state[1] = 0.0;  // Mid state  
    tone_stack_state[2] = 0.0;  // Treble state
    
    // Initialize cabinet response for 1980s high-headroom style
    if (cab_simulation_enabled) {
        cabinet_response.resize(64, 0.0);
        cabinet_delay.resize(32, 0.0);
        
        // Set up cabinet response for 1980s style (tight, controlled bass, extended highs)
        for (int i = 0; i < 64; i++) {
            double freq = (double)i * 20000.0 / 64.0;  // 0 to 20kHz
            // Response simulating 80s clean, tight, extended response
            if (freq < 60.0) {
                cabinet_response[i] = 0.85;   // Controlled low-end
            } else if (freq < 150.0) {
                cabinet_response[i] = 0.95;   // Tight low-mid
            } else if (freq < 500.0) {
                cabinet_response[i] = 1.0;    // Neutral mid
            } else if (freq < 2000.0) {
                cabinet_response[i] = 1.05;   // Slight mid presence
            } else if (freq < 8000.0) {
                cabinet_response[i] = 1.1;    // Extended highs
            } else {
                cabinet_response[i] = 0.95;   // Slight high rolloff
            }
        }
    }
}

bool TubeAmpSimulation1980s::Tick() {
    ProcessSignal();
    return true;
}

void TubeAmpSimulation1980s::ProcessSignal() {
    // Apply input level scaling
    double signal = input_signal * input_level;
    
    // Process through preamp stages (with moderate gain for headroom)
    ProcessPreamp();
    
    // Process through phase inverter
    ProcessPhaseInverter();
    
    // Process through power amp stage (with high headroom characteristics)
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

void TubeAmpSimulation1980s::ProcessPreamp() {
    // Apply the first preamp stage (moderate gain for headroom)
    double signal = input_signal * input_level * 1.0;  // Moderate input gain
    
    if (!tubes.empty()) {
        for (int i = 0; i < 2 && i < tubes.size(); i++) {  // Process through first 2 preamp stages
            auto& tube = tubes[i];
            double gain_factor = 0.008;  // Lower factor for headroom
            
            // Apply signal to grid with gain specific to high-headroom design
            double grid_voltage = -1.0 + signal * preamp_stage_gains[i] * gain_factor;
            tube->SetGridVoltage(grid_voltage);
            tube->SetPlateVoltage(290.0);  // Higher voltage for modern design and headroom
            tube->SetCathodeVoltage(0.0);
            tube->CalculateTubeBehavior();
            
            // Get the amplified signal
            signal = tube->GetPlateCurrent() * 0.12;  // Moderate plate current factor
            
            // Apply soft limiting to simulate tube saturation (80s focused on headroom)
            double saturation_factor = 2.5 + (gain / MAX_GAIN) * 1.5;  // Less saturation for headroom
            signal = tanh(signal * saturation_factor) / saturation_factor;
        }
    }
    
    // Update the input signal for next stages
    input_signal = signal;
}

void TubeAmpSimulation1980s::ProcessPhaseInverter() {
    // In a 1980s high-headroom style phase inverter
    if (tubes.size() > 2) {
        auto& phase_inv_tube = tubes[2];
        double input = input_signal;
        
        // Apply to phase inverter (80s designs had clean phase inverters)
        double grid_voltage = -1.0 + input * phase_inverter_gain * 0.045;  // Clean gain factor
        phase_inv_tube->SetGridVoltage(grid_voltage);
        phase_inv_tube->SetPlateVoltage(290.0);  // Higher voltage for clean design
        phase_inv_tube->SetCathodeVoltage(0.0);
        phase_inv_tube->CalculateTubeBehavior();
        
        // For a simplified model
        input = phase_inv_tube->GetPlateCurrent() * 0.1;
        input = tanh(input * 2.2) * 0.45;  // Clean phase inverter characteristics
    }
}

void TubeAmpSimulation1980s::ProcessPowerAmp() {
    // Process through power amp stage (with high-headroom characteristics)
    if (tubes.size() > 3) {
        // Get both power tubes
        auto& power_tube1 = tubes[3];
        auto& power_tube2 = tubes[4];
        
        // Apply the input to both power tubes (push-pull configuration)
        double input_signal_scaled = input_signal * 0.1;  // Clean scaling for headroom
        
        // Apply to both tubes in push-pull
        double grid1 = -1.0 + input_signal_scaled * 0.5;
        double grid2 = -1.0 - input_signal_scaled * 0.5;  // Inverted
        
        power_tube1->SetGridVoltage(grid1);
        power_tube1->SetPlateVoltage(440.0);  // Higher voltage for clean operation
        power_tube1->SetCathodeVoltage(0.0);
        power_tube1->CalculateTubeBehavior();
        
        power_tube2->SetGridVoltage(grid2);
        power_tube2->SetPlateVoltage(440.0);
        power_tube2->SetCathodeVoltage(0.0);
        power_tube2->CalculateTubeBehavior();
        
        // Combine outputs from both tubes
        double current1 = power_tube1->GetPlateCurrent();
        double current2 = power_tube2->GetPlateCurrent();
        
        // Simulate push-pull output with high-headroom characteristics
        power_amp_signal = (current1 - current2) * 0.07;  // Clean factor
        
        // Apply power amp saturation based on power level (80s focused on headroom)
        double saturation_factor = 2.5 + power_level * 1.5;  // 2.5 to 4.0 range (more headroom)
        power_amp_signal = tanh(power_amp_signal * saturation_factor) / saturation_factor;
        
        // Apply power amp compression (1980s amps had tight, controlled bass)
        double compression_factor = 0.9 - power_level * 0.1;  // Tight, controlled compression
        if (fabs(power_amp_signal) > 0.6) {
            power_amp_signal = power_amp_signal * compression_factor + 
                              (power_amp_signal > 0 ? 0.6 : -0.6) * (1.0 - compression_factor);
        }
    }
}

void TubeAmpSimulation1980s::ProcessToneStack() {
    // Simulate the tone stack of a typical 1980s high-headroom amp
    // 1980s designs focused on clean, controlled tone with tight bass
    
    // Apply tone controls by modifying the signal
    // 1980s tone stacks were often designed for clean, extended response
    double bass_factor = (bass - 1.0) * 0.5 + 1.0;  // -0.5 to 1.5 range around 1.0
    double mid_factor = (mid - 1.0) * 0.5 + 1.0;    // -0.5 to 1.5 range - focused mids
    double treble_factor = (treble - 1.0) * 0.6 + 1.0;  // -0.6 to 1.6 range for extended highs
    
    // Apply frequency-dependent tone controls with 80s characteristics
    double adjusted_signal = power_amp_signal;
    
    // Apply bass (with tight, controlled low-end for 80s sound)
    adjusted_signal *= (0.85 + 0.4 * bass_factor);
    
    // Apply focused midrange (less scooping than 70s high-gain)
    adjusted_signal *= (0.9 + 0.4 * mid_factor);
    
    // Apply extended treble (clean, extended highs)
    adjusted_signal *= (0.8 + 0.5 * treble_factor);
    
    power_amp_signal = adjusted_signal;
    
    // Apply presence control (for clean, defined highs)
    if (presence > 0.0) {
        // Add high-frequency clarity for clean definition
        power_amp_signal *= (1.0 + presence * 0.35);  // Clean presence
    }
    
    // Apply resonance control (controlled OT resonance for tight response)
    if (resonance > 0.0) {
        // Add controlled output transformer resonance characteristics
        power_amp_signal *= (1.0 + resonance * 0.2);  // Controlled resonance
    }
}

void TubeAmpSimulation1980s::ProcessCabinetSimulation() {
    // Apply simplified cabinet simulation for 1980s high-headroom sound
    if (cab_simulation_enabled && !cabinet_response.empty()) {
        // Apply the frequency response characteristic of 1980s clean cabs
        static double lp_state = 0.0;
        static double hp_state = 0.0;
        
        // Apply low-pass filter with 80s characteristics (tight bass, extended highs)
        double lp_coeff = 0.1;  // Tight response
        power_amp_signal = lp_state + lp_coeff * (power_amp_signal - lp_state);
        lp_state = power_amp_signal;
        
        // Apply some additional coloration for the clean, tight sound of 80s cabs
        power_amp_signal = power_amp_signal * 0.9 + 
                          sin(power_amp_signal * 12.0) * 0.03 +  // Clean harmonics
                          cos(power_amp_signal * 18.0) * 0.015;  // High-frequency detail
    }
}

double TubeAmpSimulation1980s::CalculateToneStackResponse(double input, double bass, double mid, double treble) {
    // Simplified tone stack calculation for 1980s high-headroom style
    double response = input;
    
    // Apply tone controls typical of 1980s clean designs
    response *= (0.8 + 0.6 * bass);   // Controlled bass
    response *= (0.9 + 0.4 * mid);    // Focused mids
    response *= (0.75 + 0.7 * treble); // Extended highs
    
    return response;
}

void TubeAmpSimulation1980s::SetGain(double gain) {
    this->gain = std::max(MIN_GAIN, std::min(MAX_GAIN, gain));
    
    // Update preamp stage gains based on overall gain for high-headroom characteristics
    if (!preamp_stage_gains.empty()) {
        // 1980s amps had moderate gains for headroom
        preamp_stage_gains[0] = 25.0 + gain * 0.4;  // First stage gets moderate gain
        if (preamp_stage_gains.size() > 1) {
            preamp_stage_gains[1] = 15.0 + gain * 0.2;  // Second stage gets less gain
        }
    }
}

void TubeAmpSimulation1980s::SetToneControls(double bass, double mid, double treble) {
    this->bass = std::max(MIN_BASS, std::min(MAX_BASS, bass));
    this->mid = std::max(MIN_MID, std::min(MAX_MID, mid));  // Focused midrange
    this->treble = std::max(MIN_TREBLE, std::min(MAX_TREBLE, treble));  // Extended highs
}

void TubeAmpSimulation1980s::SetPresence(double presence) {
    this->presence = std::max(MIN_PRESENCE, std::min(MAX_PRESENCE, presence));
}

void TubeAmpSimulation1980s::SetResonance(double resonance) {
    this->resonance = std::max(MIN_RESONANCE, std::min(MAX_RESONANCE, resonance));
}

void TubeAmpSimulation1980s::SetPowerLevel(double level) {
    this->power_level = std::max(MIN_POWER_LEVEL, std::min(MAX_POWER_LEVEL, level));
}

void TubeAmpSimulation1980s::SetInputLevel(double level) {
    this->input_level = std::max(MIN_LEVEL, std::min(MAX_LEVEL, level));
}

void TubeAmpSimulation1980s::SetOutputLevel(double level) {
    this->output_level = std::max(MIN_LEVEL, std::min(MAX_LEVEL, level));
}

void TubeAmpSimulation1980s::SetCabSimulation(bool enabled) {
    this->cab_simulation_enabled = enabled;
}

void TubeAmpSimulation1980s::SetHeadroomMode(bool high_headroom) {
    this->high_headroom_mode = high_headroom;
}