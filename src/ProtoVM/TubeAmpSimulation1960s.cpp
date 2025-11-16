#include "TubeAmpSimulation1960s.h"
#include "Common.h"
#include <algorithm>
#include <cmath>

// TubeAmpSimulation1960s implementation
TubeAmpSimulation1960s::TubeAmpSimulation1960s()
    : gain(30.0)  // Higher gain for 1960s rock sound
    , bass(0.8)
    , mid(1.8)    // Higher mid for that British blues rock hump
    , treble(1.2)
    , presence(0.8)   // More presence for brighter sound
    , resonance(0.9)  // More resonance for that British OT sound
    , power_level(0.7)
    , input_level(1.0)
    , output_level(1.0)
    , cab_simulation_enabled(true)
    , class_a_simulation(false)  // Most 1960s British amps were Class AB
    , phase_inverter_gain(1.0)
    , output_transformer_coupling(0.93)  // Slightly different coupling for British OTs
    , input_signal(0.0)
    , output_signal(0.0)
    , power_amp_signal(0.0)
    , sample_rate(44100.0)
    , dt(1.0 / 44100.0)
{
    InitializeAmp();
}

TubeAmpSimulation1960s::~TubeAmpSimulation1960s() {
    // Destructor handled by member destructors
}

void TubeAmpSimulation1960s::InitializeAmp() {
    // Initialize tubes for 1960s British era simulation (Marshall-style)
    // 1960s British amps like Marshall had more gain stages and higher gain overall
    tubes.push_back(std::make_unique<Triode>(100000.0, 100000.0, 1.6e-3));  // Preamp stage 1 (V1)
    tubes.push_back(std::make_unique<Triode>(100000.0, 100000.0, 1.6e-3));  // Preamp stage 2 (V2) - often more gain than 50s
    tubes.push_back(std::make_unique<Triode>(470000.0, 100000.0, 8.0e-4));  // Phase inverter (V3)
    tubes.push_back(std::make_unique<Pentode>(80000.0, 10000.0, 1.8e-3, 0.5, 15.0));  // Power amp left
    tubes.push_back(std::make_unique<Pentode>(80000.0, 10000.0, 1.8e-3, 0.5, 15.0));  // Power amp right
    
    // Set initial gains for each preamp stage (higher for 1960s British style)
    preamp_stage_gains.push_back(45.0);  // Higher gain for V1 (British approach)
    preamp_stage_gains.push_back(25.0);  // Higher gain for V2 
    preamp_stage_gains.push_back(5.0);   // Lower gain for phase inverter
    
    // Initialize tone stack state
    tone_stack_state[0] = 0.0;  // Bass state
    tone_stack_state[1] = 0.0;  // Mid state  
    tone_stack_state[2] = 0.0;  // Treble state
    
    // Initialize cabinet response for 1960s British style (Marshall 4x12)
    if (cab_simulation_enabled) {
        cabinet_response.resize(64, 0.0);
        cabinet_delay.resize(32, 0.0);
        
        // Set up basic cabinet response for 1960s British sound (Marshall-style)
        for (int i = 0; i < 64; i++) {
            double freq = (double)i * 20000.0 / 64.0;  // 0 to 20kHz
            // Response simulating classic British guitar speaker (Celestion Greenback-style)
            if (freq < 100.0) {
                cabinet_response[i] = 0.85;   // Slight low boost
            } else if (freq < 250.0) {
                cabinet_response[i] = 0.95;   // Strong low-mid presence
            } else if (freq < 500.0) {
                cabinet_response[i] = 1.1;    // Mid hump characteristic
            } else if (freq < 1000.0) {
                cabinet_response[i] = 1.2;    // Enhanced mids (the famous "scooped" mid)
            } else if (freq < 2000.0) {
                cabinet_response[i] = 1.15;   // Still strong mids
            } else if (freq < 5000.0) {
                cabinet_response[i] = 0.95;   // Starts to roll off high
            } else {
                cabinet_response[i] = 0.7;    // Heavy high rolloff
            }
        }
    }
}

bool TubeAmpSimulation1960s::Tick() {
    ProcessSignal();
    return true;
}

