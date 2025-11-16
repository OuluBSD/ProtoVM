#include "TubeAmpSimulation1950s.h"
#include "Common.h"
#include <algorithm>
#include <cmath>

// TubeAmpSimulation1950s implementation
TubeAmpSimulation1950s::TubeAmpSimulation1950s()
    : gain(10.0)
    , bass(1.0)
    , mid(1.0)
    , treble(1.0)
    , presence(0.2)
    , resonance(0.1)
    , power_level(0.5)
    , input_level(1.0)
    , output_level(1.0)
    , cab_simulation_enabled(true)
    , phase_inverter_gain(1.0)
    , output_transformer_coupling(0.95)
    , input_signal(0.0)
    , output_signal(0.0)
    , power_amp_signal(0.0)
    , sample_rate(44100.0)
    , dt(1.0 / 44100.0)
{
    InitializeAmp();
}

TubeAmpSimulation1950s::~TubeAmpSimulation1950s() {
    // Destructor handled by member destructors
}

void TubeAmpSimulation1950s::InitializeAmp() {
    // Initialize tubes for 1950s era simulation
    // 1950s amps typically had simple preamp (often 1-2 stages), phase inverter, and power amp
    tubes.push_back(std::make_unique<Triode>(100000.0, 100000.0, 1.6e-3));  // Preamp stage 1 (V1)
    tubes.push_back(std::make_unique<Triode>(100000.0, 470000.0, 1.0e-3));  // Preamp stage 2 (V2) - V1B in some circuits
    tubes.push_back(std::make_unique<Triode>(470000.0, 100000.0, 8.0e-4));  // Phase inverter (V3)
    tubes.push_back(std::make_unique<Pentode>(80000.0, 10000.0, 1.8e-3, 0.5, 15.0));  // Power amp left
    tubes.push_back(std::make_unique<Pentode>(80000.0, 10000.0, 1.8e-3, 0.5, 15.0));  // Power amp right
    
    // Set initial gains for each preamp stage
    preamp_stage_gains.push_back(30.0);  // High gain for V1
    preamp_stage_gains.push_back(15.0);  // Moderate gain for V2
    preamp_stage_gains.push_back(5.0);   // Lower gain for phase inverter
    
    // Initialize tone stack state
    tone_stack_state[0] = 0.0;  // Bass state
    tone_stack_state[1] = 0.0;  // Mid state  
    tone_stack_state[2] = 0.0;  // Treble state
    
    // Initialize cabinet response (simplified for 1950s style)
    if (cab_simulation_enabled) {
        cabinet_response.resize(64, 0.0);
        cabinet_delay.resize(32, 0.0);
        
        // Set up basic cabinet response (simplified)
        for (int i = 0; i < 64; i++) {
            double freq = (double)i * 20000.0 / 64.0;  // 0 to 20kHz
            // Simple response simulating typical guitar speaker
            if (freq < 100.0) {
                cabinet_response[i] = 0.8;   // Slight low boost
            } else if (freq < 500.0) {
                cabinet_response[i] = 1.0;   // Flat mid
            } else if (freq < 2000.0) {
                cabinet_response[i] = 0.95;  // Slight mid dip
            } else if (freq < 5000.0) {
                cabinet_response[i] = 0.85;  // High rolloff
            } else {
                cabinet_response[i] = 0.6;   // Heavy high rolloff
            }
        }
    }
}

bool TubeAmpSimulation1950s::Tick() {
    ProcessSignal();
    return true;
}

