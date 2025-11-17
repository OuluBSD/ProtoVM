#include "TubeReverbCircuits.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// SpringReverb implementation
SpringReverb::SpringReverb(SpringType type) : springType(type) {
    initializeSpring(type);
    
    // Calculate buffer sizes based on parameters
    preDelayBufferSize = static_cast<int>(preDelay * sampleRate);
    reverbBufferSize = static_cast<int>(reverbTime * sampleRate * 2); // 2x for safety
    
    // Initialize delay buffers
    preDelayBuffer.resize(preDelayBufferSize, 0.0);
    reverbBuffer.resize(reverbBufferSize, 0.0);
    
    // Initialize all-pass filters
    allpassBuffers.resize(4, 0.0);  // 4 all-pass filters
    allpassDelays = {123, 265, 591, 1183};  // Typical delay values in samples
    allpassFeedbacks = {0.7, 0.7, 0.7, 0.7};  // Feedback values
    
    // Initialize delay indices
    preDelayWriteIndex = 0;
    preDelayReadIndex = (preDelayWriteIndex - preDelayBufferSize + 1) % preDelayBufferSize;
    if (preDelayReadIndex < 0) preDelayReadIndex += preDelayBufferSize;
    
    reverbWriteIndex = 0;
    reverbReadIndex = (reverbWriteIndex - reverbBufferSize/2 + 1) % reverbBufferSize;
    if (reverbReadIndex < 0) reverbReadIndex += reverbBufferSize;
}

void SpringReverb::initializeSpring(SpringType type) {
    switch (type) {
        case ACCUTRON_2A:
            reverbTime = 2.0;
            damping = 0.3;
            wetMix = 0.4;
            preDelay = 0.005;
            springLength = 0.4;
            springTension = 120.0;
            springMass = 0.008;
            break;
            
        case ACCUTRON_3A:
            reverbTime = 2.5;
            damping = 0.25;
            wetMix = 0.35;
            preDelay = 0.008;
            springLength = 0.5;
            springTension = 100.0;
            springMass = 0.01;
            break;
            
        case SPRAGALL_4AB2A:
            reverbTime = 1.8;
            damping = 0.4;
            wetMix = 0.3;
            preDelay = 0.003;
            springLength = 0.35;
            springTension = 150.0;
            springMass = 0.007;
            break;
            
        case MODERN_SPRING:
            reverbTime = 3.0;
            damping = 0.2;
            wetMix = 0.45;
            preDelay = 0.01;
            springLength = 0.6;
            springTension = 90.0;
            springMass = 0.012;
            break;
    }
}

bool bool SpringReverb::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool SpringReverb::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == inputPin && data_bytes == sizeof(double)) {
        memcpy(&inputSignal, data, sizeof(double));
        return true;
    }
    return false;
}

bool SpringReverb::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == outputPin && data_bytes == sizeof(double)) {
        memcpy(data, &outputSignal, sizeof(double));
        return true;
    }
    return false;
}

bool SpringReverb::Tick() {
    processSignal();
    return true;
}

void SpringReverb::processSignal() {
    // Apply input gain
    double signal = inputSignal * inputGain;
    
    // Apply pre-delay
    // Write to pre-delay buffer
    preDelayBuffer[preDelayWriteIndex] = signal;
    preDelayWriteIndex = (preDelayWriteIndex + 1) % preDelayBufferSize;
    
    // Read from pre-delay buffer
    double preDelayed = preDelayBuffer[preDelayReadIndex];
    preDelayReadIndex = (preDelayReadIndex + 1) % preDelayBufferSize;
    
    // Mix direct signal with pre-delayed signal
    signal = (1.0 - wetMix) * inputSignal * inputGain + wetMix * preDelayed;
    
    // Apply multiple all-pass filters to simulate the spring's complex reflections
    for (int i = 0; i < 4; i++) {
        signal = allpassFilter(i, signal);
    }
    
    // Apply the signal to the reverb buffer
    reverbBuffer[reverbWriteIndex] = signal;
    reverbWriteIndex = (reverbWriteIndex + 1) % reverbBufferSize;
    
    // Read from various points in the reverb buffer to simulate multiple reflections
    double feedback = 0.0;
    feedback += reverbBuffer[(reverbReadIndex + 500) % reverbBufferSize] * 0.6;
    feedback += reverbBuffer[(reverbReadIndex + 800) % reverbBufferSize] * 0.4;
    feedback += reverbBuffer[(reverbReadIndex + 1200) % reverbBufferSize] * 0.3;
    feedback += reverbBuffer[(reverbReadIndex + 1800) % reverbBufferSize] * 0.2;
    
    // Apply damping and decay
    feedback = applyDamping(feedback, 0);
    
    // Apply feedback to input for reverb decay
    signal += feedback * 0.7; // Feedback factor
    
    // The final output is a combination of the processed signal
    double reverbOut = reverbBuffer[reverbReadIndex];
    reverbReadIndex = (reverbReadIndex + 1) % reverbBufferSize;
    
    // Apply output gain and mix with dry signal
    outputSignal = (1.0 - wetMix) * inputSignal + wetMix * reverbOut * outputGain;
}

