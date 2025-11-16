#include "TubeEffectCircuits.h"
#include <cmath>
#include <algorithm>
#include <numeric>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// TubeCompressor implementation
TubeCompressor::TubeCompressor(CompressionType type) : compressionType(type) {
    initializeCompressor(type);
    
    // Calculate coefficients for attack and release
    attackCoeff = exp(-1.0 / (attackTime * sampleRate));
    releaseCoeff = exp(-1.0 / (releaseTime * sampleRate));
    
    tubeCompressionFactor = 0.5; // Default tube contribution
}

void TubeCompressor::initializeCompressor(CompressionType type) {
    switch (type) {
        case CLASS_A_FETISH:
            threshold = -10.0;
            ratio = 3.0;
            attackTime = 0.005;   // 5ms
            releaseTime = 0.2;    // 200ms
            tubeGain = 15.0;
            break;
            
        case TRIODE_LIMITER:
            threshold = -1.0;     // Soft limiting
            ratio = 10.0;         // Near hard limiting
            attackTime = 0.001;   // 1ms (fast)
            releaseTime = 0.05;   // 50ms
            tubeGain = 25.0;
            break;
            
        case PENTODE_COMPRESSOR:
            threshold = -12.0;
            ratio = 4.0;
            attackTime = 0.003;   // 3ms
            releaseTime = 0.15;   // 150ms
            tubeGain = 30.0;
            break;
            
        case VAR_MU_LIMITER:
            threshold = -2.0;
            ratio = 8.0;
            attackTime = 0.002;   // 2ms
            releaseTime = 0.1;    // 100ms
            tubeGain = 20.0;
            break;
    }
}

bool TubeCompressor::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeCompressor::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == inputPin && data_bytes == sizeof(double)) {
        memcpy(&inputSignal, data, sizeof(double));
        return true;
    } else if (conn_id == controlPin && data_bytes == sizeof(double)) {
        memcpy(&controlSignal, data, sizeof(double));
        // Control signal can affect compression parameters
        setRatio(2.0 + 8.0 * (controlSignal + 1.0) / 2.0);  // Map -1,1 to ratio 2-10
        return true;
    } else if (conn_id == sidechainPin && data_bytes == sizeof(double)) {
        memcpy(&sidechainSignal, data, sizeof(double));
        return true;
    }
    return false;
}

bool TubeCompressor::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == outputPin && data_bytes == sizeof(double)) {
        memcpy(data, &outputSignal, sizeof(double));
        return true;
    }
    return false;
}

bool TubeCompressor::Tick() {
    processSignal();
    return true;
}

double TubeCompressor::calculateCompressionGain(double inputLevel) {
    // Convert to dB
    double inputLevelDB = 20.0 * log10(std::abs(inputLevel) + 1e-9);
    
    if (softKneeEnabled) {
        // Soft knee compression
        double softThreshold = threshold - kneeWidth / 2.0;
        double softRange = kneeWidth;
        
        if (inputLevelDB < softThreshold) {
            // No compression
            return 1.0;
        } else if (inputLevelDB < softThreshold + softRange) {
            // Soft knee transition
            double x = (inputLevelDB - softThreshold) / softRange;
            double gainReductionDB = (1.0 - x) * 0.0 + x * (inputLevelDB - threshold) * (1.0 - 1.0/ratio);
            return pow(10.0, -gainReductionDB / 20.0);
        } else {
            // Hard compression
            double gainReductionDB = (inputLevelDB - threshold) * (1.0 - 1.0/ratio);
            return pow(10.0, -gainReductionDB / 20.0);
        }
    } else {
        // Hard knee compression
        if (inputLevelDB < threshold) {
            return 1.0;
        } else {
            double gainReductionDB = (inputLevelDB - threshold) * (1.0 - 1.0/ratio);
            return pow(10.0, -gainReductionDB / 20.0);
        }
    }
}

void TubeCompressor::updateDetector() {
    // Simple peak detector for sidechain
    double inputLevel = std::abs(inputSignal);
    if (sidechainSignal != 0.0) {
        inputLevel = std::abs(sidechainSignal);  // Use external sidechain if provided
    }
    
    // Apply sidechain filtering if enabled
    if (sidechainFilterEnabled) {
        // Simple low-pass filter to simulate tube rectifier response
        static double filteredLevel = 0.0;
        filteredLevel = 0.7 * filteredLevel + 0.3 * inputLevel;
        inputLevel = filteredLevel;
    }
    
    // Update detector level with attack/release
    if (inputLevel > detectorLevel) {
        // Attack - follow input quickly
        detectorLevel = inputLevel * (1.0 - attackCoeff) + detectorLevel * attackCoeff;
    } else {
        // Release - fall back slowly
        detectorLevel = detectorLevel * releaseCoeff;
    }
}

