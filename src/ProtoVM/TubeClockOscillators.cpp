#include "TubeClockOscillators.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// TubeClockOscillator implementation
TubeClockOscillator::TubeClockOscillator(OscillatorType type, double frequency) 
    : oscillatorType(type), frequency(frequency) {
    waveform = TubeOscillator::SINE;
    initializeOscillator();
    updatePhaseIncrement();
    oscillating = true;
}

void TubeClockOscillator::initializeOscillator() {
    oscillator = std::make_unique<TubeOscillator>(
        static_cast<TubeOscillator::OscillatorType>(oscillatorType));
    oscillator->setFrequency(frequency);
    oscillator->setWaveform(static_cast<TubeOscillator::Waveform>(waveform));
    oscillator->setAmplitude(amplitude);
    oscillator->start();
}

bool TubeClockOscillator::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeClockOscillator::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == frequencyControlPin && data_bytes == sizeof(double)) {
        double controlVoltage;
        memcpy(&controlVoltage, data, sizeof(double));
        
        // Update frequency based on control voltage
        double newFreq = frequency * (1.0 + controlVoltage * 0.1); // 10% modulation
        setFrequency(newFreq);
        return true;
    } else if (conn_id == enablePin && data_bytes == sizeof(double)) {
        bool enableSignal;
        memcpy(&enableSignal, data, sizeof(bool));
        setEnable(enableSignal);
        return true;
    } else if (conn_id == syncPin && data_bytes == sizeof(double)) {
        memcpy(&syncSignal, data, sizeof(double));
        return true;
    }
    return false;
}

bool TubeClockOscillator::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == outputPin && data_bytes == sizeof(double)) {
        memcpy(data, &currentOutput, sizeof(double));
        return true;
    } else if (conn_id == clockOutputPin && data_bytes == sizeof(bool)) {
        bool clockOut = clockOutput;
        memcpy(data, &clockOut, sizeof(bool));
        return true;
    }
    return false;
}

bool TubeClockOscillator::Tick() {
    if (!enabled) {
        currentOutput = 0.0;
        clockOutput = false;
        return true;
    }
    
    // Generate the next sample
    currentOutput = generateNextSample();
    
    // Generate digital clock output from analog signal
    if (currentOutput > threshold && previousOutput <= threshold) {
        // Positive going transition
        clockOutput = true;
    } else if (currentOutput < threshold && previousOutput >= threshold) {
        // Negative going transition
        clockOutput = false;
    } else {
        // No transition
        clockOutput = previousClock;
    }
    
    // Update previous values
    previousOutput = currentOutput;
    previousClock = clockOutput;
    
    return true;
}

double TubeClockOscillator::generateNextSample() {
    if (!enabled || !oscillating) {
        return 0.0;
    }
    
    // Apply sync if enabled
    if (syncEnabled && syncSignal > 0.5 && currentOutput < 0) {
        // Reset phase on positive transition
        phase = 0.0;
    }
    
    // Update phase increment based on current frequency
    double newPhaseIncrement = 2.0 * M_PI * frequency / sampleRate;
    
    // Generate waveform based on type
    double sample = 0.0;
    switch (waveform) {
        case TubeOscillator::SINE:
            sample = sin(phase);
            break;
        case TubeOscillator::TRIANGLE:
            if (phase < M_PI) {
                sample = (2.0 * phase / M_PI) - 1.0;
            } else {
                sample = 1.0 - (2.0 * (phase - M_PI) / M_PI);
            }
            break;
        case TubeOscillator::SAWTOOTH:
            sample = (phase / M_PI) - 1.0;
            break;
        case TubeOscillator::SQUARE:
            sample = (phase < M_PI) ? 1.0 : -1.0;
            break;
    }
    
    // Apply amplitude
    sample *= amplitude;
    
    // Increment phase
    phase += newPhaseIncrement;
    if (phase >= 2.0 * M_PI) {
        phase -= 2.0 * M_PI;
    }
    
    return sample;
}

void TubeClockOscillator::setFrequency(double freq) {
    frequency = std::max(0.1, std::min(100000.0, freq)); // Limit to reasonable range
    updatePhaseIncrement();
    if (oscillator) {
        oscillator->setFrequency(freq);
    }
}

void TubeClockOscillator::updatePhaseIncrement() {
    phaseIncrement = 2.0 * M_PI * frequency / sampleRate;
}


// TubeClockDivider implementation
TubeClockDivider::TubeClockDivider(int divideFactor) : divideFactor(divideFactor) {
    if (divideFactor < 1) divideFactor = 1;
    resetCounter();
}