double SpringReverb::allpassFilter(int stage, double input) {
    if (stage >= static_cast<int>(allpassBuffers.size())) return input;
    
    int delay = allpassDelays[stage];
    double feedback = allpassFeedbacks[stage];
    double buf = allpassBuffers[stage];
    
    // Calculate read index
    int readIndex = (allpassIndices[stage] - delay + reverbBufferSize) % reverbBufferSize;
    
    // Process through all-pass filter
    double output = -input * feedback + buf;
    double feedbackSignal = input + buf * feedback;
    
    // Update buffer
    allpassBuffers[stage] = feedbackSignal;
    allpassIndices[stage] = (allpassIndices[stage] + 1) % reverbBufferSize;
    
    return output;
}

double SpringReverb::applyDamping(double signal, int delayIndex) {
    // Apply frequency-dependent damping
    static double dampingMemory = 0.0;
    signal = signal * (1.0 - damping) + dampingMemory * damping;
    dampingMemory = signal;
    return signal;
}

void SpringReverb::setReverbTime(double time) {
    reverbTime = std::max(0.1, std::min(10.0, time));
    // In a real implementation, this would adjust the feedback coefficients
}

void SpringReverb::setDamping(double damp) {
    damping = std::max(0.0, std::min(1.0, damp));
}


// PlateReverb implementation
PlateReverb::PlateReverb() {
    // Initialize filter parameters
    initializeFilters();
    
    // Initialize comb filters
    combBuffers.resize(NUM_COMBS);
    combBufferSizes.resize(NUM_COMBS);
    combFeedbacks.resize(NUM_COMBS);
    combIndices.resize(NUM_COMBS, 0);
    
    // Initialize with approximate values for a plate reverb
    std::vector<int> baseDelays = {1113, 1552, 1993, 2137, 3553, 3803, 4103, 4507}; // Prime numbers to avoid resonance
    for (int i = 0; i < NUM_COMBS; i++) {
        combBufferSizes[i] = static_cast<int>((baseDelays[i] / 44100.0) * sampleRate * plateSize);
        combBuffers[i].resize(combBufferSizes[i], 0.0);
        combFeedbacks[i] = 0.8 + 0.1 * brightness; // Higher brightness = more HF content
    }
    
    // Initialize all-pass filters
    allpassBuffers.resize(NUM_ALLPASSES);
    allpassBufferSizes.resize(NUM_ALLPASSES);
    allpassFeedbacks.resize(NUM_ALLPASSES, 0.7); // Standard all-pass feedback
    allpassIndices.resize(NUM_ALLPASSES, 0);
    
    std::vector<int> allpassDelays = {345, 556, 891, 1234};
    for (int i = 0; i < NUM_ALLPASSES; i++) {
        allpassBufferSizes[i] = static_cast<int>((allpassDelays[i] / 44100.0) * sampleRate * plateSize);
        allpassBuffers[i].resize(allpassBufferSizes[i], 0.0);
    }
}

void PlateReverb::initializeFilters() {
    // This function would set up the comb and all-pass filters based on reverb time, etc.
    // The actual implementation is in the constructor for simplicity
}

bool bool PlateReverb::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool PlateReverb::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == inputPin && data_bytes == sizeof(double)) {
        memcpy(&inputSignal, data, sizeof(double));
        return true;
    }
    return false;
}

bool PlateReverb::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == outputPin && data_bytes == sizeof(double)) {
        memcpy(data, &outputSignal, sizeof(double));
        return true;
    }
    return false;
}