void TubeCompressor::processSignal() {
    // Update the detector
    updateDetector();
    
    // Calculate compression gain based on detected level
    double newGain = calculateCompressionGain(detectorLevel);
    
    // Apply tube characteristics to compression
    // Tubes have a soft, musical compression characteristic
    double tubeFactor = 1.0 - tubeCompressionFactor * (1.0 - newGain);
    
    // Smooth transition between current and new gain
    compressorGain = compressorGain * 0.95 + newGain * 0.05;
    
    // Apply compression to input
    double compressedSignal = inputSignal * compressorGain * tubeFactor;
    
    // Apply makeup gain
    double makeupMultiplier = pow(10.0, makeupGain / 20.0);
    outputSignal = compressedSignal * makeupMultiplier;
    
    // Apply soft limiting to prevent clipping
    if (outputSignal > 0.9) outputSignal = 0.9 + 0.1 * tanh((outputSignal - 0.9) / 0.1);
    if (outputSignal < -0.9) outputSignal = -0.9 + 0.1 * tanh((outputSignal + 0.9) / 0.1);
    
    // Auto makeup gain if enabled
    if (autoMakeupEnabled) {
        // This would adjust makeup gain based on average compression
        // Simplified implementation
    }
}

void TubeCompressor::setThreshold(double threshold) {
    this->threshold = std::max(-60.0, std::min(0.0, threshold));
}

void TubeCompressor::setRatio(double ratio) {
    this->ratio = std::max(1.0, std::min(20.0, ratio));
}

void TubeCompressor::setAttackTime(double time) {
    this->attackTime = std::max(0.0001, std::min(0.5, time));
    attackCoeff = exp(-1.0 / (attackTime * sampleRate));
}

void TubeCompressor::setReleaseTime(double time) {
    this->releaseTime = std::max(0.01, std::min(2.0, time));
    releaseCoeff = exp(-1.0 / (releaseTime * sampleRate));
}

void TubeCompressor::setMakeupGain(double gain) {
    this->makeupGain = std::max(-20.0, std::min(30.0, gain));
}


// TubePhaser implementation
TubePhaser::TubePhaser(PhaserType type, int stages) : phaserType(type), stageCount(stages) {
    initializePhaser(type, stages);
    
    // Initialize delay buffers for each stage
    delayBuffers.resize(stageCount);
    delayBufferSizes.resize(stageCount);
    writePositions.resize(stageCount, 0);
    allpassCoefficients.resize(stageCount);
    
    // Set delay values based on stage number (creating the characteristic phaser sweep)
    for (int i = 0; i < stageCount; i++) {
        // Create exponentially increasing delay values
        double delayTime = 0.0005 * pow(1.5, i); // Start with 0.5ms and increase
        delayBufferSizes[i] = static_cast<int>(delayTime * sampleRate);
        delayBuffers[i].resize(delayBufferSizes[i], 0.0);
        
        // Initialize allpass coefficients to create the phaser effect
        allpassCoefficients[i] = 0.6; // Base coefficient
    }
}

void TubePhaser::initializePhaser(PhaserType type, int stages) {
    switch (type) {
        case CLASSIC_4_STAGE:
            stageCount = 4;
            lfoFrequency = 0.5;
            depth = 0.7;
            feedback = 0.3;
            notchCount = 4;
            break;
            
        case MODERN_6_STAGE:
            stageCount = 6;
            lfoFrequency = 0.7;
            depth = 0.6;
            feedback = 0.4;
            notchCount = 6;
            break;
            
        case TUBE_TRIODE_PHAZE:
            stageCount = 3;
            lfoFrequency = 0.4;
            depth = 0.8;
            feedback = 0.2;
            notchCount = 3;
            break;
            
        case VINTAGE_ANALOG:
            stageCount = 8;
            lfoFrequency = 0.3;
            depth = 0.75;
            feedback = 0.1;
            notchCount = 8;
            break;
    }
    
    lfoPhase = 0.0;
    lfoDepth = depth;
}