void TubeAmpSimulation1960s::ProcessSignal() {
    // Apply input level scaling
    double signal = input_signal * input_level;
    
    // Process through preamp stages (with higher gain for 1960s British sound)
    ProcessPreamp();
    
    // Process through phase inverter
    ProcessPhaseInverter();
    
    // Process through power amp stage
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

void TubeAmpSimulation1960s::ProcessPreamp() {
    // Apply the first preamp stage (higher gain for 1960s British sound)
    double signal = input_signal * input_level * 1.5;  // More input gain for British style
    
    if (!tubes.empty()) {
        auto& preamp_tube1 = tubes[0];
        // Apply signal to grid with gain (higher gain for British sound)
        double grid_voltage = -1.0 + signal * preamp_stage_gains[0] * 0.008;  // Higher gain factor
        preamp_tube1->SetGridVoltage(grid_voltage);
        preamp_tube1->SetPlateVoltage(250.0);  // Typical plate voltage
        preamp_tube1->SetCathodeVoltage(0.0);
        preamp_tube1->CalculateTubeBehavior();
        
        // Get the amplified signal
        signal = preamp_tube1->GetPlateCurrent() * 0.12;  // Higher plate current factor
        
        // Apply soft limiting to simulate tube saturation (British amps were known for early saturation)
        double saturation_factor = 1.8 + (gain / MAX_GAIN) * 1.5;  // More saturation at higher gains
        signal = tanh(signal * saturation_factor) / saturation_factor;
        
        // Apply the second preamp stage (also with higher gain)
        if (tubes.size() > 1) {
            auto& preamp_tube2 = tubes[1];
            grid_voltage = -1.0 + signal * preamp_stage_gains[1] * 0.007;  // Higher gain factor
            preamp_tube2->SetGridVoltage(grid_voltage);
            preamp_tube2->SetPlateVoltage(250.0);
            preamp_tube2->SetCathodeVoltage(0.0);
            preamp_tube2->CalculateTubeBehavior();
            
            signal = preamp_tube2->GetPlateCurrent() * 0.12;  // Higher plate current factor
            signal = tanh(signal * saturation_factor) / saturation_factor;  // Apply saturation
        }
    }
    
    // Update the input signal for next stages
    input_signal = signal;
}

void TubeAmpSimulation1960s::ProcessPhaseInverter() {
    // In a 1960s British style phase inverter (different from Fender)
    if (tubes.size() > 2) {
        auto& phase_inv_tube = tubes[2];
        double input = input_signal;
        
        // Apply to phase inverter (British designs often had different phase inverter circuits)
        double grid_voltage = -1.0 + input * phase_inverter_gain * 0.04;  // Different gain factor
        phase_inv_tube->SetGridVoltage(grid_voltage);
        phase_inv_tube->SetPlateVoltage(250.0);
        phase_inv_tube->SetCathodeVoltage(0.0);
        phase_inv_tube->CalculateTubeBehavior();
        
        // For a simplified model
        input = phase_inv_tube->GetPlateCurrent() * 0.1;
        input = tanh(input * 2.0) * 0.5;  // Apply British-style phase inverter saturation
    }
}

void TubeAmpSimulation1960s::ProcessPowerAmp() {
    // Process through power amp stage (British Class AB characteristics)
    if (tubes.size() > 3) {
        // Get both power tubes
        auto& power_tube1 = tubes[3];
        auto& power_tube2 = tubes[4];
        
        // Apply the input to both power tubes (push-pull configuration)
        double input_signal_scaled = input_signal * 0.12;  // Slightly different scaling for British sound
        
        // Apply to both tubes in push-pull
        double grid1 = -1.0 + input_signal_scaled * 0.5;
        double grid2 = -1.0 - input_signal_scaled * 0.5;  // Inverted
        
        power_tube1->SetGridVoltage(grid1);
        power_tube1->SetPlateVoltage(360.0);  // Slightly lower voltage for British sound
        power_tube1->SetCathodeVoltage(0.0);
        power_tube1->CalculateTubeBehavior();
        
        power_tube2->SetGridVoltage(grid2);
        power_tube2->SetPlateVoltage(360.0);
        power_tube2->SetCathodeVoltage(0.0);
        power_tube2->CalculateTubeBehavior();
        
        // Combine outputs from both tubes
        double current1 = power_tube1->GetPlateCurrent();
        double current2 = power_tube2->GetPlateCurrent();
        
        // Simulate push-pull output with British characteristics
        power_amp_signal = (current1 - current2) * 0.06;  // Different factor for British sound
        
        // Apply power amp saturation based on power level (British amps have distinctive power amp saturation)
        double saturation_factor = 1.5 + power_level * 2.5;  // 1.5 to 4.0 range
        power_amp_signal = tanh(power_amp_signal * saturation_factor) / saturation_factor;
        
        // Apply power amp compression (Class AB characteristics)
        double compression_factor = class_a_simulation ? 0.7 : 0.9 - power_level * 0.2;  // Less compression for Class AB
        if (fabs(power_amp_signal) > 0.6) {
            power_amp_signal = power_amp_signal * compression_factor + 
                              (power_amp_signal > 0 ? 0.6 : -0.6) * (1.0 - compression_factor);
        }
    }
}

void TubeAmpSimulation1960s::ProcessToneStack() {
    // Simulate the tone stack of a typical 1960s British amp (Marshall-style)
    // Marshall tone stacks were known for their mid-hump and interactive controls
    
    // Apply tone controls by modifying the signal
    // 1960s British amps typically had more interactive tone controls
    double bass_factor = (bass - 1.0) * 0.4 + 1.0;  // -0.4 to 1.4 range around 1.0
    double mid_factor = (mid - 1.0) * 0.8 + 1.0;    // -0.8 to 2.6 range - emphasizes the mid hump
    double treble_factor = (treble - 1.0) * 0.4 + 1.0;  // -0.4 to 1.4 range around 1.0
    
    // Apply frequency-dependent tone controls with more interaction
    double adjusted_signal = power_amp_signal;
    
    // Apply bass (affects low frequencies more significantly)
    adjusted_signal *= (0.75 + 0.5 * bass_factor);
    
    // Apply the characteristic mid hump of 1960s British amps
    adjusted_signal *= (0.8 + 0.7 * mid_factor);
    
    // Apply treble (often has interaction with presence in British designs)
    adjusted_signal *= (0.8 + 0.4 * treble_factor);
    
    power_amp_signal = adjusted_signal;
    
    // Apply presence control (affects high frequencies through negative feedback)
    // British amps typically had more present highs
    if (presence > 0.0) {
        // Add high-frequency emphasis
        power_amp_signal *= (1.0 + presence * 0.4);  // More presence than 1950s
    }
    
    // Apply resonance control (affects output transformer resonance)
    if (resonance > 0.0) {
        // Add output transformer resonance characteristics
        power_amp_signal *= (1.0 + resonance * 0.25);  // More resonance than 1950s
    }
}

void TubeAmpSimulation1960s::ProcessCabinetSimulation() {
    // Apply simplified cabinet simulation for 1960s British sound
    if (cab_simulation_enabled && !cabinet_response.empty()) {
        // Apply the frequency response characteristic of 1960s British cabs
        static double lp_state = 0.0;
        static double hp_state = 0.0;
        
        // Apply low-pass filter with resonance characteristics
        double lp_coeff = 0.15;  // Slightly more aggressive than 1950s
        power_amp_signal = lp_state + lp_coeff * (power_amp_signal - lp_state);
        lp_state = power_amp_signal;
        
        // Apply some additional coloration to simulate cabinet resonance
        // Emphasize the midrange hump characteristic of British cabinets
        power_amp_signal = power_amp_signal * 0.85 + 
                          sin(power_amp_signal * 8.0) * 0.05 +  // Midrange harmonics
                          cos(power_amp_signal * 12.0) * 0.03;  // More complex harmonics
    }
}

double TubeAmpSimulation1960s::CalculateToneStackResponse(double input, double bass, double mid, double treble) {
    // Simplified tone stack calculation for 1960s British style
    double response = input;
    
    // Apply interactive tone controls typical of 1960s British designs
    response *= (0.7 + 0.6 * bass);
    response *= (0.6 + 1.2 * mid);    // More emphasis on mid for British sound
    response *= (0.8 + 0.4 * treble);
    
    return response;
}

void TubeAmpSimulation1960s::SetGain(double gain) {
    this->gain = std::max(MIN_GAIN, std::min(MAX_GAIN, gain));
    
    // Update preamp stage gains based on overall gain (with British characteristics)
    if (!preamp_stage_gains.empty()) {
        // 1960s British amps had higher gain stages
        preamp_stage_gains[0] = 30.0 + gain * 0.7;  // First stage gets significant gain
        if (preamp_stage_gains.size() > 1) {
            preamp_stage_gains[1] = 15.0 + gain * 0.4;  // Second stage also gets some
        }
    }
}

void TubeAmpSimulation1960s::SetToneControls(double bass, double mid, double treble) {
    this->bass = std::max(MIN_BASS, std::min(MAX_BASS, bass));
    this->mid = std::max(MIN_MID, std::min(MAX_MID, mid));  // 1960s known for mid hump
    this->treble = std::max(MIN_TREBLE, std::min(MAX_TREBLE, treble));
}

void TubeAmpSimulation1960s::SetPresence(double presence) {
    this->presence = std::max(MIN_PRESENCE, std::min(MAX_PRESENCE, presence));
}

void TubeAmpSimulation1960s::SetResonance(double resonance) {
    this->resonance = std::max(MIN_RESONANCE, std::min(MAX_RESONANCE, resonance));
}

void TubeAmpSimulation1960s::SetPowerLevel(double level) {
    this->power_level = std::max(MIN_POWER_LEVEL, std::min(MAX_POWER_LEVEL, level));
}

void TubeAmpSimulation1960s::SetInputLevel(double level) {
    this->input_level = std::max(MIN_LEVEL, std::min(MAX_LEVEL, level));
}

void TubeAmpSimulation1960s::SetOutputLevel(double level) {
    this->output_level = std::max(MIN_LEVEL, std::min(MAX_LEVEL, level));
}

void TubeAmpSimulation1960s::SetCabSimulation(bool enabled) {
    this->cab_simulation_enabled = enabled;
}

void TubeAmpSimulation1960s::SetClassA(bool class_a) {
    this->class_a_simulation = class_a;
}