bool PlateReverb::Tick() {
    processSignal();
    return true;
}

void PlateReverb::processSignal() {
    // Apply input gain
    double input = inputSignal * inputGain;
    
    // Feed input to all comb filters
    double combOut = 0.0;
    for (int i = 0; i < NUM_COMBS; i++) {
        // Write input to comb buffer
        combBuffers[i][combIndices[i]] = input + combBuffers[i][combIndices[i]] * combFeedbacks[i];
        
        // Calculate output as average of all comb filters
        int readIndex = (combIndices[i] - combBufferSizes[i]/2 + combBufferSizes[i]) % combBufferSizes[i];
        combOut += combBuffers[i][readIndex];
        
        // Update write index
        combIndices[i] = (combIndices[i] + 1) % combBufferSizes[i];
    }
    combOut /= NUM_COMBS;
    
    // Apply all-pass filters to diffuse the sound
    double allpassOut = combOut;
    for (int i = 0; i < NUM_ALLPASSES; i++) {
        // Read from allpass buffer
        int readIndex = (allpassIndices[i] - allpassBufferSizes[i]/4 + allpassBufferSizes[i]) % allpassBufferSizes[i];
        double buf = allpassBuffers[i][readIndex];
        
        // Process through all-pass filter
        double output = -allpassOut * allpassFeedbacks[i] + buf;
        double feedback = allpassOut + buf * allpassFeedbacks[i];
        
        // Write to buffer
        allpassBuffers[i][allpassIndices[i]] = feedback;
        allpassIndices[i] = (allpassIndices[i] + 1) % allpassBufferSizes[i];
        
        allpassOut = output;
    }
    
    // Apply high-frequency damping based on the damping parameter
    static double dampingMemory = 0.0;
    allpassOut = allpassOut * (1.0 - damping * 0.5) + dampingMemory * (damping * 0.5);
    dampingMemory = allpassOut;
    
    // Mix with dry signal
    outputSignal = (1.0 - wetMix) * inputSignal + wetMix * allpassOut * outputGain;
}

void PlateReverb::setReverbTime(double time) {
    reverbTime = std::max(0.1, std::min(10.0, time));
    
    // Adjust feedback coefficients based on reverb time
    // Longer reverb = higher feedback coefficients
    double feedbackFactor = std::min(0.99, 0.7 + 0.25 * (time / 5.0));
    for (auto& fb : combFeedbacks) {
        fb = feedbackFactor;
    }
}

void PlateReverb::setDamping(double damp) {
    damping = std::max(0.0, std::min(1.0, damp));
}

void PlateReverb::setSize(double size) {
    plateSize = std::max(0.1, std::min(2.0, size));
    // Would need to readjust buffer sizes in a real implementation
}

void PlateReverb::setBrightness(double bright) {
    brightness = std::max(0.0, std::min(1.0, bright));
    // Would adjust high-frequency content in a real implementation
}


// TubeReverbDriver implementation
TubeReverbDriver::TubeReverbDriver(const std::string& tubeType) : tubeType(tubeType) {
    // Set parameters based on tube type
    if (tubeType == "12AX7") {
        driverGain = 50.0;
        operatingBias = -1.5;
        outputZ = 600.0;
    } else if (tubeType == "ECC83") {
        driverGain = 45.0;
        operatingBias = -1.2;
        outputZ = 620.0;
    } else if (tubeType == "12AT7") {
        driverGain = 35.0;
        operatingBias = -1.0;
        outputZ = 1500.0;
    }
}

bool bool TubeReverbDriver::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeReverbDriver::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == inputPin && data_bytes == sizeof(double)) {
        memcpy(&inputSignal, data, sizeof(double));
        return true;
    } else if (conn_id == bPlusPin && data_bytes == sizeof(double)) {
        memcpy(&bPlusVoltage, data, sizeof(double));
        return true;
    }
    return false;
}

bool TubeReverbDriver::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == outputPin && data_bytes == sizeof(double)) {
        memcpy(data, &outputSignal, sizeof(double));
        return true;
    }
    return false;
}

bool TubeReverbDriver::Tick() {
    processSignal();
    return true;
}