void TubeAmpSimulation1950s::ProcessSignal() {
    // Apply input level scaling
    double signal = input_signal * input_level;
    
    // Process through preamp stages
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

void TubeAmpSimulation1950s::ProcessPreamp() {
    // Apply the first preamp stage (high gain)
    double signal = input_signal * input_level;
    
    if (!tubes.empty()) {
        auto& preamp_tube1 = tubes[0];
        // Apply signal to grid with gain
        double grid_voltage = -1.0 + signal * preamp_stage_gains[0] * 0.01;
        preamp_tube1->SetGridVoltage(grid_voltage);
        preamp_tube1->SetPlateVoltage(250.0);  // Typical plate voltage
        preamp_tube1->SetCathodeVoltage(0.0);
        preamp_tube1->CalculateTubeBehavior();
        
        // Get the amplified signal
        signal = preamp_tube1->GetPlateCurrent() * 0.1;  // Scale appropriately
        
        // Apply soft limiting to simulate tube saturation
        signal = tanh(signal) * 0.9;  // Prevent hard clipping
        
        // Apply the second preamp stage
        if (tubes.size() > 1) {
            auto& preamp_tube2 = tubes[1];
            grid_voltage = -1.0 + signal * preamp_stage_gains[1] * 0.01;
            preamp_tube2->SetGridVoltage(grid_voltage);
            preamp_tube2->SetPlateVoltage(250.0);
            preamp_tube2->SetCathodeVoltage(0.0);
            preamp_tube2->CalculateTubeBehavior();
            
            signal = preamp_tube2->GetPlateCurrent() * 0.1;
            signal = tanh(signal) * 0.9;  // Apply saturation
        }
    }
    
    // Update the input signal for next stages
    input_signal = signal;
}

void TubeAmpSimulation1950s::ProcessPhaseInverter() {
    // In a 1950s style phase inverter (like the long-tailed pair)
    if (tubes.size() > 2) {
        auto& phase_inv_tube = tubes[2];
        double input = input_signal;
        
        // Apply to phase inverter
        double grid_voltage = -1.0 + input * phase_inverter_gain * 0.05;
        phase_inv_tube->SetGridVoltage(grid_voltage);
        phase_inv_tube->SetPlateVoltage(250.0);
        phase_inv_tube->SetCathodeVoltage(0.0);
        phase_inv_tube->CalculateTubeBehavior();
        
        // For a simplified model, just pass through with phase inversion on one side
        input = phase_inv_tube->GetPlateCurrent() * 0.1;
        input = tanh(input) * 0.9;  // Apply saturation
    }
}

void TubeAmpSimulation1950s::ProcessPowerAmp() {
    // Process through power amp stage (simplified)
    if (tubes.size() > 3) {
        // Get both power tubes
        auto& power_tube1 = tubes[3];
        auto& power_tube2 = tubes[4];
        
        // Apply the input to both power tubes (push-pull configuration)
        double input_signal_scaled = input_signal * 0.1;  // Scale down for power tubes
        
        // Apply to both tubes in push-pull
        double grid1 = -1.0 + input_signal_scaled * 0.5;
        double grid2 = -1.0 - input_signal_scaled * 0.5;  // Inverted
        
        power_tube1->SetGridVoltage(grid1);
        power_tube1->SetPlateVoltage(400.0);  // Higher voltage for power tubes
        power_tube1->SetCathodeVoltage(0.0);
        power_tube1->CalculateTubeBehavior();
        
        power_tube2->SetGridVoltage(grid2);
        power_tube2->SetPlateVoltage(400.0);
        power_tube2->SetCathodeVoltage(0.0);
        power_tube2->CalculateTubeBehavior();
        
        // Combine outputs from both tubes
        double current1 = power_tube1->GetPlateCurrent();
        double current2 = power_tube2->GetPlateCurrent();
        
        // Simulate push-pull output
        power_amp_signal = (current1 - current2) * 0.05;
        
        // Apply power amp saturation based on power level
        double saturation_factor = 2.0 + power_level * 3.0;  // 2.0 to 5.0
        power_amp_signal = tanh(power_amp_signal * saturation_factor) / saturation_factor;
        
        // Apply power amp compression
        double compression_factor = 1.0 - power_level * 0.3;  // More compression at higher power
        if (fabs(power_amp_signal) > 0.5) {
            power_amp_signal = power_amp_signal * compression_factor + 
                              (power_amp_signal > 0 ? 0.5 : -0.5) * (1.0 - compression_factor);
        }
    }
}

void TubeAmpSimulation1950s::ProcessToneStack() {
    // Simulate the tone stack of a typical 1950s amp (Fender-style)
    // Using a simplified model with state variables
    
    // Apply tone controls by modifying the signal
    double bass_factor = (bass - 1.0) * 0.5 + 1.0;  // -0.5 to 1.5 range around 1.0
    double mid_factor = (mid - 1.0) * 0.5 + 1.0;
    double treble_factor = (treble - 1.0) * 0.5 + 1.0;
    
    // Apply frequency-dependent tone controls
    // Very simplified implementation - a real tone stack would be more complex
    double adjusted_signal = power_amp_signal;
    
    // Apply bass boost/cut (affecting lower frequencies)
    adjusted_signal *= (0.8 + 0.4 * bass_factor);
    
    // Apply mid dip/boost 
    adjusted_signal *= (0.9 + 0.2 * mid_factor);
    
    // Apply treble cut/boost (affecting higher frequencies)
    adjusted_signal *= (0.85 + 0.3 * treble_factor);
    
    power_amp_signal = adjusted_signal;
    
    // Apply presence and resonance controls
    // Presence affects high frequencies through negative feedback
    if (presence > 0.0) {
        // Add a bit of high-frequency emphasis
        power_amp_signal *= (1.0 + presence * 0.2);
    }
    
    // Resonance affects low frequencies 
    if (resonance > 0.0) {
        // Add a bit of low-frequency emphasis
        power_amp_signal *= (1.0 + resonance * 0.15);
    }
}

void TubeAmpSimulation1950s::ProcessCabinetSimulation() {
    // Apply simplified cabinet simulation
    if (cab_simulation_enabled && !cabinet_response.empty()) {
        // For simplicity, apply a frequency-dependent response
        // A more realistic implementation would use FIR filters
        
        // Apply a simple low-pass characteristic typical of guitar cabs
        static double lp_state = 0.0;
        double lp_coeff = 0.1;  // Controls filter strength
        power_amp_signal = lp_state + lp_coeff * (power_amp_signal - lp_state);
        lp_state = power_amp_signal;
        
        // Apply some additional coloration to simulate cabinet resonance
        // This is a very simplified approach
        power_amp_signal = power_amp_signal * 0.95 + 
                          sin(power_amp_signal * 10.0) * 0.02;  // Add subtle harmonics
    }
}

double TubeAmpSimulation1950s::CalculateToneStackResponse(double input, double bass, double mid, double treble) {
    // Simplified tone stack calculation
    // In a real implementation, this would use the actual RC network equations
    double response = input;
    
    // Apply tone controls
    response *= bass;
    response *= mid;  
    response *= treble;
    
    return response;
}

void TubeAmpSimulation1950s::SetGain(double gain) {
    this->gain = std::max(MIN_GAIN, std::min(MAX_GAIN, gain));
    
    // Update preamp stage gains based on overall gain
    if (!preamp_stage_gains.empty()) {
        preamp_stage_gains[0] = 20.0 + gain * 0.5;  // First stage gets most of the gain
        if (preamp_stage_gains.size() > 1) {
            preamp_stage_gains[1] = 10.0 + gain * 0.2;  // Second stage gets some
        }
    }
}

void TubeAmpSimulation1950s::SetToneControls(double bass, double mid, double treble) {
    this->bass = std::max(MIN_BASS, std::min(MAX_BASS, bass));
    this->mid = std::max(MIN_MID, std::min(MAX_MID, mid));
    this->treble = std::max(MIN_TREBLE, std::min(MAX_TREBLE, treble));
}

void TubeAmpSimulation1950s::SetPresence(double presence) {
    this->presence = std::max(MIN_PRESENCE, std::min(MAX_PRESENCE, presence));
}

void TubeAmpSimulation1950s::SetResonance(double resonance) {
    this->resonance = std::max(MIN_RESONANCE, std::min(MAX_RESONANCE, resonance));
}

void TubeAmpSimulation1950s::SetPowerLevel(double level) {
    this->power_level = std::max(MIN_POWER_LEVEL, std::min(MAX_POWER_LEVEL, level));
}

void TubeAmpSimulation1950s::SetInputLevel(double level) {
    this->input_level = std::max(MIN_LEVEL, std::min(MAX_LEVEL, level));
}

void TubeAmpSimulation1950s::SetOutputLevel(double level) {
    this->output_level = std::max(MIN_LEVEL, std::min(MAX_LEVEL, level));
}

void TubeAmpSimulation1950s::SetCabSimulation(bool enabled) {
    this->cab_simulation_enabled = enabled;
}