bool TubePhaser::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubePhaser::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == inputPin && data_bytes == sizeof(double)) {
        memcpy(&inputSignal, data, sizeof(double));
        return true;
    } else if (conn_id == ratePin && data_bytes == sizeof(double)) {
        memcpy(&rateControl, data, sizeof(double));
        setLFOFrequency(lfoFrequency * (1.0 + rateControl * 0.5)); // ±50% rate control
        return true;
    } else if (conn_id == depthPin && data_bytes == sizeof(double)) {
        memcpy(&depthControl, data, sizeof(double));
        setDepth(depth * (1.0 + depthControl * 0.5)); // ±50% depth control
        return true;
    }
    return false;
}

bool TubePhaser::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == outputPin && data_bytes == sizeof(double)) {
        memcpy(data, &outputSignal, sizeof(double));
        return true;
    }
    return false;
}

bool TubePhaser::Tick() {
    processSignal();
    return true;
}

double TubePhaser::processAllpassStage(int stage, double input, double coeff) {
    int readIndex = (writePositions[stage] - delayBufferSizes[stage]/2 + delayBufferSizes[stage]) % delayBufferSizes[stage];
    double delayed = delayBuffers[stage][readIndex];
    
    // Apply all-pass filter: output = -a*input + delayed
    double output = -coeff * input + delayed;
    
    // Feedback: input = input + feedback_coeff * delayed
    double feedbackInput = input + feedback * feedbackBuffer;
    
    // Store the original input in the delay buffer
    delayBuffers[stage][writePositions[stage]] = feedbackInput;
    writePositions[stage] = (writePositions[stage] + 1) % delayBufferSizes[stage];
    
    // Update feedback buffer
    feedbackBuffer = output;
    
    return output;
}

void TubePhaser::updateLFO() {
    lfoPhase += 2.0 * M_PI * lfoFrequency / sampleRate;
    if (lfoPhase >= 2.0 * M_PI) {
        lfoPhase -= 2.0 * M_PI;
    }
    
    // Update allpass coefficients based on LFO
    for (int i = 0; i < stageCount; i++) {
        // Create a modulation value based on LFO and stage
        double stagePhase = lfoPhase + i * M_PI / stageCount * 2;
        
        // Different LFO waveform types
        double modulation;
        switch (modulationType) {
            case 0: // Sine
                modulation = sin(stagePhase);
                break;
            case 1: // Triangle
                modulation = 2.0/M_PI * asin(sin(stagePhase));
                break;
            case 2: // Square
                modulation = (stagePhase < M_PI) ? 1.0 : -1.0;
                break;
            default:
                modulation = sin(stagePhase);
        }
        
        // Apply depth and base coefficient
        allpassCoefficients[i] = 0.6 + 0.3 * depth * modulation;
        // Ensure coefficient is within safe range
        allpassCoefficients[i] = std::max(0.1, std::min(0.9, allpassCoefficients[i]));
    }
}

void TubePhaser::processSignal() {
    updateLFO();
    
    // Add feedback to input
    double signal = inputSignal + feedback * feedbackBuffer;
    
    // Process through all stages
    for (int i = 0; i < stageCount; i++) {
        signal = processAllpassStage(i, signal, allpassCoefficients[i]);
    }
    
    // Mix dry and wet signals
    outputSignal = 0.6 * inputSignal + 0.4 * signal;
    
    // Apply tube saturation characteristics
    if (outputSignal > 0.8) outputSignal = 0.8 + 0.2 * tanh((outputSignal - 0.8) / 0.2);
    if (outputSignal < -0.8) outputSignal = -0.8 + 0.2 * tanh((outputSignal + 0.8) / 0.2);
}

void TubePhaser::setLFOFrequency(double freq) {
    lfoFrequency = std::max(0.01, std::min(10.0, freq));
}

void TubePhaser::setDepth(double depth) {
    this->depth = std::max(0.0, std::min(1.0, depth));
}

void TubePhaser::setFeedback(double feedback) {
    this->feedback = std::max(-0.9, std::min(0.9, feedback));
}


