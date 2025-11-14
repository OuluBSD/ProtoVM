#include "VCA.h"
#include <cmath>
#include <algorithm>

VCA::VCA(VCACharacteristic characteristic, double gain)
    : characteristic(characteristic)
    , gain(gain)
    , control_voltage(0.0)
    , input_signal(0.0)
    , output(0.0)
    , cv_sensitivity(CV_SENSITIVITY_DEFAULT)
{
}

bool VCA::Tick() {
    // Calculate effective gain based on control voltage and characteristic
    double effective_gain = gain;
    
    // Adjust gain based on control voltage and characteristic
    switch (characteristic) {
        case VCACharacteristic::LINEAR:
            // Linear response to control voltage
            effective_gain *= (1.0 + (control_voltage / MAX_CV) * cv_sensitivity);
            break;
            
        case VCACharacteristic::EXPONENTIAL:
            // Exponential response - typical for audio applications
            // This mimics the exponential response of transistor circuits
            effective_gain *= exp(control_voltage * cv_sensitivity * 0.1); // Scale factor to prevent excessive gain
            break;
            
        case VCACharacteristic::LOGARITHMIC:
            // Logarithmic response
            if (control_voltage > 0.001) { // Avoid log(0)
                effective_gain *= (1.0 + log(control_voltage) * cv_sensitivity);
            } else {
                effective_gain *= (1.0 + log(0.001) * cv_sensitivity); // Minimum value
            }
            break;
    }
    
    // Clamp the effective gain to prevent instability
    effective_gain = std::max(MIN_GAIN, std::min(MAX_GAIN, effective_gain));
    
    // Apply the gain to the input signal
    output = input_signal * effective_gain;
    
    return true;
}

void VCA::SetCharacteristic(VCACharacteristic characteristic) {
    this->characteristic = characteristic;
}

void VCA::SetGain(double gain) {
    this->gain = std::max(MIN_GAIN, std::min(MAX_GAIN, gain));
}

void VCA::SetControlVoltage(double cv) {
    this->control_voltage = std::max(MIN_CV, std::min(MAX_CV, cv));
}

void VCA::SetInput(double input) {
    this->input_signal = input;
}

void VCA::SetCVSensitivity(double sensitivity) {
    this->cv_sensitivity = std::max(0.0, std::min(10.0, sensitivity));
}