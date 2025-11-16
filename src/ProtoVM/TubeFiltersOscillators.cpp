#include "TubeFiltersOscillators.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// TubeFilter implementation
TubeFilter::TubeFilter(FilterType type, CircuitTopology topology) 
    : filterType(type), circuitTopology(topology) {
    
    // Set defaults based on filter type
    switch (circuitTopology) {
        case RC_LPF:
        case RC_HPF:
            resistance = 10000.0;   // 10kΩ
            capacitance = 0.000001; // 10μF
            cutoffFreq = 1.0 / (2.0 * M_PI * resistance * capacitance);
            break;
        case LC_BANDPASS:
            inductance = 0.1;       // 100mH
            capacitance = 0.0000001; // 0.1μF
            cutoffFreq = 1.0 / (2.0 * M_PI * sqrt(inductance * capacitance));
            break;
        case TUBE_RC_LPF:
            resistance = 47000.0;   // 47kΩ
            capacitance = 0.00000047; // 0.47μF
            cutoffFreq = 1.0 / (2.0 * M_PI * resistance * capacitance);
            // Add a tube buffer
            tubeBuffer = std::make_unique<TriodeComponent>();
            break;
        default:
            resistance = 10000.0;
            capacitance = 0.000001;
            cutoffFreq = 1.0 / (2.0 * M_PI * resistance * capacitance);
            break;
    }
    
    // Initialize history buffers for IIR filtering
    inputHistory.resize(2, 0.0);
    outputHistory.resize(2, 0.0);
}

bool TubeFilter::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeFilter::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == inputPin && data_bytes == sizeof(double)) {
        memcpy(&inputSignal, data, sizeof(double));
        return true;
    } else if (conn_id == controlPin && data_bytes == sizeof(double)) {
        // Control voltage for voltage-controlled filter
        double controlVoltage;
        memcpy(&controlVoltage, data, sizeof(double));
        
        // Adjust cutoff frequency based on control voltage
        setCutoffFrequency(cutoffFreq * (1.0 + controlVoltage * 0.1)); // 10% per volt
        return true;
    }
    return false;
}

bool TubeFilter::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == outputPin && data_bytes == sizeof(double)) {
        memcpy(data, &outputSignal, sizeof(double));
        return true;
    }
    return false;
}

bool TubeFilter::Tick() {
    processSignal();
    return true;
}

void TubeFilter::processSignal() {
    // Calculate filter response using appropriate method based on topology
    switch (circuitTopology) {
        case RC_LPF: {
            // Simple RC low-pass filter using IIR implementation
            double RC = 1.0 / (2.0 * M_PI * cutoffFreq);
            double dt = 1.0 / sampleRate;
            double alpha = dt / (RC + dt);
            
            // First-order IIR filter
            outputSignal = alpha * inputSignal + (1.0 - alpha) * outputHistory[0];
            break;
        }
        case RC_HPF: {
            // RC high-pass filter
            double RC = 1.0 / (2.0 * M_PI * cutoffFreq);
            double dt = 1.0 / sampleRate;
            double alpha = RC / (RC + dt);
            
            // High-pass IIR filter
            outputSignal = alpha * outputHistory[0] + alpha * (inputSignal - inputHistory[0]);
            break;
        }
        case LC_BANDPASS: {
            // Simplified LC bandpass (resonant circuit)
            // This is a more complex implementation in reality
            double omega = 2.0 * M_PI * cutoffFreq;
            double omega_dt = omega / sampleRate;
            
            // For demonstration, use a simplified resonant filter
            double resonance_factor = std::max(0.1, std::min(10.0, resonance));
            outputSignal = inputSignal * resonance_factor * omega_dt;
            break;
        }
        case TUBE_RC_LPF: {
            // RC filter with tube buffer
            double RC = 1.0 / (2.0 * M_PI * cutoffFreq);
            double dt = 1.0 / sampleRate;
            double alpha = dt / (RC + dt);
            
            // Apply RC filtering
            double filtered = alpha * inputSignal + (1.0 - alpha) * outputHistory[0];
            
            // Apply tube buffer effect (mild non-linearity and buffering)
            if (tubeBuffer) {
                // Simplified tube buffering effect
                filtered *= 0.95;  // Slight gain reduction
                if (filtered > 0.8) filtered = 0.8 + 0.2 * tanh((filtered - 0.8) * 5.0);  // Soft limiting
                if (filtered < -0.8) filtered = -0.8 + 0.2 * tanh((filtered + 0.8) * 5.0);
            }
            
            outputSignal = filtered;
            break;
        }
        default:
            outputSignal = inputSignal;
            break;
    }
    
    // Apply gain
    outputSignal *= filterGain;
    
    // Update history
    inputHistory[1] = inputHistory[0];
    inputHistory[0] = inputSignal;
    outputHistory[1] = outputHistory[0];
    outputHistory[0] = outputSignal;
}

