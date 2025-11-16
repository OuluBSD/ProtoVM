#include "TubeAmplifier.h"
#include <algorithm>
#include <cmath>

TubeAmplifier::TubeAmplifier(int num_tubes, AmplifierClass amp_class, TubeConfiguration config)
    : input_signal(0.0)
    , output_signal(0.0)
    , amplifier_gain(30.0)  // Default gain
    , harmonic_distortion(0.3)  // Default 30% harmonic distortion
    , amp_class(amp_class)
    , configuration(config)
    , load_resistance(8000.0)  // 8k ohm load
    , plate_voltage(250.0)     // 250V plate voltage
    , bias_voltage(-2.0)       // -2V bias
    , distortion_enabled(true)
    , bass_control(0.0)
    , mid_control(0.0)
    , treble_control(0.0)
{
    harmonic_content.resize(MAX_HARMONICS, 0.0);
    
    // Create the requested number of tubes based on configuration
    for (int i = 0; i < num_tubes; i++) {
        // Default to triode for now, could be configurable
        tubes.push_back(std::make_unique<Triode>());
    }
}

TubeAmplifier::~TubeAmplifier() {
    // Cleanup handled by smart pointers
}

bool TubeAmplifier::Tick() {
    // Process the input signal through the amplifier
    ProcessSignal();
    
    // Tick all tubes
    for (auto& tube : tubes) {
        tube->Tick();
    }
    
    // Calculate harmonic content
    CalculateHarmonicContent(output_signal);
    
    return true;
}

void TubeAmplifier::AddTube(std::unique_ptr<Tube> tube) {
    tubes.push_back(std::move(tube));
}

void TubeAmplifier::ProcessSignal() {
    // Apply amplifier class characteristics based on configuration
    ApplyAmplifierClassCharacteristics();
    
    // Process through each tube stage
    double signal = input_signal;
    
    for (auto& tube : tubes) {
        // Set the grid voltage based on the input signal
        double grid_voltage = bias_voltage + signal * 0.1; // Scale down for tube input
        tube->SetGridVoltage(grid_voltage);
        tube->SetPlateVoltage(plate_voltage);
        tube->SetCathodeVoltage(0.0);
        
        // Calculate the tube's behavior
        tube->CalculateTubeBehavior();
        
        // Get the resulting plate current
        double plate_current = tube->GetPlateCurrent();
        
        // Convert current back to voltage using load resistance
        // V = I * R (Ohm's law)
        signal = plate_current * load_resistance / 1000.0;  // Scale appropriately
        
        // Apply the tube's amplification characteristics
        signal *= amplifier_gain / tubes.size();  // Distribute gain across all tubes
    }
    
    // Apply distortion modeling if enabled
    if (distortion_enabled) {
        signal = ApplyDistortion(signal);
    }
    
    // Apply tone controls
    signal = ApplyToneControls(signal);
    
    // Limit the output to prevent clipping
    output_signal = std::max(-5.0, std::min(5.0, signal));
}

double TubeAmplifier::ApplyDistortion(double signal) {
    // Apply harmonic distortion modeling
    // This simulates the even and odd harmonic generation characteristics of tube amps
    
    // The transfer function of a tube amplifier is approximately:
    // output = gain * input / (1 + |input|^alpha)
    // where alpha controls the saturation characteristics
    
    double alpha = 1.0 + harmonic_distortion * 2.0;  // Adjust alpha based on desired distortion
    double gain = amplifier_gain;
    
    // Apply soft clipping with harmonic generation
    if (signal > 0) {
        signal = gain * signal / (1 + pow(signal, alpha));
    } else {
        signal = gain * signal / (1 + pow(-signal, alpha));
    }
    
    // Add some even harmonics (characteristic of single-ended amps)
    if (configuration == TubeConfiguration::SINGLE_ENDED) {
        double signal_squared = signal * signal * 0.1 * harmonic_distortion;
        signal += signal_squared;
    }
    
    // Add some odd harmonics (characteristic of push-pull amps)
    if (configuration == TubeConfiguration::PUSH_PULL) {
        double signal_cubed = signal * signal * signal * 0.05 * harmonic_distortion;
        signal += signal_cubed;
    }
    
    return signal;
}