bool TubeClockDivider::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeClockDivider::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == inputPin && data_bytes == sizeof(bool)) {
        bool input;
        memcpy(&input, data, sizeof(bool));
        if (enabled) {
            processInput(input);
        }
        return true;
    } else if (conn_id == enablePin && data_bytes == sizeof(bool)) {
        memcpy(&enabled, data, sizeof(bool));
        return true;
    } else if (conn_id == resetPin && data_bytes == sizeof(bool)) {
        bool reset;
        memcpy(&reset, data, sizeof(bool));
        if (reset) {
            resetCounter();
        }
        return true;
    }
    return false;
}

bool TubeClockDivider::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == outputPin && data_bytes == sizeof(bool)) {
        memcpy(data, &output, sizeof(bool));
        return true;
    }
    return false;
}

bool TubeClockDivider::Tick() {
    // The divider state is updated in PutRaw when input changes
    return true;
}

void TubeClockDivider::processInput(bool input) {
    // Detect rising edge of input
    if (input && !previousInput) {
        // Rising edge detected
        currentCount++;
        if (currentCount >= divideFactor) {
            currentCount = 0;
            output = !output;  // Toggle output
        }
    }
    previousInput = input;
}

void TubeClockDivider::setDivideFactor(int factor) {
    if (factor < 1) factor = 1;
    divideFactor = factor;
    resetCounter();
}

void TubeClockDivider::resetCounter() {
    currentCount = 0;
    output = false;
    previousInput = false;
}


// TubePLL implementation
TubePLL::TubePLL() {
    vco.setFrequency(vcoFreq);
    vco.start();
}

bool TubePLL::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubePLL::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == referencePin && data_bytes == sizeof(bool)) {
        memcpy(&referenceClock, data, sizeof(bool));
        return true;
    } else if (conn_id == resetPin && data_bytes == sizeof(bool)) {
        bool reset;
        memcpy(&reset, data, sizeof(bool));
        if (reset) {
            reset();
        }
        return true;
    }
    return false;
}

bool TubePLL::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == outputPin && data_bytes == sizeof(bool)) {
        memcpy(data, &output, sizeof(bool));
        return true;
    } else if (conn_id == controlPin && data_bytes == sizeof(double)) {
        memcpy(data, &controlVoltage, sizeof(double));
        return true;
    }
    return false;
}

bool TubePLL::Tick() {
    // Process phase detection
    processPhaseDetection();
    
    // Process loop filter
    processLoopFilter();
    
    // Update VCO
    updateVCO();
    
    // Generate output (for this simulation, use VCO output)
    output = vco.getOutput() > 0.0;
    
    // Update previous states
    previousRefClock = referenceClock;
    previousVCOClock = output;
    
    return true;
}

void TubePLL::processPhaseDetection() {
    // Simple XOR phase detector: output high when inputs are different
    if (phaseDetectorType == 0) {  // XOR type
        // Detect edges to determine phase relationship
        if (referenceClock && !previousRefClock) {
            // Reference rising edge
            referenceEdge = true;
            if (output) {
                // Feedback is high when reference rises -> reference leads -> increase VCO
                phaseError = 0.5;
            } else {
                // Feedback is low when reference rises -> reference lags -> decrease VCO
                phaseError = -0.5;
            }
        }
    }
}

void TubePLL::processLoopFilter() {
    // Simple RC low-pass filter for loop filter
    double alpha = loopFilterCutoff / sampleRate;
    loopFilterState = loopFilterState * (1.0 - alpha) + phaseError * alpha;
    controlVoltage = loopFilterState;
}

void TubePLL::updateVCO() {
    // Update VCO frequency based on control voltage
    double newFreq = vcoFreq * (1.0 + controlVoltage * 0.1); // 10% control range
    vco.setFrequency(newFreq);
    
    // Generate VCO output
    vco.Tick();
}


// TubeFrequencySynthesizer implementation
TubeFrequencySynthesizer::TubeFrequencySynthesizer(SynthesisMethod method) 
    : synthesisMethod(method) {
    pll = std::make_unique<TubePLL>();
    oscillator = std::make_unique<TubeClockOscillator>(TubeClockOscillator::WIEN_BRIDGE, referenceFreq);
    predivider = std::make_unique<TubeClockDivider>(1);  // No division initially
    postdivider = std::make_unique<TubeClockDivider>(1); // No division initially
    
    configurePLL();
}

bool TubeFrequencySynthesizer::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeFrequencySynthesizer::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == referencePin && data_bytes == sizeof(double)) {
        memcpy(&referenceFreq, data, sizeof(double));
        return true;
    } else if (conn_id == controlPin && data_bytes == sizeof(double)) {
        // Frequency control
        double freqControl;
        memcpy(&freqControl, data, sizeof(double));
        setOutputFrequency(outputFreq * (1.0 + freqControl)); // Adjust based on control value
        return true;
    } else if (conn_id == enablePin && data_bytes == sizeof(bool)) {
        memcpy(&enabled, data, sizeof(bool));
        return true;
    } else if (conn_id == resetPin && data_bytes == sizeof(bool)) {
        bool reset;
        memcpy(&reset, data, sizeof(bool));
        if (reset) {
            locked = false;
        }
        return true;
    }
    return false;
}

