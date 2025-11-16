#include "VoltageSources.h"

// DC Voltage Source (2-terminal) implementation
DcVoltageSource::DcVoltageSource(double voltage) : voltage(voltage) {
    analog_values.resize(2, 0.0);  // Two terminals
    // Set one terminal to the voltage, other to ground reference
    analog_values[0] = voltage;    // Positive terminal
    analog_values[1] = 0.0;        // Negative terminal (ground reference)
}

bool DcVoltageSource::Tick() {
    // Maintain constant voltage difference between terminals
    analog_values[0] = voltage;    // Positive terminal
    analog_values[1] = 0.0;        // Negative terminal (ground reference)
    
    // Output voltage to connected components
    UpdateAnalogValue(0, voltage);
    UpdateAnalogValue(1, 0.0);
    
    return true;  // Always outputs the same voltage
}

void DcVoltageSource::SetVoltage(double v) {
    voltage = v;
}

// AC Voltage Source (2-terminal) implementation
AcVoltageSource::AcVoltageSource(double amplitude, double frequency, double offset) 
    : amplitude(amplitude), frequency(frequency), offset(offset) {
    analog_values.resize(2, 0.0);  // Two terminals
    analog_values[1] = 0.0;        // Reference terminal (ground)
}

bool AcVoltageSource::Tick() {
    // Calculate instantaneous AC voltage based on current simulation time
    double time = simulation_time;
    double instantaneous_voltage = amplitude * sin(2.0 * PI * frequency * time) + offset;
    
    // Apply voltage to the output terminal relative to reference
    analog_values[0] = instantaneous_voltage;
    analog_values[1] = 0.0;  // Reference terminal
    
    // Output voltage to connected components
    UpdateAnalogValue(0, instantaneous_voltage);
    UpdateAnalogValue(1, 0.0);
    
    return true;
}

void AcVoltageSource::SetAmplitude(double amp) {
    amplitude = amp;
}

void AcVoltageSource::SetFrequency(double freq) {
    frequency = freq;
}

void AcVoltageSource::SetOffset(double offset_val) {
    offset = offset_val;
}

// DC Voltage Source (1-terminal) implementation
DcVoltageSource1T::DcVoltageSource1T(double voltage) : voltage(voltage) {
    analog_values.resize(1, voltage);  // Single terminal relative to system ground
}

bool DcVoltageSource1T::Tick() {
    // Maintain constant voltage output
    analog_values[0] = voltage;
    UpdateAnalogValue(0, voltage);
    
    return true;
}

void DcVoltageSource1T::SetVoltage(double v) {
    voltage = v;
}

// AC Voltage Source (1-terminal) implementation
AcVoltageSource1T::AcVoltageSource1T(double amplitude, double frequency, double offset) 
    : amplitude(amplitude), frequency(frequency), offset(offset) {
    analog_values.resize(1, 0.0);  // Single terminal relative to system ground
}

bool AcVoltageSource1T::Tick() {
    // Calculate instantaneous AC voltage based on current simulation time
    double time = simulation_time;
    double instantaneous_voltage = amplitude * sin(2.0 * PI * frequency * time) + offset;
    
    // Apply voltage to the single terminal (relative to system ground)
    analog_values[0] = instantaneous_voltage;
    UpdateAnalogValue(0, instantaneous_voltage);
    
    return true;
}

void AcVoltageSource1T::SetAmplitude(double amp) {
    amplitude = amp;
}

void AcVoltageSource1T::SetFrequency(double freq) {
    frequency = freq;
}

void AcVoltageSource1T::SetOffset(double offset_val) {
    offset = offset_val;
}

// Square Wave Source (1-terminal) implementation
SquareWaveSource::SquareWaveSource(double amplitude, double frequency, double offset) 
    : amplitude(amplitude), frequency(frequency), offset(offset) {
    analog_values.resize(1, offset + amplitude);  // Start at positive peak
}

bool SquareWaveSource::Tick() {
    // Calculate instantaneous square wave voltage based on current simulation time
    double time = simulation_time;
    double period = 1.0 / frequency;
    double phase = fmod(time, period);
    
    double instantaneous_voltage;
    if (phase < period / 2.0) {
        instantaneous_voltage = amplitude + offset;  // High state
    } else {
        instantaneous_voltage = -amplitude + offset; // Low state
    }
    
    analog_values[0] = instantaneous_voltage;
    UpdateAnalogValue(0, instantaneous_voltage);
    
    return true;
}