void TubeReverbDriver::processSignal() {
    // Apply simple tube amplification with soft clipping
    double signal = inputSignal * driverGain;
    
    // Apply soft clipping to simulate tube saturation
    if (signal > 10.0) signal = 10.0 + 5.0 * tanh((signal - 10.0) / 5.0);
    if (signal < -10.0) signal = -10.0 + 5.0 * tanh((signal + 10.0) / 5.0);
    
    // Apply output impedance effect (slight attenuation)
    signal *= (outputZ / (outputZ + 1000.0));  // Assume 1k load
    
    outputSignal = signal;
}


// TubeReverbUnit implementation
TubeReverbUnit::TubeReverbUnit(ReverbConfiguration config) : config(config) {
    driver = std::make_unique<TubeReverbDriver>("12AX7");
    
    if (config == SPRING_REVERB) {
        springReverb = std::make_unique<SpringReverb>(SpringReverb::ACCUTRON_2A);
    } else if (config == PLATE_REVERB) {
        plateReverb = std::make_unique<PlateReverb>();
    }
}

bool bool TubeReverbUnit::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeReverbUnit::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == inputPin && data_bytes == sizeof(double)) {
        memcpy(&inputSignal, data, sizeof(double));
        return true;
    } else if (conn_id == bPlusPin && data_bytes == sizeof(double)) {
        memcpy(&bPlusVoltage, data, sizeof(double));
        return true;
    }
    return false;
}

bool TubeReverbUnit::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == outputPin && data_bytes == sizeof(double)) {
        memcpy(data, &outputSignal, sizeof(double));
        return true;
    }
    return false;
}

bool TubeReverbUnit::Tick() {
    processSignal();
    return true;
}

void TubeReverbUnit::processSignal() {
    // First, process the signal through the tube driver
    double drivenSignal = inputSignal;
    
    // For the purpose of this simulation, we'll just pass the signal value
    // and let the driver component process it separately
    
    if (config == SPRING_REVERB && springReverb) {
        // Process through spring reverb
        // This is a simplified approach - in reality, each component would be connected properly
        double tempInput = inputSignal;
        
        // Apply tube driver effect (simplified)
        if (tempInput > 0.1 || tempInput < -0.1) {
            tempInput *= 10.0;  // Amplification
            if (tempInput > 5.0) tempInput = 5.0 + 2.5 * tanh((tempInput - 5.0) / 2.5);
            if (tempInput < -5.0) tempInput = -5.0 + 2.5 * tanh((tempInput + 5.0) / 2.5);
        }
        
        // Set input to spring reverb and get output
        // In a real implementation, components would be connected with proper signals
        outputSignal = tempInput * 0.7;  // Simplified output
    } else if (config == PLATE_REVERB && plateReverb) {
        // Process through plate reverb (similar simplified approach)
        double tempInput = inputSignal;
        
        // Apply tube driver effect
        if (tempInput > 0.1 || tempInput < -0.1) {
            tempInput *= 10.0;
            if (tempInput > 5.0) tempInput = 5.0 + 2.5 * tanh((tempInput - 5.0) / 2.5);
            if (tempInput < -5.0) tempInput = -5.0 + 2.5 * tanh((tempInput + 5.0) / 2.5);
        }
        
        outputSignal = tempInput * 0.6;  // Simplified output
    }
}

void TubeReverbUnit::setReverbTime(double time) {
    if (springReverb) springReverb->setReverbTime(time);
    if (plateReverb) plateReverb->setReverbTime(time);
}

void TubeReverbUnit::setDamping(double damp) {
    if (springReverb) springReverb->setDamping(damp);
    if (plateReverb) plateReverb->setDamping(damp);
}

void TubeReverbUnit::setMix(double mix) {
    if (springReverb) springReverb->setMix(mix);
    if (plateReverb) plateReverb->setMix(mix);
}

void TubeReverbUnit::setPreDelay(double delay) {
    if (springReverb) springReverb->setPreDelay(delay);
}

void TubeReverbUnit::setConfiguration(ReverbConfiguration conf) {
    config = conf;
    
    if (config == SPRING_REVERB && !springReverb) {
        springReverb = std::make_unique<SpringReverb>(SpringReverb::ACCUTRON_2A);
    } else if (config == PLATE_REVERB && !plateReverb) {
        plateReverb = std::make_unique<PlateReverb>();
    }
}