double TubeAmplifier::ApplyToneControls(double signal) {
    // Apply simplified tone controls
    // This is a very simplified model - real tone stacks are more complex
    
    // Apply bass control (low frequency emphasis)
    signal *= (1.0 + bass_control * 0.5);
    
    // Apply mid control (mid frequency emphasis)
    signal *= (1.0 + mid_control * 0.3);
    
    // Apply treble control (high frequency emphasis)
    signal *= (1.0 + treble_control * 0.2);
    
    // Limit the overall amplification to prevent clipping
    signal = std::max(-5.0, std::min(5.0, signal));
    
    return signal;
}

void TubeAmplifier::CalculateHarmonicContent(double signal) {
    // Simplified harmonic content calculation
    // In a real implementation, this would use FFT or other spectral analysis
    
    // Calculate first 10 harmonics (though this is a very simplified approach)
    for (int i = 0; i < MAX_HARMONICS; i++) {
        // The harmonic strength decreases with frequency
        double harmonic_freq_factor = 1.0 / (i + 1);
        
        // For a tube amplifier, odd harmonics are typically stronger in push-pull
        // and even harmonics are stronger in single-ended
        if (configuration == TubeConfiguration::PUSH_PULL) {
            // Suppress even harmonics in push-pull (they cancel out)
            if ((i + 1) % 2 == 0) {
                harmonic_freq_factor *= 0.1;  // Strongly suppress even harmonics
            }
        } else {
            // Suppress odd harmonics in single-ended (they cancel less effectively)
            if ((i + 1) % 2 != 0 && i > 0) {
                harmonic_freq_factor *= 0.5;  // Partially suppress odd harmonics
            }
        }
        
        // Apply the harmonic_distortion factor
        harmonic_content[i] = abs(signal) * harmonic_freq_factor * harmonic_distortion;
    }
}

double TubeAmplifier::CalculateTHD() const {
    // Calculate Total Harmonic Distortion
    if (harmonic_content.empty()) {
        return 0.0;
    }
    
    double fundamental = harmonic_content[0];
    if (fundamental == 0.0) {
        return 0.0;  // Avoid division by zero
    }
    
    double sum_of_harmonics = 0.0;
    for (size_t i = 1; i < harmonic_content.size(); i++) {
        sum_of_harmonics += harmonic_content[i] * harmonic_content[i];
    }
    
    return sqrt(sum_of_harmonics) / fundamental;
}

void TubeAmplifier::ApplyAmplifierClassCharacteristics() {
    // Apply characteristics specific to the amplifier class
    switch (amp_class) {
        case AmplifierClass::CLASS_A:
            // Class A conducts for the full 360 degrees
            // Uses higher bias current, lower efficiency, but low distortion
            if (bias_voltage > -1.0) {  // If bias is not set low enough
                bias_voltage = -1.0;    // Set appropriate bias for Class A
            }
            break;
            
        case AmplifierClass::CLASS_AB:
            // Class AB conducts for >180 but <360 degrees
            // Balance between efficiency and distortion
            if (bias_voltage > -0.5) {
                bias_voltage = -0.5;
            }
            break;
            
        case AmplifierClass::CLASS_B:
            // Class B conducts for ~180 degrees
            // Used in push-pull configurations, higher efficiency but crossover distortion
            if (bias_voltage > 0.0) {
                bias_voltage = 0.0;  // Close to 0V for Class B
            }
            break;
            
        case AmplifierClass::CLASS_C:
            // Class C conducts for <180 degrees
            // High efficiency but high distortion, used in RF applications
            if (bias_voltage > 0.5) {
                bias_voltage = 0.5;  // Positive bias for Class C
            }
            break;
    }
}