void TubeFilter::setCutoffFrequency(double freq) {
    cutoffFreq = std::max(20.0, std::min(20000.0, freq)); // Limit to audio range
    
    // Update component values if needed based on new frequency
    switch (circuitTopology) {
        case RC_LPF:
        case RC_HPF:
            // Keep RC constant, just update frequency response calculation
            break;
        case LC_BANDPASS:
            // Adjust for LC resonant frequency
            // Either adjust L or C; we'll adjust C for this example
            capacitance = 1.0 / (4.0 * M_PI * M_PI * freq * freq * inductance);
            break;
        case TUBE_RC_LPF:
            // Adjust RC values
            double RC = 1.0 / (2.0 * M_PI * freq);
            // For now, just keep the ratio, adjust as needed
            break;
    }
}

void TubeFilter::setResonance(double res) {
    resonance = std::max(0.1, std::min(10.0, res));
}


// TubeOscillator implementation
TubeOscillator::TubeOscillator(OscillatorType type) : oscillatorType(type), waveform(SINE) {
    initOscillator();
    start();
}

void TubeOscillator::initOscillator() {
    // Set defaults based on oscillator type
    switch (oscillatorType) {
        case HARTLEY:
        case COLPITTS:
            inductance = 0.0001;      // 100μH
            capacitance = 0.0000001;  // 0.1μF
            frequency = 1.0 / (2.0 * M_PI * sqrt(inductance * (capacitance + capacitance))); // Approximation
            break;
        case PIERCE:
            // Crystal oscillator (simplified)
            frequency = 1000000.0; // 1MHz default
            break;
        case WIEN_BRIDGE:
            resistance = 10000.0;     // 10kΩ
            capacitance = 0.000001;   // 10μF
            frequency = 1.0 / (2.0 * M_PI * resistance * capacitance);
            break;
        case PHASE_SHIFT:
            resistance = 10000.0;     // 10kΩ
            capacitance = 0.000001;   // 10μF
            frequency = 1.0 / (2.0 * M_PI * sqrt(6) * resistance * capacitance); // Approximation
            break;
        case RELAXATION:
            resistance = 10000.0;
            capacitance = 0.000001;
            frequency = 1.0 / (0.693 * resistance * capacitance); // For 555-style relaxation oscillator
            break;
        default:
            frequency = 440.0;  // A4
            break;
    }
    
    updatePhaseIncrement();
    
    // Create tube components
    oscillatorTube = std::make_unique<TriodeComponent>();
    bufferTube = std::make_unique<TriodeComponent>();
}