void SquareWaveSource::SetAmplitude(double amp) {
    amplitude = amp;
}

void SquareWaveSource::SetFrequency(double freq) {
    frequency = freq;
}

void SquareWaveSource::SetOffset(double offset_val) {
    offset = offset_val;
}

// Clock Source (1-terminal) implementation
ClockSource::ClockSource(double frequency, double duty_cycle) 
    : frequency(frequency), duty_cycle(duty_cycle), phase(0.0) {
    analog_values.resize(1, 0.0);  // Single terminal relative to system ground
}

bool ClockSource::Tick() {
    // Calculate instantaneous clock voltage based on current simulation time
    double time = simulation_time;
    double period = 1.0 / frequency;
    double high_duration = period * duty_cycle;
    double phase = fmod(time, period);
    
    double instantaneous_voltage;
    if (phase < high_duration) {
        instantaneous_voltage = 5.0;  // High state (5V typical for digital)
    } else {
        instantaneous_voltage = 0.0;  // Low state (0V)
    }
    
    analog_values[0] = instantaneous_voltage;
    UpdateAnalogValue(0, instantaneous_voltage);
    
    return true;
}

void ClockSource::SetFrequency(double freq) {
    frequency = freq;
}

void ClockSource::SetDutyCycle(double duty) {
    duty_cycle = duty;
    if (duty_cycle < 0.0) duty_cycle = 0.0;
    if (duty_cycle > 1.0) duty_cycle = 1.0;
}

// AC Sweep Source implementation
AcSweepSource::AcSweepSource(double start_freq, double stop_freq, double amplitude, double duration) 
    : start_freq(start_freq), stop_freq(stop_freq), amplitude(amplitude), duration(duration), current_time(0.0) {
    analog_values.resize(1, 0.0);  // Single terminal relative to system ground
}

bool AcSweepSource::Tick() {
    // Update simulation time reference
    current_time = simulation_time;
    
    // Calculate instantaneous frequency for logarithmic sweep
    double normalized_time = current_time / duration;
    if (normalized_time > 1.0) normalized_time = 1.0;  // Clamp to duration
    
    // Logarithmic frequency sweep: f(t) = start_freq * (stop_freq/start_freq)^(t/duration)
    double current_freq = start_freq * pow(stop_freq / start_freq, normalized_time);
    
    // Calculate instantaneous voltage based on current frequency and time
    double instantaneous_voltage = amplitude * sin(2.0 * PI * current_freq * current_time);
    
    analog_values[0] = instantaneous_voltage;
    UpdateAnalogValue(0, instantaneous_voltage);
    
    return true;
}

void AcSweepSource::SetStartFrequency(double freq) {
    start_freq = freq;
}

void AcSweepSource::SetStopFrequency(double freq) {
    stop_freq = freq;
}

void AcSweepSource::SetAmplitude(double amp) {
    amplitude = amp;
}

void AcSweepSource::SetDuration(double dur) {
    duration = dur;
}

// Variable Voltage Source (1-terminal) implementation
VariableVoltageSource::VariableVoltageSource(double min_voltage, double max_voltage, double initial_voltage) 
    : min_voltage(min_voltage), max_voltage(max_voltage) {
    voltage = (initial_voltage < min_voltage) ? min_voltage : 
              (initial_voltage > max_voltage) ? max_voltage : initial_voltage;
    analog_values.resize(1, voltage);  // Single terminal relative to system ground
}

bool VariableVoltageSource::Tick() {
    // Clamp voltage to allowed range
    if (voltage < min_voltage) voltage = min_voltage;
    if (voltage > max_voltage) voltage = max_voltage;
    
    analog_values[0] = voltage;
    UpdateAnalogValue(0, voltage);
    
    return true;
}

void VariableVoltageSource::SetVoltage(double v) {
    voltage = v;
}

void VariableVoltageSource::SetMinVoltage(double v) {
    min_voltage = v;
}

void VariableVoltageSource::SetMaxVoltage(double v) {
    max_voltage = v;
}