// SingleEndedAmp implementation
SingleEndedAmp::SingleEndedAmp() 
    : TubeAmplifier(1, AmplifierClass::CLASS_A, TubeConfiguration::SINGLE_ENDED) {
    // Add a single triode for single-ended operation
    tubes.clear();
    tubes.push_back(std::make_unique<Triode>(100.0, 62000.0, 1.6e-3)); // 12AX7 values
    
    // Set up for guitar amp characteristics
    SetLoadResistance(100000.0);  // 100k plate load
    SetPlateVoltage(250.0);       // 250V plate voltage
    SetBiasVoltage(-1.5);         // Appropriate bias for Class A
    SetGain(40.0);                // High gain for preamp stage
}

void SingleEndedAmp::ProcessSignal() {
    // Single-ended operation - only one tube processes the full signal
    double signal = input_signal;
    
    if (!tubes.empty()) {
        // Set tube parameters
        tubes[0]->SetGridVoltage(bias_voltage + signal * 0.1);
        tubes[0]->SetPlateVoltage(plate_voltage);
        tubes[0]->SetCathodeVoltage(0.0);
        
        // Calculate tube behavior
        tubes[0]->CalculateTubeBehavior();
        
        // Convert plate current to output voltage
        double plate_current = tubes[0]->GetPlateCurrent();
        signal = plate_current * GetLoadResistance() / 1000.0;  // Scale appropriately
    }
    
    // Apply distortion (single-ended amps have even-order harmonics)
    if (IsDistortionEnabled()) {
        signal = ApplyDistortion(signal);
    }
    
    // Apply tone controls
    signal = ApplyToneControls(signal);
    
    // Limit output
    output_signal = std::max(-5.0, std::min(5.0, signal));
}


// PushPullAmp implementation
PushPullAmp::PushPullAmp() 
    : TubeAmplifier(2, AmplifierClass::CLASS_AB, TubeConfiguration::PUSH_PULL) {
    // Add two tubes for push-pull operation
    tubes.clear();
    tubes.push_back(std::make_unique<Triode>(100.0, 62000.0, 1.6e-3)); // 12AX7 values
    tubes.push_back(std::make_unique<Triode>(100.0, 62000.0, 1.6e-3)); // 12AX7 values
    
    // Set up for power amp characteristics
    SetLoadResistance(8000.0);    // 8k output load (typical for guitar amp)
    SetPlateVoltage(420.0);       // Higher plate voltage for power amp
    SetBiasVoltage(-35.0);        // Negative bias for power tubes (like EL34)
    SetGain(20.0);                // Moderate gain for power amp
}

void PushPullAmp::ProcessSignal() {
    // Push-pull operation - signal is inverted for one tube
    double signal_positive = input_signal;
    double signal_negative = -input_signal;  // Inverted for the other tube
    
    double output_positive = 0.0;
    double output_negative = 0.0;
    
    if (tubes.size() >= 2) {
        // Process positive phase through first tube
        tubes[0]->SetGridVoltage(bias_voltage + signal_positive * 0.1);
        tubes[0]->SetPlateVoltage(plate_voltage);
        tubes[0]->SetCathodeVoltage(0.0);
        tubes[0]->CalculateTubeBehavior();
        output_positive = tubes[0]->GetPlateCurrent() * GetLoadResistance() / 1000.0;
        
        // Process negative phase through second tube
        tubes[1]->SetGridVoltage(bias_voltage + signal_negative * 0.1);
        tubes[1]->SetPlateVoltage(plate_voltage);
        tubes[1]->SetCathodeVoltage(0.0);
        tubes[1]->CalculateTubeBehavior();
        output_negative = tubes[1]->GetPlateCurrent() * GetLoadResistance() / 1000.0;
    }
    
    // The output is the difference between the two phases (differential output)
    double signal = output_positive - output_negative;
    
    // Apply distortion (push-pull amps have odd-order harmonics)
    if (IsDistortionEnabled()) {
        signal = ApplyDistortion(signal);
    }
    
    // Apply tone controls
    signal = ApplyToneControls(signal);
    
    // Limit output
    output_signal = std::max(-5.0, std::min(5.0, signal));
}


