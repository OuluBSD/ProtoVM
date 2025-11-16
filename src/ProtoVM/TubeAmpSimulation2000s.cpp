#include "TubeAmpSimulation2000s.h"
#include "Common.h"
#include <algorithm>
#include <cmath>

// TubeAmpSimulation2000s implementation
TubeAmpSimulation2000s::TubeAmpSimulation2000s()
    : gain(75.0)  // High gain for modern saturated sound
    , bass(1.3)
    , mid(1.1)    // Focused, modern midrange
    , treble(1.4) // Extended but controlled highs
    , presence(1.1)   // Extended presence for modern clarity
    , resonance(0.9)  // Controlled resonance for tight response
    , power_level(0.7)  // Higher power for modern saturation
    , input_level(1.0)
    , output_level(1.0)
    , cab_simulation_enabled(true)
    , modern_mode(true)  // Modern tight low-end mode
    , phase_inverter_gain(1.0)
    , output_transformer_coupling(0.94)  // Modern coupling for tight response
    , input_signal(0.0)
    , output_signal(0.0)
    , power_amp_signal(0.0)
    , sample_rate(44100.0)
    , dt(1.0 / 44100.0)
{
    InitializeAmp();
}

TubeAmpSimulation2000s::~TubeAmpSimulation2000s() {
    // Destructor handled by member destructors
}

void TubeAmpSimulation2000s::InitializeAmp() {
    // Initialize tubes for 2000s modern era simulation
    // 2000s amps often had complex gain structures with tight low-end control
    tubes.push_back(std::make_unique<Triode>(100000.0, 100000.0, 1.6e-3));  // Preamp stage 1 (V1)
    tubes.push_back(std::make_unique<Triode>(100000.0, 100000.0, 1.6e-3));  // Preamp stage 2 (V2)
    tubes.push_back(std::make_unique<Triode>(100000.0, 100000.0, 1.6e-3));  // Preamp stage 3 (V3) - extra gain
    tubes.push_back(std::make_unique<Triode>(470000.0, 100000.0, 8.0e-4));  // Phase inverter (V4)
    tubes.push_back(std::make_unique<Pentode>(100000.0, 8000.0, 1.5e-3, 0.35, 15.0));  // Power amp left
    tubes.push_back(std::make_unique<Pentode>(100000.0, 8000.0, 1.5e-3, 0.35, 15.0));  // Power amp right
    
    // Set initial gains for each preamp stage (high for modern saturated sound)
    preamp_stage_gains.push_back(45.0);  // High gain for V1
    preamp_stage_gains.push_back(35.0);  // High gain for V2 
    preamp_stage_gains.push_back(25.0);  // High gain for V3 (extra stage)
    preamp_stage_gains.push_back(5.0);   // Lower gain for phase inverter
    
    // Initialize tone stack state
    tone_stack_state[0] = 0.0;  // Bass state
    tone_stack_state[1] = 0.0;  // Mid state  
    tone_stack_state[2] = 0.0;  // Treble state
    
    // Initialize cabinet response for 2000s modern style
    if (cab_simulation_enabled) {
        cabinet_response.resize(64, 0.0);
        cabinet_delay.resize(32, 0.0);
        
        // Set up cabinet response for 2000s modern tight, extended sound
        for (int i = 0; i < 64; i++) {
            double freq = (double)i * 20000.0 / 64.0;  // 0 to 20kHz
            // Response simulating 2000s modern cab (tight low-end, extended highs)
            if (freq < 60.0) {
                cabinet_response[i] = 0.85;   // Very tight, controlled low-end
            } else if (freq < 150.0) {
                cabinet_response[i] = 0.95;   // Tight low-mid
            } else if (freq < 500.0) {
                cabinet_response[i] = 1.0;    // Neutral mid
            } else if (freq < 1000.0) {
                cabinet_response[i] = 1.05;   // Slight mid presence
            } else if (freq < 4000.0) {
                cabinet_response[i] = 1.0;    // Extended highs
            } else if (freq < 8000.0) {
                cabinet_response[i] = 1.05;   // Presence bump
            } else {
                cabinet_response[i] = 0.95;   // Slight high rolloff
            }
        }
    }
}

bool TubeAmpSimulation2000s::Tick() {
    ProcessSignal();
    return true;
}