bool TubeOscillator::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeOscillator::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == frequencyControlPin && data_bytes == sizeof(double)) {
        // Control voltage for VCO functionality
        double controlVoltage;
        memcpy(&controlVoltage, data, sizeof(double));
        
        // Update frequency based on control voltage
        double freqMod = 1.0 + controlVoltage * 0.1; // 10% per volt
        setFrequency(frequency * freqMod);
        return true;
    } else if (conn_id == syncPin && data_bytes == sizeof(double)) {
        // Hard sync input
        memcpy(&syncSignal, data, sizeof(double));
        if (syncEnabled && syncSignal > 0.5 && currentOutput < 0) {
            // Reset phase on positive transition
            phase = 0.0;
        }
        return true;
    } else if (conn_id == modulationPin && data_bytes == sizeof(double)) {
        // Modulation input (for FM/AM)
        memcpy(&modulationSignal, data, sizeof(double));
        return true;
    }
    return false;
}

bool TubeOscillator::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == outputPin && data_bytes == sizeof(double)) {
        memcpy(data, &currentOutput, sizeof(double));
        return true;
    }
    return false;
}

bool TubeOscillator::Tick() {
    if (oscillating) {
        currentOutput = generateNextSample();
    } else {
        currentOutput = 0.0;
    }
    return true;
}

double TubeOscillator::generateNextSample() {
    double sample = 0.0;
    
    // Apply modulation if enabled
    double modulatedFreq = frequency;
    if (modulationEnabled) {
        modulatedFreq *= (1.0 + modulationSignal * 0.1); // 10% modulation depth
    }
    
    // Update phase increment based on possibly modulated frequency
    double newPhaseIncrement = 2.0 * M_PI * modulatedFreq / sampleRate;
    
    // Generate waveform based on type
    switch (waveform) {
        case SINE:
            sample = sin(phase);
            break;
        case TRIANGLE:
            // Convert phase to triangle wave
            if (phase < M_PI) {
                sample = (2.0 * phase / M_PI) - 1.0;
            } else {
                sample = 1.0 - (2.0 * (phase - M_PI) / M_PI);
            }
            break;
        case SAWTOOTH:
            sample = (phase / M_PI) - 1.0;
            break;
        case SQUARE:
            sample = (phase < M_PI) ? 1.0 : -1.0;
            break;
    }
    
    // Apply amplitude
    sample *= amplitude;
    
    // Apply tube distortion characteristics (mild)
    if (oscillatorTube) {
        // Simple tube-style soft clipping
        if (sample > 0.7) sample = 0.7 + 0.3 * tanh((sample - 0.7) / 0.3);
        if (sample < -0.7) sample = -0.7 + 0.3 * tanh((sample + 0.7) / 0.3);
    }
    
    // Increment phase
    phase += newPhaseIncrement;
    if (phase >= 2.0 * M_PI) {
        phase -= 2.0 * M_PI;
    }
    
    return sample;
}

void TubeOscillator::updatePhaseIncrement() {
    phaseIncrement = 2.0 * M_PI * frequency / sampleRate;
}

void TubeOscillator::setFrequency(double freq) {
    frequency = std::max(0.01, std::min(20000.0, freq)); // Limit to reasonable range
    updatePhaseIncrement();
}

void TubeOscillator::start() {
    oscillating = true;
    phase = 0.0; // Start at zero phase
}

void TubeOscillator::stop() {
    oscillating = false;
    currentOutput = 0.0;
}


// TubeVCO implementation
TubeVCO::TubeVCO() : TubeOscillator(WIEN_BRIDGE) {
    minFrequency = 20.0;
    maxFrequency = 20000.0;
}

void TubeVCO::setControlRange(double minFreq, double maxFreq) {
    minFrequency = std::max(0.1, minFreq);
    maxFrequency = std::min(100000.0, maxFreq);
}

double TubeVCO::controlVoltageToFrequency(double controlVoltage) {
    if (linearControl) {
        // Linear mapping: 0V = minFreq, 10V = maxFreq
        return minFrequency + (maxFrequency - minFrequency) * controlVoltage / 10.0;
    } else {
        // Exponential mapping: 1V/octave (2:1 frequency ratio per volt)
        return minFrequency * pow(2.0, controlVoltage);
    }
}