// ClassAChampAmp implementation (Fender Champ-style)
ClassAChampAmp::ClassAChampAmp() 
    : TubeAmplifier(1, AmplifierClass::CLASS_A, TubeConfiguration::CASCADE) {
    // Preamp stage (V1)
    tubes.push_back(std::make_unique<Triode>(100.0, 100000.0, 1.6e-3));  // 12AX7
    // Power stage (V2) - would normally be a power tube like 6V6, but using triode model
    tubes.push_back(std::make_unique<Triode>(15.0, 4700.0, 6.0e-3));     // Simplified power tube model
    
    SetLoadResistance(8000.0);  // 8-ohm speaker load reflected to primary
    SetPlateVoltage(250.0);     // B+ voltage
    SetBiasVoltage(-15.0);      // Typical for 6V6 power tube
    SetGain(35.0);              // Total gain for Champ-style
    SetDistortion(0.4);         // More distortion for classic tone
}

void ClassAChampAmp::ProcessSignal() {
    // Fender Champ-style single-ended Class A amplifier
    double signal = input_signal;
    
    // Stage 1: Preamp (V1)
    if (tubes.size() > 0) {
        tubes[0]->SetGridVoltage(bias_voltage/10.0 + signal * 0.1);  // Higher input sensitivity
        tubes[0]->SetPlateVoltage(plate_voltage);
        tubes[0]->SetCathodeVoltage(0.0);
        tubes[0]->CalculateTubeBehavior();
        
        double stage1_output = tubes[0]->GetPlateCurrent() * 100000.0 / 1000.0;  // High gain preamp
        signal = stage1_output;
    }
    
    // Stage 2: Power amp (V2) - single-ended Class A
    if (tubes.size() > 1) {
        // Apply power amp characteristics
        tubes[1]->SetGridVoltage(bias_voltage + signal * 0.02);  // Lower gain stage
        tubes[1]->SetPlateVoltage(plate_voltage);
        tubes[1]->SetCathodeVoltage(0.0);
        tubes[1]->CalculateTubeBehavior();
        
        double stage2_output = tubes[1]->GetPlateCurrent() * GetLoadResistance() / 1000.0;
        signal = stage2_output;
    }
    
    // Apply classic Champ-style distortion
    if (IsDistortionEnabled()) {
        // Overdrive the power section for that classic single-ended sag
        signal *= 1.2;  // Slightly overdrive
        
        // Apply soft clipping
        if (signal > 1.0) {
            signal = 1.0 + 0.5 * log(signal);
        } else if (signal < -1.0) {
            signal = -1.0 - 0.5 * log(-signal);
        }
    }
    
    // Apply simple tone controls like the Champ has
    signal = ApplyToneControls(signal);
    
    // Limit output
    output_signal = std::max(-5.0, std::min(5.0, signal));
}


// ClassABFenderTwinAmp implementation (Fender Twin-style)
ClassABFenderTwinAmp::ClassABFenderTwinAmp() 
    : TubeAmplifier(4, AmplifierClass::CLASS_AB, TubeConfiguration::CASCADE) {
    // Preamp stages (V1-V3)
    tubes.push_back(std::make_unique<Triode>(100.0, 100000.0, 1.6e-3));  // Phase inverter input
    tubes.push_back(std::make_unique<Triode>(100.0, 100000.0, 1.6e-3));  // Phase inverter output 1
    tubes.push_back(std::make_unique<Triode>(100.0, 100000.0, 1.6e-3));  // Phase inverter output 2
    // Power stage (V4-V5) - push-pull configuration
    tubes.push_back(std::make_unique<Triode>(20.0, 5000.0, 6.0e-3));     // Power tube 1 (simplified EL34 model)
    tubes.push_back(std::make_unique<Triode>(20.0, 5000.0, 6.0e-3));     // Power tube 2 (simplified EL34 model)
    
    SetLoadResistance(8000.0);  // 8-ohm speaker load reflected to primary
    SetPlateVoltage(430.0);     // Higher B+ voltage for Twin Reverb
    SetBiasVoltage(-38.0);      // Typical for EL34 in Class AB
    SetGain(25.0);              // Moderate gain but high power output
    SetDistortion(0.15);        // Lower distortion for cleaner Class AB operation
}