// TubeChorus implementation
TubeChorus::TubeChorus(int voices) : voiceCount(voices) {
    initializeChorus(voices);
    
    // Initialize delay buffers for each voice
    delayBuffers.resize(voiceCount);
    bufferSizes.resize(voiceCount);
    writePositions.resize(voiceCount, 0);
    lfoPhases.resize(voiceCount, 0.0);
    lfoOffsets.resize(voiceCount, 0.0);
    
    // Set base delay and create phase offsets for chorus effect
    for (int i = 0; i < voiceCount; i++) {
        double voiceDelay = baseDelayTime + (i * separation / 1000.0); // Convert ms to seconds
        bufferSizes[i] = static_cast<int>(voiceDelay * sampleRate * 2); // 2x for modulation headroom
        delayBuffers[i].resize(bufferSizes[i], 0.0);
        
        // Create phase offsets so voices are out of phase
        lfoOffsets[i] = (2.0 * M_PI * i) / voiceCount;
    }
}

void TubeChorus::initializeChorus(int voices) {
    voiceCount = std::max(1, std::min(8, voices));
    lfoFrequency = 1.0;
    depth = 0.3;
    baseDelayTime = 0.012;  // 12ms
    feedback = 0.0;
    separation = 2.0;       // 2ms separation between voices
}

bool TubeChorus::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeChorus::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == inputPin && data_bytes == sizeof(double)) {
        memcpy(&inputSignal, data, sizeof(double));
        return true;
    } else if (conn_id == ratePin && data_bytes == sizeof(double)) {
        memcpy(&inputSignal, data, sizeof(double));
        setLFOFrequency(lfoFrequency * (1.0 + inputSignal * 0.5)); // ±50% rate control
        return true;
    } else if (conn_id == depthPin && data_bytes == sizeof(double)) {
        memcpy(&inputSignal, data, sizeof(double));
        setDepth(depth * (1.0 + inputSignal * 0.5)); // ±50% depth control
        return true;
    }
    return false;
}

bool TubeChorus::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == outputPin && data_bytes == sizeof(double)) {
        memcpy(data, &outputSignal, sizeof(double));
        return true;
    }
    return false;
}

bool TubeChorus::Tick() {
    processSignal();
    return true;
}

double TubeChorus::processDelayLine(int voice, double input) {
    // Calculate modulated delay time
    double modulation = depth * sin(lfoPhases[voice]);
    double modulatedDelayTime = baseDelayTime * (1.0 + modulation);
    int modulatedDelaySamples = static_cast<int>(modulatedDelayTime * sampleRate);
    
    // Ensure delay is within buffer bounds
    modulatedDelaySamples = std::max(1, std::min(bufferSizes[voice] - 1, modulatedDelaySamples));
    
    // Calculate read position
    int readPosition = (writePositions[voice] - modulatedDelaySamples + bufferSizes[voice]) % bufferSizes[voice];
    if (readPosition < 0) readPosition += bufferSizes[voice];
    
    // Read from delay line
    double delayed = delayBuffers[voice][readPosition];
    
    // Write to delay line with feedback
    double writeValue = input + feedback * delayed;
    delayBuffers[voice][writePositions[voice]] = writeValue;
    writePositions[voice] = (writePositions[voice] + 1) % bufferSizes[voice];
    
    return delayed;
}

void TubeChorus::updateLFO() {
    for (int i = 0; i < voiceCount; i++) {
        lfoPhases[i] += 2.0 * M_PI * lfoFrequency / sampleRate;
        if (lfoPhases[i] >= 2.0 * M_PI) {
            lfoPhases[i] -= 2.0 * M_PI;
        }
    }
}

void TubeChorus::processSignal() {
    updateLFO();
    
    // Process each voice and mix the results
    double totalOutput = 0.0;
    for (int i = 0; i < voiceCount; i++) {
        double voiceOutput = processDelayLine(i, inputSignal);
        totalOutput += voiceOutput / voiceCount;  // Average the voices
    }
    
    // Mix with dry signal
    outputSignal = 0.6 * inputSignal + 0.4 * totalOutput;
    
    // Apply tube saturation characteristics
    if (outputSignal > 0.85) outputSignal = 0.85 + 0.15 * tanh((outputSignal - 0.85) / 0.15);
    if (outputSignal < -0.85) outputSignal = -0.85 + 0.15 * tanh((outputSignal + 0.85) / 0.15);
}

void TubeChorus::setLFOFrequency(double freq) {
    lfoFrequency = std::max(0.1, std::min(10.0, freq));
}

void TubeChorus::setDepth(double depth) {
    this->depth = std::max(0.0, std::min(1.0, depth));
}