void TubeAmpSimulation200s::ProcessSignal() {
    // Apply input level scaling
    double signal = input_signal * input_level;
    
    // Process through preamp stages (with high gain for modern sound)
    ProcessPreamp();
    
    // Process through phase inverter
    ProcessPhaseInverter();
    
    // Process through power amp stage (with modern tight characteristics)
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

void TubeAmpSimulation2000s::ProcessPreamp() {
    // Apply the first preamp stage (high gain for modern sound)
    double signal = input_signal * input_level * 1.15;  // High input gain
    
    if (!tubes.empty()) {
        for (int i = 0; i < 3 && i < tubes.size(); i++) {  // Process through first 3 preamp stages
            auto& tube = tubes[i];
            double gain_factor = 0.0065;  // High factor for modern gain structure
            
            // Apply signal to grid with gain specific to modern design
            double grid_voltage = -1.0 + signal * preamp_stage_gains[i] * gain_factor;
            tube->SetGridVoltage(grid_voltage);
            tube->SetPlateVoltage(300.0);  // Higher voltage for modern design
            tube->SetCathodeVoltage(0.0);
            tube->CalculateTubeBehavior();
            
            // Get the amplified signal
            signal = tube->GetPlateCurrent() * 0.14;  // High plate current factor
            
            // Apply soft limiting to simulate tube saturation (modern focused on tight saturation)
            double saturation_factor = 2.0 + (i * 0.4) + (gain / MAX_GAIN) * 1.5;  // Tight saturation for modern
            signal = tanh(signal * saturation_factor) / saturation_factor;
        }
    }
    
    // Update the input signal for next stages
    input_signal = signal;
}

void TubeAmpSimulation2000s::ProcessPhaseInverter() {
    // In a 2000s modern style phase inverter
    if (tubes.size() > 3) {
        auto& phase_inv_tube = tubes[3];
        double input = input_signal;
        
        // Apply to phase inverter (modern designs had very precise phase inverters)
        double grid_voltage = -1.0 + input * phase_inverter_gain * 0.04;  // Modern gain factor
        phase_inv_tube->SetGridVoltage(grid_voltage);
        phase_inv_tube->SetPlateVoltage(300.0);  // Higher voltage for modern design
        phase_inv_tube->SetCathodeVoltage(0.0);
        phase_inv_tube->CalculateTubeBehavior();
        
        // For a simplified model
        input = phase_inv_tube->GetPlateCurrent() * 0.1;
        input = tanh(input * 2.3) * 0.43;  // Modern phase inverter characteristics
    }
}

void TubeAmpSimulation2000s::ProcessPowerAmp() {
    // Process through power amp stage (with modern tight low-end characteristics)
    if (tubes.size() > 4) {
        // Get both power tubes
        auto& power_tube1 = tubes[4];
        auto& power_tube2 = tubes[5];
        
        // Apply the input to both power tubes (push-pull configuration)
        double input_signal_scaled = input_signal * 0.085;  // Modern scaling for tightness
        
        // Apply to both tubes in push-pull
        double grid1 = -1.0 + input_signal_scaled * 0.5;
        double grid2 = -1.0 - input_signal_scaled * 0.5;  // Inverted
        
        power_tube1->SetGridVoltage(grid1);
        power_tube1->SetPlateVoltage(450.0);  // Higher voltage for modern design
        power_tube1->SetCathodeVoltage(0.0);
        power_tube1->CalculateTubeBehavior();
        
        power_tube2->SetGridVoltage(grid2);
        power_tube2->SetPlateVoltage(450.0);
        power_tube2->SetCathodeVoltage(0.0);
        power_tube2->CalculateTubeBehavior();
        
        // Combine outputs from both tubes
        double current1 = power_tube1->GetPlateCurrent();
        double current2 = power_tube2->GetPlateCurrent();
        
        // Simulate push-pull output with modern tight characteristics
        power_amp_signal = (current1 - current2) * 0.08;  // Modern factor
        
        // Apply power amp saturation based on power level (2000s focused on tight saturation)
        double saturation_factor = 2.0 + power_level * 2.0;  // 2.0 to 4.0 range (tight)
        power_amp_signal = tanh(power_amp_signal * saturation_factor) / saturation_factor;
        
        // Apply power amp compression (2000s amps had very tight, controlled response)
        double compression_factor = 0.9 - power_level * 0.15;  // Very tight compression for modern
        if (fabs(power_amp_signal) > 0.55) {
            power_amp_signal = power_amp_signal * compression_factor + 
                              (power_amp_signal > 0 ? 0.55 : -0.55) * (1.0 - compression_factor);
        }
    }
}

void TubeAmpSimulation2000s::ProcessToneStack() {
    // Simulate the tone stack of a typical 2000s modern amp
    // 2000s designs focused on tight, controlled response with extended range
    
    // Apply tone controls by modifying the signal
    // 2000s tone stacks were often very precise with tight control
    double bass_factor = (bass - 1.0) * 0.55 + 1.0;  // -0.55 to 1.55 range around 1.0
    double mid_factor = (mid - 1.0) * 0.55 + 1.0;    // -0.55 to 1.55 range - focused mids
    double treble_factor = (treble - 1.0) * 0.55 + 1.0;  // -0.55 to 1.55 range for extended highs
    
    // Apply frequency-dependent tone controls with modern characteristics
    double adjusted_signal = power_amp_signal;
    
    // Apply very tight bass control (signature of 2000s modern sound)
    adjusted_signal *= (0.8 + 0.5 * bass_factor);
    
    // Apply focused midrange (precise, modern mid)
    adjusted_signal *= (0.9 + 0.4 * mid_factor);
    
    // Apply extended but controlled treble (modern highs)
    adjusted_signal *= (0.8 + 0.5 * treble_factor);
    
    power_amp_signal = adjusted_signal;
    
    // Apply presence control (very important in modern designs for clarity)
    if (presence > 0.0) {
        // Add presence for modern clarity and definition
        power_amp_signal *= (1.0 + presence * 0.4);  // Modern presence
    }
    
    // Apply resonance control (very controlled OT resonance for modern tightness)
    if (resonance > 0.0) {
        // Add very controlled output transformer resonance characteristics
        power_amp_signal *= (1.0 + resonance * 0.2);  // Very controlled resonance
    }
}

void TubeAmpSimulation2000s::ProcessCabinetSimulation() {
    // Apply simplified cabinet simulation for 2000s modern sound
    if (cab_simulation_enabled && !cabinet_response.empty()) {
        // Apply the frequency response characteristic of 2000s modern cabs
        static double lp_state = 0.0;
        static double hp_state = 0.0;
        
        // Apply low-pass filter with modern characteristics (very tight bass, extended highs)
        double lp_coeff = 0.09;  // Very tight response
        power_amp_signal = lp_state + lp_coeff * (power_amp_signal - lp_state);
        lp_state = power_amp_signal;
        
        // Apply some additional coloration for the modern tight sound
        power_amp_signal = power_amp_signal * 0.88 + 
                          sin(power_amp_signal * 13.0) * 0.025 +  // Modern harmonics
                          cos(power_amp_signal * 19.0) * 0.015;   // High-frequency detail
    }
}

double TubeAmpSimulation2000s::CalculateToneStackResponse(double input, double bass, double mid, double treble) {
    // Simplified tone stack calculation for 2000s modern style
    double response = input;
    
    // Apply tone controls typical of 2000s modern designs (tight, extended range)
    response *= (0.8 + 0.6 * bass);   // Very tight bass control
    response *= (0.9 + 0.4 * mid);    // Focused mids
    response *= (0.8 + 0.5 * treble); // Extended but controlled highs
    
    return response;
}

void TubeAmpSimulation2000s::SetGain(double gain) {
    this->gain = std::max(MIN_GAIN, std::min(MAX_GAIN, gain));
    
    // Update preamp stage gains based on overall gain for modern tight characteristics
    if (!preamp_stage_gains.empty()) {
        // 2000s modern amps had high gains with tight control
        preamp_stage_gains[0] = 35.0 + gain * 0.6;  // First stage gets high gain
        if (preamp_stage_gains.size() > 1) {
            preamp_stage_gains[1] = 25.0 + gain * 0.4;  // Second stage gets high gain
        }
        if (preamp_stage_gains.size() > 2) {
            preamp_stage_gains[2] = 15.0 + gain * 0.2;  // Third stage for extra gain
        }
    }
}

void TubeAmpSimulation2000s::SetToneControls(double bass, double mid, double treble) {
    this->bass = std::max(MIN_BASS, std::min(MAX_BASS, bass));
    this->mid = std::max(MIN_MID, std::min(MAX_MID, mid));  // Focused, modern mid
    this->treble = std::max(MIN_TREBLE, std::min(MAX_TREBLE, treble));  // Extended highs
}

void TubeAmpSimulation2000s::SetPresence(double presence) {
    this->presence = std::max(MIN_PRESENCE, std::min(MAX_PRESENCE, presence));
}

void TubeAmpSimulation2000s::SetResonance(double resonance) {
    this->resonance = std::max(MIN_RESONANCE, std::min(MAX_RESONANCE, resonance));
}

void TubeAmpSimulation2000s::SetPowerLevel(double level) {
    this->power_level = std::max(MIN_POWER_LEVEL, std::min(MAX_POWER_LEVEL, level));
}

void TubeAmpSimulation2000s::SetInputLevel(double level) {
    this->input_level = std::max(MIN_LEVEL, std::min(MAX_LEVEL, level));
}

void TubeAmpSimulation2000s::SetOutputLevel(double level) {
    this->output_level = std::max(MIN_LEVEL, std::min(MAX_LEVEL, level));
}

void TubeAmpSimulation2000s::SetCabSimulation(bool enabled) {
    this->cab_simulation_enabled = enabled;
}

void TubeAmpSimulation2000s::SetModernMode(bool modern_mode) {
    this->modern_mode = modern_mode;
}