void ClassABFenderTwinAmp::ProcessSignal() {
    // Fender Twin-style Class AB push-pull amplifier
    double signal = input_signal;
    
    // Stage 1: Preamp (V1)
    if (tubes.size() > 0) {
        tubes[0]->SetGridVoltage(bias_voltage/20.0 + signal * 0.1);
        tubes[0]->SetPlateVoltage(plate_voltage);
        tubes[0]->SetCathodeVoltage(0.0);
        tubes[0]->CalculateTubeBehavior();
        
        signal = tubes[0]->GetPlateCurrent() * 100000.0 / 1000.0;
    }
    
    // Stage 2: Phase inverter (V2, V3) - generates inverted signal for push-pull
    double signal_inverted = 0.0;
    if (tubes.size() > 2) {
        // Standard long-tailed pair phase inverter
        tubes[1]->SetGridVoltage(bias_voltage/20.0 + signal * 0.05);
        tubes[1]->SetPlateVoltage(plate_voltage);
        tubes[1]->SetCathodeVoltage(0.0);
        tubes[1]->CalculateTubeBehavior();
        
        tubes[2]->SetGridVoltage(bias_voltage/20.0 - signal * 0.05);  // Inverted input
        tubes[2]->SetPlateVoltage(plate_voltage);
        tubes[2]->SetCathodeVoltage(0.0);
        tubes[2]->CalculateTubeBehavior();
        
        signal = tubes[1]->GetPlateCurrent() * 100000.0 / 1000.0;
        signal_inverted = tubes[2]->GetPlateCurrent() * 100000.0 / 1000.0;
    }
    
    // Stage 3: Power amp (V4, V5) - push-pull Class AB
    double output_positive = 0.0, output_negative = 0.0;
    if (tubes.size() > 4) {
        // Positive phase
        tubes[3]->SetGridVoltage(bias_voltage + signal * 0.1);
        tubes[3]->SetPlateVoltage(plate_voltage);
        tubes[3]->SetCathodeVoltage(0.0);
        tubes[3]->CalculateTubeBehavior();
        output_positive = tubes[3]->GetPlateCurrent() * GetLoadResistance() / 1000.0;
        
        // Negative phase
        tubes[4]->SetGridVoltage(bias_voltage + signal_inverted * 0.1);
        tubes[4]->SetPlateVoltage(plate_voltage);
        tubes[4]->SetCathodeVoltage(0.0);
        tubes[4]->CalculateTubeBehavior();
        output_negative = tubes[4]->GetPlateCurrent() * GetLoadResistance() / 1000.0;
    }
    
    // The output is the difference between the two phases
    signal = output_positive - output_negative;
    
    // Apply Class AB characteristics
    if (IsDistortionEnabled()) {
        // Apply soft clipping characteristic of Class AB
        if (signal > 2.0) {
            signal = 2.0 + (signal - 2.0) * 0.3;  // Gentle limiting
        } else if (signal < -2.0) {
            signal = -2.0 + (signal + 2.0) * 0.3;
        }
    }
    
    // Apply Twin-style tone stack (simplified)
    signal = ApplyToneControls(signal);
    
    // Limit output
    output_signal = std::max(-5.0, std::min(5.0, signal));
}