void TubeChorus::setDelayTime(double time) {
    baseDelayTime = std::max(0.001, std::min(0.1, time)); // Limit to 1ms to 100ms
}


// TubeFlanger implementation
TubeFlanger::TubeFlanger() {
    initializeFlanger();
    
    // Initialize delay buffer
    bufferSize = static_cast<int>(0.02 * sampleRate); // 20ms buffer
    delayBuffer.resize(bufferSize, 0.0);
}

void TubeFlanger::initializeFlanger() {
    lfoFrequency = 0.25;
    depth = 0.6;
    feedback = 0.5;
    baseDelay = 0.001;  // 1ms
    manualSetting = 0.5;
    lfoPhase = 0.0;
    writePosition = 0;
}

bool TubeFlanger::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeFlanger::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == inputPin && data_bytes == sizeof(double)) {
        memcpy(&inputSignal, data, sizeof(double));
        return true;
    } else if (conn_id == ratePin && data_bytes == sizeof(double)) {
        memcpy(&inputSignal, data, sizeof(double));
        setLFOFrequency(lfoFrequency * (1.0 + inputSignal * 0.5)); // ±50% rate control
        return true;
    } else if (conn_id == depthPin && data_bytes == sizeof(double)) {
        memcpy(&inputSignal, data, sizeof(double));
        setDepth(depth * (1.0 + inputSignal * 0.5)); // ±50% depth control
        return true;
    } else if (conn_id == feedbackPin && data_bytes == sizeof(double)) {
        memcpy(&inputSignal, data, sizeof(double));
        setFeedback(feedback * (1.0 + inputSignal * 0.8)); // ±80% feedback control
        return true;
    }
    return false;
}

bool TubeFlanger::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == outputPin && data_bytes == sizeof(double)) {
        memcpy(data, &outputSignal, sizeof(double));
        return true;
    }
    return false;
}

bool TubeFlanger::Tick() {
    processSignal();
    return true;
}

double TubeFlanger::processDelayLine(double input) {
    // Calculate modulated delay time
    double modulation = depth * sin(lfoPhase);
    double totalDelay = baseDelay + manualSetting * 0.01 + 0.005 * modulation; // 0-10ms modulation
    
    // Convert to samples
    int delaySamples = static_cast<int>(totalDelay * sampleRate);
    
    // Ensure delay is within bounds
    delaySamples = std::max(1, std::min(bufferSize - 1, delaySamples));
    
    // Calculate read position
    int readPosition = (writePosition - delaySamples + bufferSize) % bufferSize;
    if (readPosition < 0) readPosition += bufferSize;
    
    // Read from delay line
    double delayed = delayBuffer[readPosition];
    
    // Write to delay line with feedback
    double feedbackSample = input + feedback * delayed;
    delayBuffer[writePosition] = feedbackSample;
    writePosition = (writePosition + 1) % bufferSize;
    
    return delayed;
}

void TubeFlanger::updateLFO() {
    lfoPhase += 2.0 * M_PI * lfoFrequency / sampleRate;
    if (lfoPhase >= 2.0 * M_PI) {
        lfoPhase -= 2.0 * M_PI;
    }
}

void TubeFlanger::processSignal() {
    updateLFO();
    
    // Process through delay line
    double delayedSignal = processDelayLine(inputSignal);
    
    // Mix with input (flanging effect)
    outputSignal = 0.7 * inputSignal + 0.3 * delayedSignal;
    
    // Apply tube saturation characteristics
    if (outputSignal > 0.8) outputSignal = 0.8 + 0.2 * tanh((outputSignal - 0.8) / 0.2);
    if (outputSignal < -0.8) outputSignal = -0.8 + 0.2 * tanh((outputSignal + 0.8) / 0.2);
}

void TubeFlanger::setLFOFrequency(double freq) {
    lfoFrequency = std::max(0.05, std::min(5.0, freq));
}

void TubeFlanger::setDepth(double depth) {
    this->depth = std::max(0.0, std::min(1.0, depth));
}

void TubeFlanger::setFeedback(double feedback) {
    this->feedback = std::max(-0.9, std::min(0.9, feedback));
}

void TubeFlanger::setBaseDelay(double delay) {
    baseDelay = std::max(0.0001, std::min(0.01, delay)); // 0.1ms to 10ms
}

void TubeFlanger::setManual(double manual) {
    manualSetting = std::max(0.0, std::min(1.0, manual));
}