bool TubeFrequencySynthesizer::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == outputPin && data_bytes == sizeof(double)) {
        double output_freq = outputFreq;
        memcpy(data, &output_freq, sizeof(double));
        return true;
    }
    return false;
}

bool TubeFrequencySynthesizer::Tick() {
    updateOutput();
    return true;
}

void TubeFrequencySynthesizer::setOutputFrequency(double freq) {
    outputFreq = freq;
    configurePLL();
}

void TubeFrequencySynthesizer::configurePLL() {
    if (synthesisMethod == PLL_BASED) {
        // Calculate division ratios for PLL
        // For this example, use integer division
        nCounter = static_cast<int>(outputFreq / referenceFreq);
        if (nCounter < 1) nCounter = 1;
        
        // Update post-dividers based on fractional needs
        if (outputFreq > referenceFreq) {
            // Multiply: use post-multiplier
            postdivider->setDivideFactor(1); // Effectively multiplication
        } else {
            // Divide: use post-divider
            postdivider->setDivideFactor(nCounter);
        }
        
        // Configure the VCO frequency
        if (pll) {
            pll->setReferenceFrequency(referenceFreq * nCounter);
        }
    }
}

void TubeFrequencySynthesizer::updateOutput() {
    if (!enabled) {
        return;
    }
    
    // In a real system, this would update the output based on the synthesis method
    // For this simulation, we'll just track if the system is "locked"
    locked = true; // Simplified for simulation
}


// TubeClockSystem implementation
TubeClockSystem::TubeClockSystem() {
    initializeSystem();
}

void TubeClockSystem::initializeSystem() {
    // Create master oscillator
    masterOsc = std::make_unique<TubeClockOscillator>(TubeClockOscillator::WIEN_BRIDGE, masterFreq);
    
    // Create frequency dividers: divide by 2, 4, 8
    freqDividers.push_back(std::make_unique<TubeClockDivider>(2));
    freqDividers.push_back(std::make_unique<TubeClockDivider>(4));
    freqDividers.push_back(std::make_unique<TubeClockDivider>(8));
    
    // Connect the dividers in series from the master clock
    // This is a simplified approach; in reality, each would get its input separately
}

bool TubeClockSystem::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeClockSystem::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == enablePin && data_bytes == sizeof(bool)) {
        bool enable;
        memcpy(&enable, data, sizeof(bool));
        setEnable(enable);
        return true;
    } else if (conn_id == resetAllPin && data_bytes == sizeof(bool)) {
        bool reset;
        memcpy(&reset, data, sizeof(bool));
        if (reset) {
            // Reset all dividers
            for (auto& divider : freqDividers) {
                divider->setReset(true);
            }
        }
        return true;
    }
    return false;
}

bool TubeClockSystem::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == masterClockPin && data_bytes == sizeof(bool)) {
        bool master = getMasterClock();
        memcpy(data, &master, sizeof(bool));
        return true;
    } else if (conn_id == halfClockPin && data_bytes == sizeof(bool)) {
        bool half = getHalfClock();
        memcpy(data, &half, sizeof(bool));
        return true;
    } else if (conn_id == quarterClockPin && data_bytes == sizeof(bool)) {
        bool quarter = getQuarterClock();
        memcpy(data, &quarter, sizeof(bool));
        return true;
    } else if (conn_id == eighthClockPin && data_bytes == sizeof(bool)) {
        bool eighth = getEighthClock();
        memcpy(data, &eighth, sizeof(bool));
        return true;
    }
    return false;
}

bool TubeClockSystem::Tick() {
    // Update master oscillator
    masterOsc->Tick();
    
    // Update dividers - connect them to the appropriate inputs
    bool masterClock = masterOsc->getClockOutput();
    
    // First divider: gets master clock
    freqDividers[0]->PutRaw(0, reinterpret_cast<byte*>(&masterClock), sizeof(bool), 0);
    freqDividers[0]->Tick();
    
    // Second divider: gets output of first divider
    bool firstDivOutput = freqDividers[0]->getOutput();
    freqDividers[1]->PutRaw(0, reinterpret_cast<byte*>(&firstDivOutput), sizeof(bool), 0);
    freqDividers[1]->Tick();
    
    // Third divider: gets output of second divider
    bool secondDivOutput = freqDividers[1]->getOutput();
    freqDividers[2]->PutRaw(0, reinterpret_cast<byte*>(&secondDivOutput), sizeof(bool), 0);
    freqDividers[2]->Tick();
    
    return true;
}