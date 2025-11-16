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


// TubeExpander implementation
TubeExpander::TubeExpander(ExpanderType type) : expanderType(type) {
    initializeExpander(type);

    // Calculate coefficients for attack and release
    attackCoeff = exp(-1.0 / (attackTime * sampleRate));
    releaseCoeff = exp(-1.0 / (releaseTime * sampleRate));

    tubeExpansionFactor = 0.5; // Default tube contribution
}

void TubeExpander::initializeExpander(ExpanderType type) {
    switch (type) {
        case GATE:
            threshold = -30.0;    // Gate threshold
            ratio = 3.0;          // Expansion ratio
            attackTime = 0.003;   // 3ms (faster attack)
            releaseTime = 0.15;   // 150ms (slower release)
            tubeGain = 15.0;
            range = -24.0;        // Max gain reduction
            break;

        case BAND_GATE:
            threshold = -25.0;    // Multiband gate threshold
            ratio = 4.0;          // Expansion ratio
            attackTime = 0.005;   // 5ms
            releaseTime = 0.2;    // 200ms
            tubeGain = 18.0;
            range = -30.0;        // Max gain reduction
            break;

        case DOWNWARD_EXPANDER:
            threshold = -20.0;    // Expansion threshold
            ratio = 2.0;          // Expansion ratio
            attackTime = 0.008;   // 8ms
            releaseTime = 0.1;    // 100ms
            tubeGain = 20.0;
            range = -12.0;        // Max gain reduction
            break;

        case UPWARD_EXPANDER:
            threshold = -40.0;    // Lower threshold for upward expansion
            ratio = 0.5;          // Values < 1 for upward expansion
            attackTime = 0.002;   // 2ms (very fast)
            releaseTime = 0.08;   // 80ms
            tubeGain = 25.0;
            range = -6.0;         // Max gain reduction (upward expansion is limited)
            break;
    }
}

bool TubeExpander::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeExpander::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == inputPin && data_bytes == sizeof(double)) {
        memcpy(&inputSignal, data, sizeof(double));
        return true;
    } else if (conn_id == controlPin && data_bytes == sizeof(double)) {
        memcpy(&controlSignal, data, sizeof(double));
        // Control signal can affect expansion parameters
        setRatio(1.0 + 4.0 * (controlSignal + 1.0) / 2.0);  // Map -1,1 to ratio 1-5
        return true;
    } else if (conn_id == sidechainPin && data_bytes == sizeof(double)) {
        memcpy(&sidechainSignal, data, sizeof(double));
        return true;
    }
    return false;
}

bool TubeExpander::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == outputPin && data_bytes == sizeof(double)) {
        memcpy(data, &outputSignal, sizeof(double));
        return true;
    }
    return false;
}

bool TubeExpander::Tick() {
    processSignal();
    return true;
}

double TubeExpander::calculateExpansionGain(double inputLevel) {
    // Convert to dB
    double inputLevelDB = 20.0 * log10(std::abs(inputLevel) + 1e-9);

    if (softKneeEnabled) {
        // Soft knee expansion (opposite of compression)
        double softThreshold = threshold + kneeWidth / 2.0;
        double softRange = kneeWidth;

        if (inputLevelDB > softThreshold) {
            // No expansion (or slight compression above threshold)
            return 1.0;
        } else if (inputLevelDB > softThreshold - softRange) {
            // Soft knee transition
            double x = (softThreshold - inputLevelDB) / softRange;
            // Expansion below threshold
            double gainReductionDB = (1.0 - x) * 0.0 + x * (threshold - inputLevelDB) * (1.0 - 1.0/ratio);
            // Apply range limiting
            gainReductionDB = std::min(gainReductionDB, -range);
            return pow(10.0, -gainReductionDB / 20.0);
        } else {
            // Full expansion
            double gainReductionDB = (threshold - inputLevelDB) * (1.0 - 1.0/ratio);
            // Apply range limiting
            gainReductionDB = std::min(gainReductionDB, -range);
            return pow(10.0, -gainReductionDB / 20.0);
        }
    } else {
        // Hard knee expansion
        if (inputLevelDB > threshold) {
            // No expansion above threshold
            return 1.0;
        } else {
            // Apply expansion below threshold
            double gainReductionDB = (threshold - inputLevelDB) * (1.0 - 1.0/ratio);
            // Apply range limiting
            gainReductionDB = std::min(gainReductionDB, -range);
            return pow(10.0, -gainReductionDB / 20.0);
        }
    }
}

void TubeExpander::updateDetector() {
    // Simple peak detector for sidechain
    double inputLevel = std::abs(inputSignal);
    if (sidechainSignal != 0.0) {
        inputLevel = std::abs(sidechainSignal);  // Use external sidechain if provided
    }

    // Apply sidechain filtering if enabled
    if (true) { // Always apply filtering for expanders
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

void TubeExpander::processSignal() {
    // Update the detector
    updateDetector();

    // Calculate expansion gain based on detected level
    double newGain = calculateExpansionGain(detectorLevel);

    // Apply tube characteristics to expansion
    // Tubes can add harmonic content that's useful in expanders
    double tubeFactor = 1.0 - tubeExpansionFactor * (1.0 - newGain);

    // Smooth transition between current and new gain
    expanderGain = expanderGain * 0.95 + newGain * 0.05;

    // Apply expansion to input
    double expandedSignal = inputSignal * expanderGain * tubeFactor;

    // Apply makeup gain
    double makeupMultiplier = pow(10.0, makeupGain / 20.0);
    outputSignal = expandedSignal * makeupMultiplier;

    // Apply soft limiting to prevent clipping
    if (outputSignal > 0.9) outputSignal = 0.9 + 0.1 * tanh((outputSignal - 0.9) / 0.1);
    if (outputSignal < -0.9) outputSignal = -0.9 + 0.1 * tanh((outputSignal + 0.9) / 0.1);

    // Auto makeup gain if enabled
    if (autoMakeupEnabled) {
        // This would adjust makeup gain based on average expansion
        // Simplified implementation
    }
}

void TubeExpander::setThreshold(double threshold) {
    this->threshold = std::max(-80.0, std::min(0.0, threshold));
}

void TubeExpander::setRatio(double ratio) {
    // For expanders, ratio > 1 expands (reduces gain) below threshold
    // For upward expanders, ratio < 1 expands (increases gain) below threshold
    this->ratio = std::max(0.1, std::min(20.0, ratio));
}

void TubeExpander::setAttackTime(double time) {
    this->attackTime = std::max(0.0001, std::min(0.5, time));
    attackCoeff = exp(-1.0 / (attackTime * sampleRate));
}

void TubeExpander::setReleaseTime(double time) {
    this->releaseTime = std::max(0.01, std::min(2.0, time));
    releaseCoeff = exp(-1.0 / (releaseTime * sampleRate));
}

void TubeExpander::setMakeupGain(double gain) {
    this->makeupGain = std::max(-20.0, std::min(30.0, gain));
}

void TubeExpander::setRange(double range) {
    this->range = std::max(-60.0, std::min(0.0, range));
}


// TubeMaximizer implementation
TubeMaximizer::TubeMaximizer(MaximizerType type) : maximizerType(type) {
    initializeMaximizer(type);

    // Calculate coefficients for attack and release
    attackCoeff = exp(-1.0 / (attackTime * sampleRate));
    releaseCoeff = exp(-1.0 / (releaseTime * sampleRate));

    tubeMaximizationFactor = 0.3; // Default tube contribution
    
    // Initialize look-ahead buffer
    int delaySamples = static_cast<int>(lookAheadTime * sampleRate);
    delayBufferSize = delaySamples + 10;  // Add some extra space
    delayBuffer.resize(delayBufferSize, 0.0);
    delayWritePosition = 0;
}

void TubeMaximizer::initializeMaximizer(MaximizerType type) {
    switch (type) {
        case PEEK_MAXIMIZER:
            ceiling = -0.1;         // -0.1dB ceiling
            attackTime = 0.0001;    // Very fast attack (0.1ms)
            releaseTime = 0.05;     // 50ms release
            tubeGain = 25.0;
            harmonicContent = 0.05; // Low harmonic content
            break;

        case RMS_MAXIMIZER:
            ceiling = -0.5;         // -0.5dB ceiling
            attackTime = 0.002;     // 2ms attack (slower than peek)
            releaseTime = 0.1;      // 100ms release (adaptive)
            tubeGain = 22.0;
            harmonicContent = 0.1;  // Medium harmonic content
            adaptiveRelease = true;
            break;

        case INTEGRAL_MAXIMIZER:
            ceiling = -0.2;         // -0.2dB ceiling
            attackTime = 0.0005;    // 0.5ms attack
            releaseTime = 0.08;     // 80ms release with integral control
            tubeGain = 24.0;
            harmonicContent = 0.08; // Medium harmonic content
            gainRecoveryEnabled = true;
            gainRecoverySpeed = 0.998; // Slower recovery
            break;

        case DUAL_STAGE_MAXIMIZER:
            ceiling = -0.3;         // -0.3dB ceiling
            attackTime = 0.0002;    // Very fast attack (0.2ms)
            releaseTime = 0.15;     // 150ms release (adaptive)
            tubeGain = 26.0;
            harmonicContent = 0.15; // Higher harmonic content
            softClippingEnabled = true;
            break;
    }
}

bool TubeMaximizer::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeMaximizer::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == inputPin && data_bytes == sizeof(double)) {
        memcpy(&inputSignal, data, sizeof(double));
        // Add to delay buffer for look-ahead
        delayBuffer[delayWritePosition] = inputSignal;
        delayWritePosition = (delayWritePosition + 1) % delayBufferSize;
        return true;
    } else if (conn_id == controlPin && data_bytes == sizeof(double)) {
        memcpy(&controlSignal, data, sizeof(double));
        // Control signal can affect maximization parameters
        setCeiling(-0.5 + 0.4 * (controlSignal + 1.0));  // Map -1,1 to ceiling -0.9 to -0.1
        return true;
    } else if (conn_id == sidechainPin && data_bytes == sizeof(double)) {
        memcpy(&sidechainSignal, data, sizeof(double));
        return true;
    }
    return false;
}

bool TubeMaximizer::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == outputPin && data_bytes == sizeof(double)) {
        memcpy(data, &outputSignal, sizeof(double));
        return true;
    }
    return false;
}

bool TubeMaximizer::Tick() {
    processSignal();
    return true;
}

double TubeMaximizer::calculateLimitingGain(double inputLevel) {
    // Convert to dB
    double inputLevelDB = 20.0 * log10(std::abs(inputLevel) + 1e-9);
    double ceilingDB = ceiling;

    // If input is above ceiling, apply limiting
    if (inputLevelDB > ceilingDB) {
        double gainReductionDB = inputLevelDB - ceilingDB;
        double gainReduction = pow(10.0, -gainReductionDB / 20.0);
        return gainReduction;
    }
    
    // Otherwise, return 1.0 (no limiting) but consider adaptive release
    if (adaptiveRelease) {
        // Adaptive release - faster release when signal is much lower than ceiling
        double levelBelowCeiling = ceilingDB - inputLevelDB;
        double adaptiveCoeff = exp(-1.0 / (releaseTime * sampleRate * (1.0 + levelBelowCeiling/10.0)));
        // Gradually recover toward 1.0
        return currentGain * adaptiveCoeff + 1.0 * (1.0 - adaptiveCoeff);
    }
    
    return 1.0;
}

void TubeMaximizer::updateGainRecovery() {
    if (gainRecoveryEnabled) {
        // Slowly recover to unity gain when signal is low
        gainRecoveryFactor = gainRecoveryFactor * gainRecoverySpeed + 1.0 * (1.0 - gainRecoverySpeed);
        // Clamp to ensure it doesn't go below allowed minimum
        gainRecoveryFactor = std::max(0.1, gainRecoveryFactor);
    }
}

double TubeMaximizer::getLookAheadSignal() {
    // Calculate read position for look-ahead
    int delaySamples = static_cast<int>(lookAheadTime * sampleRate);
    int readPos = (delayWritePosition - delaySamples + delayBufferSize) % delayBufferSize;
    return delayBuffer[readPos];
}

void TubeMaximizer::processSignal() {
    // Use look-ahead signal for detection
    double lookaheadSignal = getLookAheadSignal();
    double detectionLevel = std::abs(lookaheadSignal);
    
    // If sidechain is provided, use that for detection instead
    if (sidechainSignal != 0.0) {
        detectionLevel = std::abs(sidechainSignal);
    }
    
    // Calculate limiting gain based on detected level
    double newGain = calculateLimitingGain(detectionLevel);

    // Apply adaptive behavior if enabled
    if (adaptiveRelease) {
        // Update current gain with adaptive release
        if (newGain < currentGain) {
            // Attack (gain reduction) - respond quickly
            currentGain = newGain; // Immediate response for attack
        } else {
            // Release - adapt based on signal level
            double adaptiveCoeff = exp(-1.0 / (releaseTime * sampleRate * (1.0 + (ceiling - 20*log10(detectionLevel + 1e-9))/5.0)));
            currentGain = currentGain * adaptiveCoeff + newGain * (1.0 - adaptiveCoeff);
        }
    } else {
        // Simple attack/release behavior
        if (newGain < currentGain) {
            // Attacking (gain reduction)
            currentGain = currentGain * attackCoeff + newGain * (1.0 - attackCoeff);
        } else {
            // Releasing (gain increase)
            currentGain = currentGain * releaseCoeff + newGain * (1.0 - releaseCoeff);
        }
    }
    
    // Update gain recovery if enabled
    updateGainRecovery();

    // Apply tube characteristics to maximization
    double tubeFactor = 1.0 - tubeMaximizationFactor * (1.0 - currentGain);

    // Apply maximization to input signal (use the real input signal, not lookahead)
    double processedSignal = inputSignal * currentGain * tubeFactor;

    // Apply soft clipping if enabled
    if (softClippingEnabled) {
        // Apply soft clipping with harmonic content
        double clipThresh = pow(10.0, ceiling / 20.0) * 0.8; // 80% of ceiling for soft knee
        if (processedSignal > clipThresh) {
            processedSignal = clipThresh + 0.2 * clipThresh * tanh((processedSignal - clipThresh) / (0.2 * clipThresh));
        } else if (processedSignal < -clipThresh) {
            processedSignal = -clipThresh + 0.2 * clipThresh * tanh((processedSignal + clipThresh) / (0.2 * clipThresh));
        }
    }

    // Apply harmonic enhancement
    double harmonicSignal = processedSignal + harmonicContent * processedSignal * processedSignal * (processedSignal > 0 ? 1 : -1);
    processedSignal = processedSignal * (1.0 - harmonicContent) + harmonicSignal * harmonicContent;

    // Apply makeup gain
    double makeupMultiplier = pow(10.0, makeupGain / 20.0);
    outputSignal = processedSignal * makeupMultiplier;

    // Apply final soft limiting to ensure ceiling is not exceeded
    double ceilingLevel = pow(10.0, ceiling / 20.0);
    if (outputSignal > ceilingLevel * 0.95) outputSignal = ceilingLevel * 0.95 + 0.05 * ceilingLevel * tanh((outputSignal - ceilingLevel * 0.95) / (0.05 * ceilingLevel));
    if (outputSignal < -ceilingLevel * 0.95) outputSignal = -ceilingLevel * 0.95 + 0.05 * ceilingLevel * tanh((outputSignal + ceilingLevel * 0.95) / (0.05 * ceilingLevel));
}

void TubeMaximizer::setCeiling(double ceiling) {
    this->ceiling = std::max(-12.0, std::min(0.0, ceiling));
}

void TubeMaximizer::setAttackTime(double time) {
    this->attackTime = std::max(0.00001, std::min(0.01, time)); // 10µs to 10ms
    attackCoeff = exp(-1.0 / (attackTime * sampleRate));
}

void TubeMaximizer::setReleaseTime(double time) {
    this->releaseTime = std::max(0.001, std::min(2.0, time)); // 1ms to 2s
    releaseCoeff = exp(-1.0 / (releaseTime * sampleRate));
}

void TubeMaximizer::setMakeupGain(double gain) {
    this->makeupGain = std::max(0.0, std::min(30.0, gain));
}

void TubeMaximizer::setAdaptiveRelease(bool adaptive) {
    this->adaptiveRelease = adaptive;
}

void TubeMaximizer::setLookAheadTime(double time) {
    this->lookAheadTime = std::max(0.0001, std::min(0.01, time)); // 0.1ms to 10ms
    // Resize delay buffer based on new look-ahead time
    int delaySamples = static_cast<int>(lookAheadTime * sampleRate);
    delayBufferSize = delaySamples + 10;  // Add some extra space
    delayBuffer.resize(delayBufferSize, 0.0);
    delayWritePosition = 0;
}

void TubeMaximizer::setHarmonicContent(double content) {
    this->harmonicContent = std::max(0.0, std::min(0.5, content));
}


// TubeLoudnessCompressor implementation
TubeLoudnessCompressor::TubeLoudnessCompressor(LoudnessCompressorType type) : compressorType(type) {
    initializeCompressor(type);
    
    // Initialize K-filter coefficients (simulating the K-weighting used in loudness measurements)
    // This is a simplified IIR filter that approximates the K-weighting curve
    // The coefficients are calculated for a 44.1kHz sample rate
    double fs = sampleRate * oversamplingFactor;  // Effective sample rate with oversampling
    
    // K-filter coefficients (simplified implementation)
    // The K-weighting filter is a high-frequency shelving filter that emphasizes higher frequencies
    // in a way that approximates human hearing perception
    double f0 = 1681.97445;  // Corner frequency in Hz
    double Q = 0.707106781;   // Quality factor (sqrt(2)/2 for Butterworth)
    double A = pow(10.0, 3.99984374/20.0);  // Gain factor (~4dB)

    double omega = 2 * M_PI * f0 / fs;
    double sin_omega = sin(omega);
    double cos_omega = cos(omega);
    double alpha = sin_omega / (2 * Q);

    double b0 = 1 + alpha * A;
    double b1 = -2 * cos_omega;
    double b2 = 1 - alpha * A;
    double a0 = 1 + alpha / A;
    double a1 = -2 * cos_omega;
    double a2 = 1 - alpha / A;

    // Normalize coefficients
    kFilter_b0 = b0 / a0;
    kFilter_b1 = b1 / a0;
    kFilter_b2 = b2 / a0;
    kFilter_a1 = a1 / a0;
    kFilter_a2 = a2 / a0;
    
    // Initialize buffers
    signalBuffer.resize(integratedWindow, 0.0);
    kFilteredBuffer.resize(integratedWindow, 0.0);
    kFilterState.resize(2, 0.0);
    
    // Initialize oversampling buffer if needed
    if (oversamplingFactor > 1) {
        oversampledBuffer.resize(oversamplingFactor, 0.0);
        effectiveSampleRate = sampleRate * oversamplingFactor;
    } else {
        effectiveSampleRate = sampleRate;
    }
    
    integratedLoudness = -70.0;  // Start with quiet signal
    shortTermLoudness = -70.0;
    momentaryLoudness = -70.0;
    currentGain = 1.0;
    smoothGain = 1.0;
    targetGain = 1.0;
    
    tubeLoudnessFactor = 0.4;  // Default tube contribution to loudness processing
}

void TubeLoudnessCompressor::initializeCompressor(LoudnessCompressorType type) {
    switch (type) {
        case INTEGRATED_ONLY:
            integratedTarget = -16.0;  // Standard integrated loudness target
            range = 7.0;               // Standard dynamic range
            lra = 10.0;                // Standard loudness range
            truePeakCeiling = -1.0;    // True peak ceiling
            break;

        case SHORT_TERM:
            integratedTarget = -14.0;
            range = 7.0;
            lra = 7.0;
            truePeakCeiling = -1.2;
            shortTermWindow = static_cast<int>(sampleRate * 3);  // 3-second window
            break;

        case MOMENTARY:
            integratedTarget = -12.0;
            range = 5.0;  // More compressed range
            lra = 5.0;
            truePeakCeiling = -1.5;
            momentaryWindow = static_cast<int>(sampleRate * 0.4);  // 0.4-second window
            shortTermWindow = static_cast<int>(sampleRate * 3);    // 3-second window
            break;

        case TRUE_PEAK:
            integratedTarget = -14.0;
            range = 7.0;
            lra = 7.0;
            truePeakCeiling = -2.0;  // Even more conservative ceiling
            oversamplingFactor = 8;  // Higher oversampling for accurate peak detection
            effectiveSampleRate = sampleRate * oversamplingFactor;
            oversampledBuffer.resize(oversamplingFactor, 0.0);
            break;
    }
}

bool TubeLoudnessCompressor::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeLoudnessCompressor::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == inputPin && data_bytes == sizeof(double)) {
        memcpy(&inputSignal, data, sizeof(double));
        return true;
    } else if (conn_id == controlPin && data_bytes == sizeof(double)) {
        memcpy(&controlSignal, data, sizeof(double));
        // Control signal can affect target loudness
        setIntegratedTarget(integratedTarget + 4.0 * controlSignal);  // Adjust target by ±4 LU
        return true;
    } else if (conn_id == targetPin && data_bytes == sizeof(double)) {
        memcpy(&targetSignal, data, sizeof(double));
        // Direct target setting
        setIntegratedTarget(-20.0 + 10.0 * (targetSignal + 1.0));  // Map -1,1 to -20 to -10 LUFS
        return true;
    }
    return false;
}

bool TubeLoudnessCompressor::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == outputPin && data_bytes == sizeof(double)) {
        memcpy(data, &outputSignal, sizeof(double));
        return true;
    }
    return false;
}

bool TubeLoudnessCompressor::Tick() {
    processSignal();
    return true;
}

double TubeLoudnessCompressor::calculateKWeightedLoudness(const std::vector<double>& buffer, int start, int length) {
    // Calculate the K-weighted loudness over the given buffer segment
    // This simulates the K-weighting filter used in loudness measurements
    
    // For this simplified implementation, we'll calculate the average
    // K-weighted power over the window and convert to LUFS
    double sum = 0.0;
    int count = 0;
    
    for (int i = 0; i < length && (start + i) < buffer.size(); i++) {
        double sample = buffer[(start + i) % buffer.size()];
        if (!std::isnan(sample) && !std::isinf(sample)) {
            sum += sample * sample;
            count++;
        }
    }
    
    if (count > 0) {
        double meanSquare = sum / count;
        if (meanSquare > 0.0) {
            // Convert to LUFS (with -0.691dB correction to align with ITU-R BS.1770)
            return 10.0 * log10(meanSquare) - 0.691;
        } else {
            return -70.0;  // Very quiet
        }
    } else {
        return -70.0;  // Very quiet
    }
}

void TubeLoudnessCompressor::updateLoudnessMeasurements() {
    // Add current input signal to buffer with K-filtering
    signalBuffer[bufferWritePos] = inputSignal;
    kFilteredBuffer[bufferWritePos] = applyKFilter(inputSignal);
    
    // Calculate integrated loudness (over 3+ seconds window)
    integratedLoudness = calculateKWeightedLoudness(kFilteredBuffer, 
        (bufferWritePos - integratedWindow + signalBuffer.size()) % signalBuffer.size(), 
        integratedWindow);
    
    // Calculate short-term loudness (over 3 seconds window)
    if (compressorType >= SHORT_TERM) {
        shortTermLoudness = calculateKWeightedLoudness(kFilteredBuffer, 
            (bufferWritePos - shortTermWindow + signalBuffer.size()) % signalBuffer.size(), 
            shortTermWindow);
    }
    
    // Calculate momentary loudness (over 0.4 seconds window)
    if (compressorType >= MOMENTARY) {
        momentaryLoudness = calculateKWeightedLoudness(kFilteredBuffer, 
            (bufferWritePos - momentaryWindow + signalBuffer.size()) % signalBuffer.size(), 
            momentaryWindow);
    }
    
    bufferWritePos = (bufferWritePos + 1) % signalBuffer.size();
}

double TubeLoudnessCompressor::calculateTargetGain() {
    // Calculate gain based on the difference between current loudness and target
    double loudnessDiff = integratedTarget - integratedLoudness;
    
    // Convert loudness difference to gain factor
    double gainFromIntegrated = pow(10.0, loudnessDiff / 20.0);
    
    // For more sophisticated control, consider short-term and momentary loudness
    if (compressorType >= SHORT_TERM) {
        double shortTermDiff = integratedTarget - shortTermLoudness;
        double gainFromShortTerm = pow(10.0, shortTermDiff / 20.0);
        gainFromIntegrated = std::min(gainFromIntegrated, gainFromShortTerm);
    }
    
    if (compressorType >= MOMENTARY) {
        double momentaryDiff = integratedTarget - momentaryLoudness;
        double gainFromMomentary = pow(10.0, momentaryDiff / 20.0);
        gainFromIntegrated = std::min(gainFromIntegrated, gainFromMomentary);
    }
    
    // Limit the gain to prevent extreme changes
    gainFromIntegrated = std::max(0.1, std::min(10.0, gainFromIntegrated));
    
    return gainFromIntegrated;
}

double TubeLoudnessCompressor::applyKFilter(double input) {
    // Apply the K-weighting filter to the input signal
    // This simulates the frequency response of human hearing
    
    // Direct form II implementation of biquad filter
    double output = kFilter_b0 * input + kFilterState[kFilterStateIndex];
    
    // Update filter state
    kFilterState[kFilterStateIndex] = kFilter_b1 * input - kFilter_a1 * output + kFilterState[(kFilterStateIndex + 1) % 2];
    kFilterState[(kFilterStateIndex + 1) % 2] = kFilter_b2 * input - kFilter_a2 * output;
    
    kFilterStateIndex = (kFilterStateIndex + 1) % 2;
    
    return output;
}

double TubeLoudnessCompressor::calculateTruePeak(const std::vector<double>& buffer, int start, int length) {
    // Calculate true peak by oversampling and finding peak
    // This is a simplified implementation - in real systems, you'd use a proper interpolator
    if (buffer.empty()) return 0.0;
    
    double maxAbs = 0.0;
    for (int i = 0; i < length && (start + i) < buffer.size(); i++) {
        double sample = buffer[(start + i) % buffer.size()];
        double absSample = std::abs(sample);
        if (absSample > maxAbs) {
            maxAbs = absSample;
        }
    }
    
    // Convert to dBTP (dB True Peak)
    if (maxAbs > 0.0) {
        return 20.0 * log10(maxAbs);
    } else {
        return -100.0;  // Very low
    }
}

void TubeLoudnessCompressor::processSignal() {
    // Update loudness measurements
    updateLoudnessMeasurements();
    
    // Calculate target gain based on loudness measurements
    targetGain = calculateTargetGain();
    
    // Apply tube characteristics to loudness processing
    double tubeFactor = 1.0 - tubeLoudnessFactor * (1.0 - targetGain);
    
    // Smooth the gain transition to avoid abrupt changes
    smoothGain = smoothGain * 0.95 + targetGain * 0.05;
    
    // Apply gain to input signal
    double processedSignal = inputSignal * smoothGain * tubeFactor;
    
    // Apply True Peak limiting if enabled
    if (truePeakLimiterEnabled) {
        // For true peak limiting, we need to oversample and check for inter-sample peaks
        if (oversamplingFactor > 1) {
            // Simple interpolation for oversampling (in a real system, you'd use a proper interpolator)
            // For now, we'll just check if the signal approaches the ceiling
            double ceiling = pow(10.0, truePeakCeiling / 20.0);
            
            if (std::abs(processedSignal) > ceiling * 0.9) {
                // Apply soft limiting
                double excess = std::abs(processedSignal) - ceiling * 0.9;
                double reduction = 1.0 - (excess / (1.0 - ceiling * 0.9));
                reduction = std::max(0.0, std::min(1.0, reduction));
                processedSignal *= reduction;
            }
        } else {
            // Just apply basic ceiling limiting
            double ceiling = pow(10.0, truePeakCeiling / 20.0);
            if (processedSignal > ceiling) processedSignal = ceiling;
            else if (processedSignal < -ceiling) processedSignal = -ceiling;
        }
    }
    
    // Apply tube saturation characteristics for musicality
    if (processedSignal > 0.8) processedSignal = 0.8 + 0.2 * tanh((processedSignal - 0.8) / 0.2);
    if (processedSignal < -0.8) processedSignal = -0.8 + 0.2 * tanh((processedSignal + 0.8) / 0.2);
    
    outputSignal = processedSignal;
}

void TubeLoudnessCompressor::setIntegratedTarget(double lufs) {
    integratedTarget = std::max(-70.0, std::min(0.0, lufs));  // LUFS range: -70 to 0
}

void TubeLoudnessCompressor::setRange(double range) {
    this->range = std::max(0.1, std::min(30.0, range));  // Range in LU
}

void TubeLoudnessCompressor::setLRA(double lra) {
    this->lra = std::max(1.0, std::min(20.0, lra));  // Loudness Range in LU
}

void TubeLoudnessCompressor::setTruePeakCeiling(double ceiling) {
    this->truePeakCeiling = std::max(-12.0, std::min(0.0, ceiling));  // Ceiling in dBTP
}

void TubeLoudnessCompressor::setOversampling(int factor) {
    int validFactors[] = {1, 2, 4, 8, 16};
    oversamplingFactor = 1;
    for (int f : validFactors) {
        if (f == factor) {
            oversamplingFactor = factor;
            break;
        }
    }
    
    // Resize oversampling buffer
    if (oversamplingFactor > 1) {
        oversampledBuffer.resize(oversamplingFactor, 0.0);
        effectiveSampleRate = sampleRate * oversamplingFactor;
    } else {
        effectiveSampleRate = sampleRate;
    }
}


// TubeLoudnessLimiter implementation
TubeLoudnessLimiter::TubeLoudnessLimiter(LoudnessLimiterType type) : limiterType(type) {
    initializeLimiter(type);
    
    // Initialize K-filter coefficients (same as in compressor)
    double fs = sampleRate * oversamplingFactor;
    
    // K-filter coefficients (simplified implementation)
    double f0 = 1681.97445;  // Corner frequency in Hz
    double Q = 0.707106781;   // Quality factor
    double A = pow(10.0, 3.99984374/20.0);  // Gain factor (~4dB)

    double omega = 2 * M_PI * f0 / fs;
    double sin_omega = sin(omega);
    double cos_omega = cos(omega);
    double alpha = sin_omega / (2 * Q);

    double b0 = 1 + alpha * A;
    double b1 = -2 * cos_omega;
    double b2 = 1 - alpha * A;
    double a0 = 1 + alpha / A;
    double a1 = -2 * cos_omega;
    double a2 = 1 - alpha / A;

    // Normalize coefficients
    kFilter_b0 = b0 / a0;
    kFilter_b1 = b1 / a0;
    kFilter_b2 = b2 / a0;
    kFilter_a1 = a1 / a0;
    kFilter_a2 = a2 / a0;
    
    // Initialize buffers
    signalBuffer.resize(integratedWindow, 0.0);
    kFilteredBuffer.resize(integratedWindow, 0.0);
    kFilterState.resize(2, 0.0);
    
    // Initialize oversampling buffer if needed
    if (oversamplingFactor > 1) {
        oversampledBuffer.resize(oversamplingFactor, 0.0);
        effectiveSampleRate = sampleRate * oversamplingFactor;
    } else {
        effectiveSampleRate = sampleRate;
    }
    
    integratedLoudness = -70.0;
    shortTermLoudness = -70.0;
    momentaryLoudness = -70.0;
    currentGain = 1.0;
    maxGainReduction = 1.0;
    
    // Calculate attack and release coefficients
    attackCoeff = exp(-1.0 / (attackTime * effectiveSampleRate));
    releaseCoeff = exp(-1.0 / (releaseTime * effectiveSampleRate));
    
    tubeLoudnessFactor = 0.3;  // Default tube contribution to loudness processing
}

void TubeLoudnessLimiter::initializeLimiter(LoudnessLimiterType type) {
    switch (type) {
        case INTEGRATED_LIMITER:
            lufsCeiling = -1.0;      // 1 LU above target
            integratedTarget = -23.0; // EBU R128 standard
            attackTime = 0.0001;     // 0.1ms very fast attack
            releaseTime = 0.2;       // 200ms release
            truePeakCeiling = -1.0;  // True peak ceiling
            break;

        case SHORT_TERM_LIMITER:
            lufsCeiling = -1.5;      // More conservative ceiling
            integratedTarget = -23.0;
            attackTime = 0.0001;     // 0.1ms attack
            releaseTime = 0.15;      // 150ms release with adaptive behavior
            truePeakCeiling = -1.2;
            break;

        case MOMENTARY_LIMITER:
            lufsCeiling = -2.0;      // Even more conservative for momentary
            integratedTarget = -21.0; // Slightly higher target for momentary focus
            attackTime = 0.00005;    // 0.05ms ultra-fast attack
            releaseTime = 0.1;       // 100ms release
            truePeakCeiling = -1.5;
            break;

        case TRUE_PEAK_LIMITER:
            lufsCeiling = -1.0;
            integratedTarget = -23.0;
            attackTime = 0.00002;    // 0.02ms fastest attack
            releaseTime = 0.08;      // 80ms release
            truePeakCeiling = -2.0;  // Very conservative true peak ceiling
            oversamplingFactor = 8;  // Higher oversampling for accurate peak detection
            effectiveSampleRate = sampleRate * oversamplingFactor;
            oversampledBuffer.resize(oversamplingFactor, 0.0);
            break;
    }
    
    // Update attack and release coefficients
    attackCoeff = exp(-1.0 / (attackTime * effectiveSampleRate));
    releaseCoeff = exp(-1.0 / (releaseTime * effectiveSampleRate));
}

bool TubeLoudnessLimiter::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeLoudnessLimiter::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == inputPin && data_bytes == sizeof(double)) {
        memcpy(&inputSignal, data, sizeof(double));
        return true;
    } else if (conn_id == controlPin && data_bytes == sizeof(double)) {
        memcpy(&controlSignal, data, sizeof(double));
        // Control signal can affect limiter behavior
        setLUFSCeiling(lufsCeiling + 2.0 * controlSignal);  // Adjust ceiling by ±2 LU
        return true;
    } else if (conn_id == ceilingPin && data_bytes == sizeof(double)) {
        memcpy(&ceilingSignal, data, sizeof(double));
        // Direct ceiling setting
        setLUFSCeiling(-3.0 + 2.0 * (ceilingSignal + 1.0));  // Map -1,1 to -3 to -1 LUFS
        return true;
    }
    return false;
}

bool TubeLoudnessLimiter::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == outputPin && data_bytes == sizeof(double)) {
        memcpy(data, &outputSignal, sizeof(double));
        return true;
    }
    return false;
}

bool TubeLoudnessLimiter::Tick() {
    processSignal();
    return true;
}

double TubeLoudnessLimiter::calculateKWeightedLoudness(const std::vector<double>& buffer, int start, int length) {
    // Calculate the K-weighted loudness over the given buffer segment
    double sum = 0.0;
    int count = 0;
    
    for (int i = 0; i < length && (start + i) < buffer.size(); i++) {
        double sample = buffer[(start + i) % buffer.size()];
        if (!std::isnan(sample) && !std::isinf(sample)) {
            sum += sample * sample;
            count++;
        }
    }
    
    if (count > 0) {
        double meanSquare = sum / count;
        if (meanSquare > 0.0) {
            // Convert to LUFS (with -0.691dB correction)
            return 10.0 * log10(meanSquare) - 0.691;
        } else {
            return -70.0;  // Very quiet
        }
    } else {
        return -70.0;  // Very quiet
    }
}

void TubeLoudnessLimiter::updateLoudnessMeasurements() {
    // Add current input signal to buffer with K-filtering
    signalBuffer[bufferWritePos] = inputSignal;
    kFilteredBuffer[bufferWritePos] = applyKFilter(inputSignal);
    
    // Calculate integrated loudness (over 3+ seconds window)
    integratedLoudness = calculateKWeightedLoudness(kFilteredBuffer, 
        (bufferWritePos - integratedWindow + signalBuffer.size()) % signalBuffer.size(), 
        integratedWindow);
    
    // Calculate short-term loudness (over 3 seconds window)
    if (limiterType >= SHORT_TERM_LIMITER) {
        shortTermLoudness = calculateKWeightedLoudness(kFilteredBuffer, 
            (bufferWritePos - shortTermWindow + signalBuffer.size()) % signalBuffer.size(), 
            shortTermWindow);
    }
    
    // Calculate momentary loudness (over 0.4 seconds window)
    if (limiterType >= MOMENTARY_LIMITER) {
        momentaryLoudness = calculateKWeightedLoudness(kFilteredBuffer, 
            (bufferWritePos - momentaryWindow + signalBuffer.size()) % signalBuffer.size(), 
            momentaryWindow);
    }
    
    bufferWritePos = (bufferWritePos + 1) % signalBuffer.size();
}

double TubeLoudnessLimiter::calculateLimiterGain() {
    // Determine the limiting threshold based on target + ceiling
    double threshold = integratedTarget + lufsCeiling;
    
    // Calculate gain needed to bring integrated loudness to threshold
    double integratedExcess = integratedLoudness - threshold;
    double integratedGain = 1.0;
    if (integratedExcess > 0.0) {
        integratedGain = pow(10.0, -integratedExcess / 20.0);
    }
    
    // Calculate gain for short-term loudness if needed
    double shortTermGain = 1.0;
    if (limiterType >= SHORT_TERM_LIMITER) {
        double shortTermThreshold = integratedTarget + lufsCeiling - 2.0; // Short-term allowed to be higher
        double shortTermExcess = shortTermLoudness - shortTermThreshold;
        if (shortTermExcess > 0.0) {
            shortTermGain = pow(10.0, -shortTermExcess / 20.0);
        }
    }
    
    // Calculate gain for momentary loudness if needed
    double momentaryGain = 1.0;
    if (limiterType >= MOMENTARY_LIMITER) {
        double momentaryThreshold = integratedTarget + lufsCeiling - 4.0; // Momentary allowed to be higher
        double momentaryExcess = momentaryLoudness - momentaryThreshold;
        if (momentaryExcess > 0.0) {
            momentaryGain = pow(10.0, -momentaryExcess / 20.0);
        }
    }
    
    // Take the minimum gain (most limiting) of all approaches
    double requiredGain = std::min(integratedGain, std::min(shortTermGain, momentaryGain));
    
    // Apply adaptive release if enabled
    if (adaptiveReleaseEnabled) {
        // If we're not currently limiting much, allow faster release
        if (requiredGain > maxGainReduction * 1.1) { // 1.1 to account for small fluctuations
            // Releasing - use release coefficient
            maxGainReduction = maxGainReduction * releaseCoeff + requiredGain * (1.0 - releaseCoeff);
        } else {
            // Attacking or holding - quickly take the new required gain
            maxGainReduction = std::min(maxGainReduction, requiredGain);
        }
    } else {
        // Standard attack/release
        if (requiredGain < currentGain) {
            // Attacking (applying more limiting)
            currentGain = requiredGain; // Fast attack
        } else {
            // Releasing (reducing limiting)
            currentGain = currentGain * releaseCoeff + requiredGain * (1.0 - releaseCoeff);
        }
        return currentGain;
    }
    
    return maxGainReduction;
}

double TubeLoudnessLimiter::applyKFilter(double input) {
    // Apply the K-weighting filter to the input signal
    double output = kFilter_b0 * input + kFilterState[kFilterStateIndex];
    
    // Update filter state
    kFilterState[kFilterStateIndex] = kFilter_b1 * input - kFilter_a1 * output + kFilterState[(kFilterStateIndex + 1) % 2];
    kFilterState[(kFilterStateIndex + 1) % 2] = kFilter_b2 * input - kFilter_a2 * output;
    
    kFilterStateIndex = (kFilterStateIndex + 1) % 2;
    
    return output;
}

double TubeLoudnessLimiter::calculateTruePeak(const std::vector<double>& buffer, int start, int length) {
    // Calculate true peak by oversampling and finding peak
    if (buffer.empty()) return 0.0;
    
    double maxAbs = 0.0;
    for (int i = 0; i < length && (start + i) < buffer.size(); i++) {
        double sample = buffer[(start + i) % buffer.size()];
        double absSample = std::abs(sample);
        if (absSample > maxAbs) {
            maxAbs = absSample;
        }
    }
    
    // Convert to dBTP
    if (maxAbs > 0.0) {
        return 20.0 * log10(maxAbs);
    } else {
        return -100.0;  // Very low
    }
}

void TubeLoudnessLimiter::processSignal() {
    // Update loudness measurements
    updateLoudnessMeasurements();
    
    // Calculate required limiting gain
    double newGain = calculateLimiterGain();
    
    // Apply tube characteristics to loudness processing
    double tubeFactor = 1.0 - tubeLoudnessFactor * (1.0 - newGain);
    
    // Apply the limiting gain to input signal
    double processedSignal = inputSignal * newGain * tubeFactor;
    
    // Apply True Peak limiting if enabled
    if (truePeakLimiterEnabled) {
        double ceiling = pow(10.0, truePeakCeiling / 20.0);
        
        if (std::abs(processedSignal) > ceiling * 0.95) {
            // Hard limit to prevent overshoot
            processedSignal = std::max(-ceiling, std::min(ceiling, processedSignal));
        }
    }
    
    // Apply tube saturation characteristics for musicality
    if (processedSignal > 0.85) processedSignal = 0.85 + 0.15 * tanh((processedSignal - 0.85) / 0.15);
    if (processedSignal < -0.85) processedSignal = -0.85 + 0.15 * tanh((processedSignal + 0.85) / 0.15);
    
    outputSignal = processedSignal;
}

void TubeLoudnessLimiter::setLUFSCeiling(double lufs) {
    lufsCeiling = std::max(-10.0, std::min(10.0, lufs));  // LUFS ceiling range: -10 to 10
}

void TubeLoudnessLimiter::setTruePeakCeiling(double ceiling) {
    this->truePeakCeiling = std::max(-12.0, std::min(0.0, ceiling));  // Ceiling in dBTP
}

void TubeLoudnessLimiter::setOversampling(int factor) {
    int validFactors[] = {1, 2, 4, 8, 16};
    oversamplingFactor = 1;
    for (int f : validFactors) {
        if (f == factor) {
            oversamplingFactor = factor;
            break;
        }
    }
    
    // Update effective sample rate
    effectiveSampleRate = sampleRate * oversamplingFactor;
    
    // Recalculate coefficients for new effective sample rate
    attackCoeff = exp(-1.0 / (attackTime * effectiveSampleRate));
    releaseCoeff = exp(-1.0 / (releaseTime * effectiveSampleRate));
    
    // Resize oversampling buffer
    if (oversamplingFactor > 1) {
        oversampledBuffer.resize(oversamplingFactor, 0.0);
    }
}

void TubeLoudnessLimiter::setLimiterAttack(double time) {
    attackTime = std::max(0.00001, std::min(0.01, time)); // 10µs to 10ms
    attackCoeff = exp(-1.0 / (attackTime * effectiveSampleRate));
}

void TubeLoudnessLimiter::setLimiterRelease(double time) {
    releaseTime = std::max(0.001, std::min(2.0, time)); // 1ms to 2s
    releaseCoeff = exp(-1.0 / (releaseTime * effectiveSampleRate));
}


// TubeLimiter implementation
TubeLimiter::TubeLimiter(LimiterType type) : limiterType(type) {
    initializeLimiter(type);

    // Calculate coefficients for attack and release
    attackCoeff = exp(-1.0 / (attackTime * sampleRate));
    releaseCoeff = exp(-1.0 / (releaseTime * sampleRate));

    tubeLimitingFactor = 0.4; // Higher tube contribution for limiter
}

void TubeLimiter::initializeLimiter(LimiterType type) {
    switch (type) {
        case PLAIN_LIMITER:
            threshold = -0.5;     // -0.5dB threshold
            ceiling = -0.1;       // -0.1dB ceiling
            attackTime = 0.0001;  // Very fast attack (0.1ms)
            releaseTime = 0.05;   // 50ms release
            tubeGain = 25.0;
            kneeWidth = 0.5;      // Narrow knee for hard limiting
            break;

        case DEESSING_LIMITER:
            threshold = -1.0;     // More conservative threshold
            ceiling = -0.2;       // -0.2dB ceiling
            attackTime = 0.0002;  // 0.2ms attack
            releaseTime = 0.1;    // 100ms release
            tubeGain = 30.0;      // Higher gain for de-essing
            kneeWidth = 1.0;      // Softer knee for de-essing
            break;

        case RMS_LIMITER:
            threshold = -1.5;     // RMS-based threshold
            ceiling = -0.3;       // -0.3dB ceiling
            attackTime = 0.001;   // 1ms attack (slightly slower for RMS)
            releaseTime = 0.08;   // 80ms release
            tubeGain = 22.0;
            kneeWidth = 1.5;      // Softer knee for RMS limiting
            autoReleaseEnabled = true;
            break;

        case VARI_MU_LIMITER:
            threshold = -0.8;     // Variable-Mu threshold
            ceiling = -0.15;      // -0.15dB ceiling
            attackTime = 0.0005;  // 0.5ms attack (medium speed)
            releaseTime = 0.15;   // 150ms release (slower, more musical)
            tubeGain = 28.0;      // High gain for vari-mu character
            kneeWidth = 2.0;      // Soft knee for vari-mu character
            break;
    }
}

bool TubeLimiter::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeLimiter::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == inputPin && data_bytes == sizeof(double)) {
        memcpy(&inputSignal, data, sizeof(double));
        return true;
    } else if (conn_id == controlPin && data_bytes == sizeof(double)) {
        memcpy(&controlSignal, data, sizeof(double));
        // Control signal can affect limiting parameters
        setThreshold(-2.0 + 1.5 * (controlSignal + 1.0));  // Map -1,1 to -2.0 to -0.5 dB
        return true;
    } else if (conn_id == sidechainPin && data_bytes == sizeof(double)) {
        memcpy(&sidechainSignal, data, sizeof(double));
        return true;
    }
    return false;
}

bool TubeLimiter::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == outputPin && data_bytes == sizeof(double)) {
        memcpy(data, &outputSignal, sizeof(double));
        return true;
    }
    return false;
}

bool TubeLimiter::Tick() {
    processSignal();
    return true;
}

double TubeLimiter::calculateLimitingGain(double inputLevel) {
    // Convert to dB
    double inputLevelDB = 20.0 * log10(std::abs(inputLevel) + 1e-9);
    double effectiveCeiling = ceiling;

    // If soft knee is enabled, implement smooth transition
    if (kneeWidth > 0.01) {
        // Soft knee limiting
        double softCeiling = effectiveCeiling - kneeWidth / 2.0;
        double softRange = kneeWidth;

        if (inputLevelDB < softCeiling) {
            // No limiting
            return 1.0;
        } else if (inputLevelDB < softCeiling + softRange) {
            // Soft knee transition
            double x = (inputLevelDB - softCeiling) / softRange;
            // Use a smooth curve from no limiting to full limiting
            double gainReductionDB = x * (inputLevelDB - effectiveCeiling);
            return pow(10.0, -gainReductionDB / 20.0);
        } else {
            // Full hard limiting
            double gainReductionDB = inputLevelDB - effectiveCeiling;
            return pow(10.0, -gainReductionDB / 20.0);
        }
    } else {
        // Hard knee limiting
        if (inputLevelDB < effectiveCeiling) {
            return 1.0;  // No limiting
        } else {
            // Apply hard limiting
            double gainReductionDB = inputLevelDB - effectiveCeiling;
            return pow(10.0, -gainReductionDB / 20.0);
        }
    }
}

void TubeLimiter::updateDetector() {
    // Simple peak detector for sidechain
    double inputLevel = std::abs(inputSignal);
    if (sidechainSignal != 0.0) {
        inputLevel = std::abs(sidechainSignal);  // Use external sidechain if provided
    }

    // Apply sidechain filtering if enabled
    if (true) { // Always apply for limiters
        // Simple peak detector with fast attack and release
        static double filteredLevel = 0.0;
        
        if (inputLevel > filteredLevel) {
            // Fast attack to catch peaks
            filteredLevel = inputLevel;
        } else {
            // Release based on release time
            filteredLevel = filteredLevel * releaseCoeff + inputLevel * (1.0 - releaseCoeff);
        }
        
        inputLevel = filteredLevel;
    }

    // Update detector level with attack/release characteristics appropriate for limiter
    if (inputLevel > detectorLevel) {
        // Fast attack to catch peaks immediately
        detectorLevel = inputLevel;
    } else {
        // Release with configured release time
        detectorLevel = detectorLevel * releaseCoeff + inputLevel * (1.0 - releaseCoeff);
    }
}

void TubeLimiter::processSignal() {
    // Update the detector
    updateDetector();

    // Calculate limiting gain based on detected level
    double newGain = calculateLimitingGain(detectorLevel);

    // Apply adaptive release if enabled
    if (autoReleaseEnabled) {
        // Adaptive release - faster when gain reduction is less
        double adaptiveReleaseCoeff = releaseCoeff * (0.5 + 0.5 * newGain); // Faster release when less limiting
        if (newGain < limiterGain) {
            // Attacking (applying more limiting) - respond immediately
            limiterGain = newGain;
        } else {
            // Releasing (reducing limiting) - use adaptive coefficient
            limiterGain = limiterGain * adaptiveReleaseCoeff + newGain * (1.0 - adaptiveReleaseCoeff);
        }
    } else {
        // Standard attack/release
        if (newGain < limiterGain) {
            // Attacking (applying more limiting) - respond immediately
            limiterGain = newGain;
        } else {
            // Releasing (reducing limiting) - use release coefficient
            limiterGain = limiterGain * releaseCoeff + newGain * (1.0 - releaseCoeff);
        }
    }

    // Apply tube characteristics to limiting
    // For limiters, we want to preserve the limiting characteristic while adding tube warmth
    double tubeFactor = 1.0 - tubeLimitingFactor * (1.0 - limiterGain);

    // Apply limiting to input
    double limitedSignal = inputSignal * limiterGain * tubeFactor;

    // Apply overshoot protection if enabled
    if (overshootProtection) {
        double ceilingLevel = pow(10.0, ceiling / 20.0);
        if (limitedSignal > ceilingLevel * 0.98) {
            limitedSignal = ceilingLevel * 0.98 + 0.02 * ceilingLevel * tanh((limitedSignal - ceilingLevel * 0.98) / (0.02 * ceilingLevel));
        } else if (limitedSignal < -ceilingLevel * 0.98) {
            limitedSignal = -ceilingLevel * 0.98 + 0.02 * ceilingLevel * tanh((limitedSignal + ceilingLevel * 0.98) / (0.02 * ceilingLevel));
        }
    }

    // Apply soft clipping if enabled for musicality
    if (softClippingEnabled) {
        double clipThreshold = pow(10.0, ceiling / 20.0) * 0.85; // Slightly below ceiling
        if (limitedSignal > clipThreshold) {
            limitedSignal = clipThreshold + 0.15 * clipThreshold * tanh((limitedSignal - clipThreshold) / (0.15 * clipThreshold));
        } else if (limitedSignal < -clipThreshold) {
            limitedSignal = -clipThreshold + 0.15 * clipThreshold * tanh((limitedSignal + clipThreshold) / (0.15 * clipThreshold));
        }
    }

    // Apply makeup gain
    double makeupMultiplier = pow(10.0, makeupGain / 20.0);
    outputSignal = limitedSignal * makeupMultiplier;

    // Final limiting to ensure ceiling is not exceeded
    double ceilingLevel = pow(10.0, ceiling / 20.0);
    if (outputSignal > ceilingLevel) outputSignal = ceilingLevel;
    if (outputSignal < -ceilingLevel) outputSignal = -ceilingLevel;
}

void TubeLimiter::setThreshold(double threshold) {
    this->threshold = std::max(-20.0, std::min(0.0, threshold));
}

void TubeLimiter::setCeiling(double ceiling) {
    this->ceiling = std::max(-10.0, std::min(0.0, ceiling));
}

void TubeLimiter::setAttackTime(double time) {
    this->attackTime = std::max(0.00001, std::min(0.01, time)); // 10µs to 10ms
    attackCoeff = exp(-1.0 / (attackTime * sampleRate));
}

void TubeLimiter::setReleaseTime(double time) {
    this->releaseTime = std::max(0.001, std::min(2.0, time)); // 1ms to 2s
    releaseCoeff = exp(-1.0 / (releaseTime * sampleRate));
}

void TubeLimiter::setMakeupGain(double gain) {
    this->makeupGain = std::max(-20.0, std::min(30.0, gain));
}

void TubeLimiter::setSoftKnee(double kneeWidth) {
    this->kneeWidth = std::max(0.0, std::min(5.0, kneeWidth));
}

void TubeLimiter::setOvershootProtection(bool enable) {
    this->overshootProtection = enable;
}


// TubeHarmonicExciter implementation
TubeHarmonicExciter::TubeHarmonicExciter(ExciterType type) : exciterType(type) {
    initializeExciter(type);
}

void TubeHarmonicExciter::initializeExciter(ExciterType type) {
    switch (type) {
        case ODD_HARMONIC:
            oddEvenBalance = -0.8;        // Mostly odd harmonics
            amount = 0.4;                 // Moderate amount
            harmonicOrder = 9;            // Up to 9th harmonic
            lowFrequency = 200.0;         // Start from 200Hz
            highFrequency = 5000.0;       // Up to 5kHz
            break;
            
        case EVEN_HARMONIC:
            oddEvenBalance = 0.7;         // Mostly even harmonics
            amount = 0.35;                // Moderate amount
            harmonicOrder = 8;            // Up to 8th harmonic
            lowFrequency = 150.0;         // Start from 150Hz
            highFrequency = 6000.0;       // Up to 6kHz
            break;
            
        case BALANCED_HARMONIC:
            oddEvenBalance = 0.0;         // Balanced
            amount = 0.3;                 // Moderate amount
            harmonicOrder = 7;            // Up to 7th harmonic
            lowFrequency = 100.0;         // Full range
            highFrequency = 8000.0;
            break;
            
        case FORMANT_EXCITER:
            oddEvenBalance = 0.2;         // Slightly even-heavy
            amount = 0.25;                // More subtle for formant control
            harmonicOrder = 11;           // Higher harmonics for formant shaping
            lowFrequency = 300.0;         // Voice range
            highFrequency = 4000.0;       // Voice range
            formantFreq = 1000.0;         // Default formant frequency
            break;
    }
}

bool TubeHarmonicExciter::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeHarmonicExciter::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == inputPin && data_bytes == sizeof(double)) {
        memcpy(&inputSignal, data, sizeof(double));
        return true;
    } else if (conn_id == amountPin && data_bytes == sizeof(double)) {
        memcpy(&amountControl, data, sizeof(double));
        setAmount(amount + 0.5 * amountControl); // Modulate around current setting
        return true;
    } else if (conn_id == balancePin && data_bytes == sizeof(double)) {
        memcpy(&balanceControl, data, sizeof(double));
        setOddEvenBalance(oddEvenBalance + 0.8 * balanceControl); // Modulate balance
        return true;
    }
    return false;
}

bool TubeHarmonicExciter::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == outputPin && data_bytes == sizeof(double)) {
        memcpy(data, &outputSignal, sizeof(double));
        return true;
    }
    return false;
}

bool TubeHarmonicExciter::Tick() {
    processSignal();
    return true;
}

double TubeHarmonicExciter::estimateFrequency() {
    // Very simplified frequency estimation based on zero crossings
    // In a real implementation, you'd use more sophisticated methods
    static double lastZeroCrossing = 0;
    static double estimatedFreq = 440.0; // Default to 440Hz
    
    // If we're crossing zero in a positive direction
    if ((previousInput <= 0 && inputSignal > 0) || (previousInput >= 0 && inputSignal < 0)) {
        // Estimate frequency based on time since last zero crossing
        static int sampleCounter = 0;
        if (sampleCounter > 0) {
            double estimated = sampleRate / sampleCounter;
            // Smooth the estimate
            estimatedFreq = 0.9 * estimatedFreq + 0.1 * estimated;
        }
        sampleCounter = 0;
    } else {
        sampleCounter++;
    }
    
    // Clamp to reasonable range
    estimatedFreq = std::max(50.0, std::min(5000.0, estimatedFreq));
    
    previousInput = inputSignal;
    return estimatedFreq;
}

double TubeHarmonicExciter::generateHarmonics(double input, double fundamentalFreq) {
    double output = 0.0;
    double inputFreq = estimateFrequency();
    
    // Only process if frequency is in our target range
    if (inputFreq >= lowFrequency && inputFreq <= highFrequency) {
        // Generate harmonics up to the specified order
        for (int h = 2; h <= harmonicOrder; h++) {
            // Determine if this is an odd or even harmonic
            bool isOdd = (h % 2 == 1);
            
            // Calculate harmonic frequency
            double harmonicFreq = fundamentalFreq * h;
            
            // Skip if harmonic is above our frequency range
            if (harmonicFreq > highFrequency) continue;
            
            // Calculate harmonic contribution based on odd/even balance
            double harmonicContribution = 0.0;
            if (isOdd) {
                // Odd harmonics contribution
                harmonicContribution = (1.0 - std::max(-1.0, std::min(1.0, oddEvenBalance))) / h;
            } else {
                // Even harmonics contribution
                harmonicContribution = (1.0 + std::max(-1.0, std::min(1.0, oddEvenBalance))) / h;
            }
            
            // Generate harmonic component with phase based on input
            double harmonicPhase = h * asin(std::max(-1.0, std::min(1.0, input)));
            double harmonicComponent = sin(harmonicPhase) * harmonicContribution;
            
            // Add to output
            output += harmonicComponent * amount;
        }
    }
    
    // Apply formant filtering if in formant mode
    if (exciterType == FORMANT_EXCITER) {
        // Simple resonant filter at formant frequency
        static double resonantOutput1 = 0.0, resonantOutput2 = 0.0;
        
        // Calculate filter coefficients for formant
        double omega = 2 * M_PI * formantFreq / sampleRate;
        double resonance = 0.7; // Quality factor
        double alpha = sin(omega) / (2 * resonance);
        
        double b0 = alpha;
        double b1 = 0.0;
        double b2 = -alpha;
        double a0 = 1 + alpha;
        double a1 = -2 * cos(omega);
        double a2 = 1 - alpha;
        
        // Apply resonant filter
        double filteredOutput = (b0 * output + b1 * resonantOutput1 + b2 * resonantOutput2 - 
                                a1 * resonantOutput1 - a2 * resonantOutput2) / a0;
        
        resonantOutput2 = resonantOutput1;
        resonantOutput1 = output;
        
        output = filteredOutput;
    }
    
    return output;
}

void TubeHarmonicExciter::processSignal() {
    // Calculate adaptive gain if enabled
    if (adaptiveExcitationEnabled) {
        // Adjust excitation based on input signal level
        double inputLevel = std::abs(inputSignal);
        adaptiveGain = 0.5 + 0.5 * (1.0 / (1.0 + inputLevel * 10)); // Less excitation for louder signals
    }
    
    // Use a basic frequency estimation for harmonic generation
    double estimatedFreq = estimateFrequency();
    
    // Generate harmonics
    double harmonics = generateHarmonics(inputSignal, estimatedFreq);
    
    // Mix original signal with harmonics
    double mixedSignal = inputSignal * (1.0 - amount * adaptiveGain * 0.7) + harmonics * 0.7;
    
    // Apply tube saturation if enabled
    if (tubeSaturationEnabled) {
        // Apply soft clipping to simulate tube saturation
        double saturation = 0.6 + 0.4 * oddEvenBalance; // Adjust based on odd/even balance
        if (mixedSignal > saturation) {
            mixedSignal = saturation + (1 - saturation) * tanh((mixedSignal - saturation) / (1 - saturation));
        } else if (mixedSignal < -saturation) {
            mixedSignal = -saturation + (1 - saturation) * tanh((mixedSignal + saturation) / (1 - saturation));
        }
    }
    
    // Apply tone control as a simple shelving filter
    double highFreqGain = 0.8 + 0.4 * toneControl; // Map 0-1 to 0.8-1.2
    mixedSignal = inputSignal * (1.0 - amount * 0.3) + mixedSignal * highFreqGain * amount * 0.3;
    
    outputSignal = mixedSignal;
    
    previousOutput = outputSignal;
}

void TubeHarmonicExciter::setAmount(double amount) {
    this->amount = std::max(0.0, std::min(1.0, amount));
}

void TubeHarmonicExciter::setOddEvenBalance(double balance) {
    // -1.0 = all odd harmonics, 1.0 = all even harmonics
    this->oddEvenBalance = std::max(-1.0, std::min(1.0, balance));
}

void TubeHarmonicExciter::setFrequencyRange(double low, double high) {
    this->lowFrequency = std::max(20.0, std::min(20000.0, low));
    this->highFrequency = std::max(lowFrequency, std::min(20000.0, high));
}

void TubeHarmonicExciter::setHarmonicOrder(int order) {
    this->harmonicOrder = std::max(2, std::min(20, order));
}

void TubeHarmonicExciter::setToneControl(double tone) {
    this->toneControl = std::max(0.0, std::min(1.0, tone));
}

void TubeHarmonicExciter::setFormantFrequency(double freq) {
    this->formantFreq = std::max(100.0, std::min(8000.0, freq));
}


// TubeTapeHarmonics implementation
TubeTapeHarmonics::TubeTapeHarmonics(TapeType type) : tapeType(type) {
    initializeTape(type);
    
    // Initialize noise buffer
    for (int i = 0; i < 10; i++) {
        noiseBuffer[i] = (static_cast<double>(rand()) / RAND_MAX - 0.5) * 0.1;
    }
}

void TubeTapeHarmonics::initializeTape(TapeType type) {
    switch (type) {
        case FERRIC_456:
            amount = 0.4;
            bias = 0.7;                    // Medium bias for 456
            speed = 1.0;                   // Standard speed (7.5 ips)
            noiseLevel = 0.03;             // Characteristic noise
            compression = 0.3;             // Medium compression
            wowFlutter = 0.02;             // Low wow/flutter
            break;
            
        case FERRIC_911:
            amount = 0.35;
            bias = 0.65;                   // Slightly lower bias
            speed = 1.0;                   // Standard speed
            noiseLevel = 0.025;            // Lower noise than 456
            compression = 0.25;            // Less compression
            wowFlutter = 0.03;             // Slightly more flutter
            break;
            
        case CHROME_TYPE_2:
            amount = 0.45;
            bias = 0.8;                    // Higher bias for chrome
            speed = 1.0;                   // Standard speed
            noiseLevel = 0.015;            // Lower noise
            compression = 0.2;             // Less compression
            wowFlutter = 0.01;             // Very low wow/flutter
            break;
            
        case METAL_TYPE_4:
            amount = 0.5;
            bias = 0.85;                   // Highest bias for metal
            speed = 1.0;                   // Standard speed
            noiseLevel = 0.01;             // Lowest noise
            compression = 0.15;            // Least compression
            wowFlutter = 0.005;            // Minimum wow/flutter
            break;
            
        case VINTAGE_REEL_TO_REEL:
            amount = 0.55;
            bias = 0.6;                    // Lower bias for vintage sound
            speed = 1.2;                   // Slightly faster to simulate RTR
            noiseLevel = 0.04;             // Higher noise for vintage charm
            compression = 0.4;             // More compression
            wowFlutter = 0.08;             // Noticeable wow/flutter
            break;
    }
}

bool TubeTapeHarmonics::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeTapeHarmonics::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == inputPin && data_bytes == sizeof(double)) {
        memcpy(&inputSignal, data, sizeof(double));
        return true;
    } else if (conn_id == amountPin && data_bytes == sizeof(double)) {
        memcpy(&amountControl, data, sizeof(double));
        setAmount(amount + 0.5 * amountControl); // Modulate around current setting
        return true;
    } else if (conn_id == biasPin && data_bytes == sizeof(double)) {
        memcpy(&biasControl, data, sizeof(double));
        setBias(bias + 0.3 * biasControl); // Modulate bias level
        return true;
    }
    return false;
}

bool TubeTapeHarmonics::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == outputPin && data_bytes == sizeof(double)) {
        memcpy(data, &outputSignal, sizeof(double));
        return true;
    }
    return false;
}

bool TubeTapeHarmonics::Tick() {
    processSignal();
    return true;
}

double TubeTapeHarmonics::applyTapeCompression(double input) {
    if (!tapeCompressionEnabled) {
        return input;
    }
    
    // Simulate magnetic tape's characteristic soft compression
    // This models the magnetic saturation characteristics of tape
    double bias_adjusted = 0.5 + 0.4 * bias; // Convert bias to useful range
    double saturation = 0.6 + 0.3 * compression; // Saturation level based on compression setting
    
    // Apply soft limiting similar to tape saturation
    if (input > saturation) {
        // Apply soft clipping characteristic of tape
        double excess = input - saturation;
        double gain = 1.0 / (1.0 + excess * 5.0); // Steeper curve near saturation
        input = saturation + (input - saturation) * gain;
    } else if (input < -saturation) {
        double excess = -input - saturation;
        double gain = 1.0 / (1.0 + excess * 5.0);
        input = -saturation - (-input - saturation) * gain;
    }
    
    // Add subtle harmonic distortion characteristic of tape
    // Tape adds even-order harmonics more than odd-order
    double harmonic_distortion = 0.02 * compression * (input * input * (input > 0 ? 1 : -1)) * (1.0 - bias);
    input += harmonic_distortion;
    
    return input;
}

double TubeTapeHarmonics::generateTapeNoise() {
    if (!noiseEnabled) {
        return 0.0;
    }
    
    // Simple noise generation with some correlation to simulate tape hiss
    double newNoise = (static_cast<double>(rand()) / RAND_MAX - 0.5) * noiseLevel;
    
    // Smooth the noise slightly to make it less harsh
    noiseBuffer[noiseIndex] = newNoise;
    noiseIndex = (noiseIndex + 1) % 10;
    
    // Average of recent noise values
    double avgNoise = 0.0;
    for (int i = 0; i < 10; i++) {
        avgNoise += noiseBuffer[i];
    }
    avgNoise /= 10.0;
    
    return avgNoise * 0.7;
}

double TubeTapeHarmonics::applyWowFlutter(double input, double phase) {
    if (!wowFlutterEnabled) {
        return input;
    }
    
    // Generate wow/flutter modulation
    // Wow is slower modulation (0.1 - 2Hz), flutter is faster (10-100Hz)
    double wow_rate = 0.5;    // 0.5Hz wow
    double flutter_rate = 25.0; // 25Hz flutter
    
    // Update phase
    wowPhase += 2.0 * M_PI * flutter_rate / sampleRate;
    if (wowPhase >= 2.0 * M_PI) {
        wowPhase -= 2.0 * M_PI;
    }
    
    // Create modulation effect
    double wow_mod = 0.5 * wowFlutter * sin(2.0 * M_PI * wow_rate * tapeHeadPosition / sampleRate);
    double flutter_mod = 0.5 * wowFlutter * sin(wowPhase);
    
    // Apply subtle pitch modulation
    return input * (1.0 + wow_mod + flutter_mod);
}

void TubeTapeHarmonics::processSignal() {
    // Apply tape compression first
    double processedSignal = applyTapeCompression(inputSignal);
    
    // Apply frequency response based on tape speed
    // Higher speed = more high-frequency content (simplified)
    double freqResponse = 0.8 + 0.2 * speed;
    processedSignal = inputSignal * (1.0 - amount * 0.3) + processedSignal * freqResponse * amount * 0.3;
    
    // Apply wow and flutter
    processedSignal = applyWowFlutter(processedSignal, tapeHeadPosition);
    
    // Add tape noise
    double noise = generateTapeNoise();
    processedSignal += noise * amount;
    
    // Add harmonic content characteristic of tape saturation
    double harmonicContent = 0.05 * bias * (processedSignal * processedSignal * (processedSignal > 0 ? 1 : -1));
    processedSignal = processedSignal * (1.0 - amount * 0.2) + harmonicContent * amount * 0.2;
    
    // Final mix
    outputSignal = inputSignal * (1.0 - amount) + processedSignal * amount;
    
    // Update tape head position (just a simulation counter)
    tapeHeadPosition += 1.0 / sampleRate;
    previousOutput = outputSignal;
}

void TubeTapeHarmonics::setAmount(double amount) {
    this->amount = std::max(0.0, std::min(1.0, amount));
}

void TubeTapeHarmonics::setBias(double bias) {
    this->bias = std::max(0.0, std::min(1.0, bias));
}

void TubeTapeHarmonics::setSpeed(double speed) {
    this->speed = std::max(0.1, std::min(3.0, speed));
}

void TubeTapeHarmonics::setNoiseLevel(double noise) {
    this->noiseLevel = std::max(0.0, std::min(1.0, noise));
}

void TubeTapeHarmonics::setCompression(double compression) {
    this->compression = std::max(0.0, std::min(1.0, compression));
}

void TubeTapeHarmonics::setWowFlutter(double wow) {
    this->wowFlutter = std::max(0.0, std::min(1.0, wow));
}


// TubeParametricEQ implementation
TubeParametricEQ::TubeParametricEQ(int numBands) {
    initializeEQ(numBands);
}

void TubeParametricEQ::initializeEQ(int numBands) {
    this->numBands = std::max(1, std::min(10, numBands));  // Limit to 1-10 bands
    bands.resize(this->numBands);
    
    // Default band configuration (4 bands: low shelf, 2 peaking, high shelf)
    if (this->numBands >= 1) {
        bands[0].type = LOW_SHELF;
        bands[0].frequency = 100.0;   // Low frequencies
        bands[0].gain = 0.0;
        bands[0].q = 0.707;
    }
    
    if (this->numBands >= 2) {
        bands[1].type = PEAKING;
        bands[1].frequency = 500.0;   // Low-mid frequencies
        bands[1].gain = 0.0;
        bands[1].q = 1.0;
    }
    
    if (this->numBands >= 3) {
        bands[2].type = PEAKING;
        bands[2].frequency = 2000.0;  // High-mid frequencies
        bands[2].gain = 0.0;
        bands[2].q = 1.0;
    }
    
    if (this->numBands >= 4) {
        bands[3].type = HIGH_SHELF;
        bands[3].frequency = 10000.0; // High frequencies
        bands[3].gain = 0.0;
        bands[3].q = 0.707;
    }
    
    // Initialize filter coefficients for all bands
    for (int i = 0; i < static_cast<int>(bands.size()); i++) {
        calculateFilterCoefficients(i);
    }
}

bool TubeParametricEQ::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeParametricEQ::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == inputPin && data_bytes == sizeof(double)) {
        memcpy(&inputSignal, data, sizeof(double));
        return true;
    }
    return false;
}

bool TubeParametricEQ::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == outputPin && data_bytes == sizeof(double)) {
        memcpy(data, &outputSignal, sizeof(double));
        return true;
    }
    return false;
}

bool TubeParametricEQ::Tick() {
    processSignal();
    return true;
}

void TubeParametricEQ::calculateFilterCoefficients(int bandIndex) {
    if (bandIndex < 0 || bandIndex >= static_cast<int>(bands.size())) return;
    
    auto& band = bands[bandIndex];
    
    // Normalize frequency to pi
    double w0 = 2 * M_PI * band.frequency / sampleRate;
    double cos_w0 = cos(w0);
    double sin_w0 = sin(w0);
    double A = pow(10.0, band.gain / 40.0);  // For peak/shelf filters
    
    switch (band.type) {
        case LOW_SHELF: {
            double alpha = sin_w0 / 2.0 * sqrt((A + 1/A) * (1/band.q - 1) + 2);
            double temp = 2 * sqrt(A) * alpha;
            
            band.b0 = A * ((A + 1) - (A - 1) * cos_w0 + temp);
            band.b1 = 2 * A * ((A - 1) - (A + 1) * cos_w0);
            band.b2 = A * ((A + 1) - (A - 1) * cos_w0 - temp);
            band.a0 = (A + 1) + (A - 1) * cos_w0 + temp;
            band.a1 = -2 * ((A - 1) + (A + 1) * cos_w0);
            band.a2 = (A + 1) + (A - 1) * cos_w0 - temp;
            break;
        }
        
        case HIGH_SHELF: {
            double alpha = sin_w0 / 2.0 * sqrt((A + 1/A) * (1/band.q - 1) + 2);
            double temp = 2 * sqrt(A) * alpha;
            
            band.b0 = A * ((A + 1) + (A - 1) * cos_w0 + temp);
            band.b1 = -2 * A * ((A - 1) + (A + 1) * cos_w0);
            band.b2 = A * ((A + 1) + (A - 1) * cos_w0 - temp);
            band.a0 = (A + 1) - (A - 1) * cos_w0 + temp;
            band.a1 = 2 * ((A - 1) - (A + 1) * cos_w0);
            band.a2 = (A + 1) - (A - 1) * cos_w0 - temp;
            break;
        }
        
        case PEAKING: {
            double alpha = sin_w0 / (2.0 * band.q);
            
            band.b0 = 1 + alpha * A;
            band.b1 = -2 * cos_w0;
            band.b2 = 1 - alpha * A;
            band.a0 = 1 + alpha / A;
            band.a1 = -2 * cos_w0;
            band.a2 = 1 - alpha / A;
            break;
        }
        
        case LOW_PASS: {
            double alpha = sin_w0 / (2.0 * band.q);
            
            band.b0 = (1 - cos_w0) / 2;
            band.b1 = 1 - cos_w0;
            band.b2 = (1 - cos_w0) / 2;
            band.a0 = 1 + alpha;
            band.a1 = -2 * cos_w0;
            band.a2 = 1 - alpha;
            break;
        }
        
        case HIGH_PASS: {
            double alpha = sin_w0 / (2.0 * band.q);
            
            band.b0 = (1 + cos_w0) / 2;
            band.b1 = -(1 + cos_w0);
            band.b2 = (1 + cos_w0) / 2;
            band.a0 = 1 + alpha;
            band.a1 = -2 * cos_w0;
            band.a2 = 1 - alpha;
            break;
        }
        
        case BAND_PASS: {
            double alpha = sin_w0 / (2.0 * band.q);
            
            band.b0 = alpha;
            band.b1 = 0;
            band.b2 = -alpha;
            band.a0 = 1 + alpha;
            band.a1 = -2 * cos_w0;
            band.a2 = 1 - alpha;
            break;
        }
    }
    
    // Normalize coefficients
    if (band.a0 != 0) {
        band.b0 /= band.a0;
        band.b1 /= band.a0;
        band.b2 /= band.a0;
        band.a1 /= band.a0;
        band.a2 /= band.a0;
        // band.a0 is normalized to 1.0
    }
}

double TubeParametricEQ::processBand(int bandIndex, double input) {
    if (bandIndex < 0 || bandIndex >= static_cast<int>(bands.size()) || !bands[bandIndex].enabled) {
        return input;
    }
    
    auto& band = bands[bandIndex];
    
    // Direct Form II implementation of biquad filter
    double output = band.b0 * input + band.x1 * band.b1 + band.x2 * band.b2 
                              - band.y1 * band.a1 - band.y2 * band.a2;
    
    // Update filter state
    band.x2 = band.x1;
    band.x1 = input;
    band.y2 = band.y1;
    band.y1 = output;
    
    return output;
}

void TubeParametricEQ::processSignal() {
    // Start with the input signal
    double signal = inputSignal;
    
    // Process through each enabled band in sequence
    for (auto& band : bands) {
        if (band.enabled) {
            signal = processBand(static_cast<int>(&band - &bands[0]), signal);
        }
    }
    
    // Apply tube characteristics if enabled
    if (tubeCharacteristicsEnabled) {
        // Apply soft saturation to simulate tube warmth
        double saturation = 0.7 + 0.3 * tubeSaturation; // Adjust saturation based on setting
        if (signal > saturation) {
            signal = saturation + (1 - saturation) * tanh((signal - saturation) / (1 - saturation));
        } else if (signal < -saturation) {
            signal = -saturation + (1 - saturation) * tanh((signal + saturation) / (1 - saturation));
        }
        
        // Add subtle harmonic content
        double harmonic = 0.05 * tubeSaturation * signal * signal * (signal > 0 ? 1 : -1);
        signal = signal * (1.0 - 0.1 * tubeSaturation) + harmonic * 0.1 * tubeSaturation;
    }
    
    outputSignal = signal;
}

void TubeParametricEQ::setNumBands(int numBands) {
    int oldNumBands = this->numBands;
    this->numBands = std::max(1, std::min(10, numBands));
    
    if (this->numBands > oldNumBands) {
        // Add new bands with default settings
        for (int i = oldNumBands; i < this->numBands; i++) {
            EQBand newBand;
            newBand.type = PEAKING;
            newBand.frequency = 1000.0 * pow(2.0, (i - 2));  // Doubling frequency for each band
            newBand.gain = 0.0;
            newBand.q = 1.0;
            bands.push_back(newBand);
        }
    }
    
    bands.resize(this->numBands);
    
    // Recalculate coefficients for all bands
    for (int i = 0; i < static_cast<int>(bands.size()); i++) {
        calculateFilterCoefficients(i);
    }
}

void TubeParametricEQ::setBandType(int bandIndex, EQBandType type) {
    if (bandIndex >= 0 && bandIndex < static_cast<int>(bands.size())) {
        bands[bandIndex].type = type;
        calculateFilterCoefficients(bandIndex);
    }
}

void TubeParametricEQ::setBandFrequency(int bandIndex, double freq) {
    if (bandIndex >= 0 && bandIndex < static_cast<int>(bands.size())) {
        bands[bandIndex].frequency = std::max(20.0, std::min(20000.0, freq));
        calculateFilterCoefficients(bandIndex);
    }
}

void TubeParametricEQ::setBandGain(int bandIndex, double gain) {
    if (bandIndex >= 0 && bandIndex < static_cast<int>(bands.size())) {
        bands[bandIndex].gain = std::max(-20.0, std::min(20.0, gain));
        calculateFilterCoefficients(bandIndex);
    }
}

void TubeParametricEQ::setBandQ(int bandIndex, double q) {
    if (bandIndex >= 0 && bandIndex < static_cast<int>(bands.size())) {
        bands[bandIndex].q = std::max(0.1, std::min(10.0, q));
        calculateFilterCoefficients(bandIndex);
    }
}

void TubeParametricEQ::enableBand(int bandIndex, bool enable) {
    if (bandIndex >= 0 && bandIndex < static_cast<int>(bands.size())) {
        bands[bandIndex].enabled = enable;
    }
}


// TubeStereoWidener implementation
TubeStereoWidener::TubeStereoWidener(WidenerType type) : widenerType(type) {
    initializeWidener(type);
    
    // Initialize delay buffers based on maximum delay time
    delayBufferSize = static_cast<int>(0.010 * sampleRate); // 10ms max delay
    leftDelayBuffer.resize(delayBufferSize, 0.0);
    rightDelayBuffer.resize(delayBufferSize, 0.0);
    
    // Initialize band frequencies for band-based widener
    bandFrequencies.resize(bandCount);
    for (int i = 0; i < bandCount; i++) {
        // Logarithmic spacing from 100Hz to 10kHz
        bandFrequencies[i] = 100.0 * pow(10000.0 / 100.0, static_cast<double>(i) / (bandCount - 1));
    }
}

void TubeStereoWidener::initializeWidener(WidenerType type) {
    switch (type) {
        case MID_SIDE_WIDENER:
            amount = 0.6;           // Moderate widening
            delayTimeMs = 1.2;      // 1.2ms delay
            width = 1.4;            // Increased width
            midSideRatio = 0.7;     // Emphasize side information
            break;
            
        case BAND_PASS_WIDENER:
            amount = 0.5;           // Balanced amount
            delayTimeMs = 0.8;      // 0.8ms delay
            width = 1.5;            // High width
            midSideRatio = 0.5;     // Balanced mid-side
            bandCount = 10;         // 10 bands as required
            break;
            
        case ALL_PASS_WIDENER:
            amount = 0.7;           // High widening
            delayTimeMs = 0.5;      // 0.5ms delay
            width = 1.7;            // Very wide
            midSideRatio = 0.3;     // More side processing
            break;
            
        case PHASE_WIDENER:
            amount = 0.4;           // Moderate widening
            delayTimeMs = 2.0;      // 2.0ms delay
            width = 1.3;            // Moderate width
            midSideRatio = 0.8;     // More mid processing
            break;
    }
}

bool TubeStereoWidener::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeStereoWidener::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == leftInput && data_bytes == sizeof(double)) {
        memcpy(&leftInputSignal, data, sizeof(double));
        return true;
    } else if (conn_id == rightInput && data_bytes == sizeof(double)) {
        memcpy(&rightInputSignal, data, sizeof(double));
        return true;
    } else if (conn_id == controlInput && data_bytes == sizeof(double)) {
        memcpy(&controlSignal, data, sizeof(double));
        setAmount(amount + 0.3 * controlSignal); // Modulate around current setting
        return true;
    }
    return false;
}

bool TubeStereoWidener::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == leftOutput && data_bytes == sizeof(double)) {
        memcpy(data, &leftOutputSignal, sizeof(double));
        return true;
    } else if (conn_id == rightOutput && data_bytes == sizeof(double)) {
        memcpy(data, &rightOutputSignal, sizeof(double));
        return true;
    }
    return false;
}

bool TubeStereoWidener::Tick() {
    processSignal();
    return true;
}

void TubeStereoWidener::stereoToMidSide(double left, double right, double& mid, double& side) {
    mid = (left + right) * 0.5;
    side = (left - right) * 0.5;
}

void TubeStereoWidener::midSideToStereo(double mid, double side, double& left, double& right) {
    left = mid + side;
    right = mid - side;
}

void TubeStereoWidener::processFrequencyBands(double& left, double& right) {
    if (widenerType != BAND_PASS_WIDENER) return;
    
    // For the band-based widener, we apply different delays to odd/even bands
    // This follows the specification in the task: "odd has + and even - delay in left channel"
    
    double originalLeft = left;
    double originalRight = right;
    
    // Process through each band
    for (int band = 0; band < bandCount; band++) {
        // Calculate filter coefficients for this band
        double omega = 2.0 * M_PI * bandFrequencies[band] / sampleRate;
        double sin_omega = sin(omega);
        double cos_omega = cos(omega);
        double alpha = sin_omega / 2.0;  // Low Q for broad bands
        
        // Simplified bandpass filter coefficients
        double a0 = 1.0 + alpha;
        double a1 = -2.0 * cos_omega;
        double a2 = 1.0 - alpha;
        double b0 = alpha;
        double b1 = 0.0;
        double b2 = -alpha;
        
        // Normalize
        b0 /= a0;
        b1 /= a0;
        b2 /= a0;
        a1 /= a0;
        a2 /= a0;
        
        // Apply bandpass filter to both channels
        static double lx1 = 0.0, lx2 = 0.0, ly1 = 0.0, ly2 = 0.0;  // Left filter state
        static double rx1 = 0.0, rx2 = 0.0, ry1 = 0.0, ry2 = 0.0;  // Right filter state
        
        double l_filtered = b0 * originalLeft + b1 * lx1 + b2 * lx2 - a1 * ly1 - a2 * ly2;
        double r_filtered = b0 * originalRight + b1 * rx1 + b2 * rx2 - a1 * ry1 - a2 * ry2;
        
        // Update filter states
        lx2 = lx1; lx1 = originalLeft; ly2 = ly1; ly1 = l_filtered;
        rx2 = rx1; rx1 = originalRight; ry2 = ry1; ry1 = r_filtered;
        
        // Apply different delays based on band number (odd/even)
        int delay_samples = static_cast<int>((delayTimeMs / 1000.0) * sampleRate);
        if (band % 2 == 0) {
            // Even band: add positive delay to left
            int left_delay_pos = (leftWritePos - delay_samples + delayBufferSize) % delayBufferSize;
            left_delay_pos = (left_delay_pos + delayBufferSize) % delayBufferSize;
            
            left += l_filtered * amount * 0.1;  // Add with positive delay to left
        } else {
            // Odd band: add negative delay (advance) to left, or delay to right
            int right_delay_pos = (rightWritePos - delay_samples + delayBufferSize) % delayBufferSize;
            right_delay_pos = (right_delay_pos + delayBufferSize) % delayBufferSize;
            
            right += r_filtered * amount * 0.1;  // Add with delay to right
        }
    }
}

void TubeStereoWidener::processSignal() {
    // Convert to mid-side for processing
    double mid, side;
    stereoToMidSide(leftInputSignal, rightInputSignal, mid, side);
    
    // Apply widening factor to the side signal
    side *= width;
    
    // Different processing based on widener type
    switch (widenerType) {
        case MID_SIDE_WIDENER:
        case PHASE_WIDENER: {
            // Apply delay to the mid signal and recombine
            int delay_samples = static_cast<int>((delayTimeMs / 1000.0) * sampleRate);
            delay_samples = std::min(delay_samples, delayBufferSize - 1);
            
            // Add delayed mid to side in a way that widens the stereo field
            int mid_delay_pos = (leftWritePos - delay_samples + delayBufferSize) % delayBufferSize;
            mid_delay_pos = (mid_delay_pos + delayBufferSize) % delayBufferSize;
            
            // Store input in delay buffer
            leftDelayBuffer[leftWritePos] = mid * midSideRatio + side * (1.0 - midSideRatio);
            rightDelayBuffer[rightWritePos] = side * midSideRatio + mid * (1.0 - midSideRatio);
            
            // Get delayed values
            double delayed_mid = leftDelayBuffer[mid_delay_pos];
            double delayed_side = rightDelayBuffer[(mid_delay_pos + delay_samples/2) % delayBufferSize]; // Different delay for side
            
            // Apply widening based on the amount parameter
            double temp_mid = mid * (1.0 - amount) + delayed_mid * amount;
            double temp_side = side * (1.0 - amount) + delayed_side * amount;
            
            // Convert back to left/right
            midSideToStereo(temp_mid, temp_side, leftOutputSignal, rightOutputSignal);
            
            // Update write positions
            leftWritePos = (leftWritePos + 1) % delayBufferSize;
            rightWritePos = (rightWritePos + 1) % delayBufferSize;
            break;
        }
        
        case BAND_PASS_WIDENER: {
            // Process through frequency bands
            double leftOut = leftInputSignal;
            double rightOut = rightInputSignal;
            processFrequencyBands(leftOut, rightOut);
            
            // Apply the processed signals with widening
            leftOutputSignal = leftInputSignal * (1.0 - amount) + leftOut * amount;
            rightOutputSignal = rightInputSignal * (1.0 - amount) + rightOut * amount;
            
            break;
        }
        
        case ALL_PASS_WIDENER: {
            // Apply all-pass filter based widening
            // This simulates frequency-dependent phase shifts that create stereo widening
            static double apx1 = 0.0, apy1 = 0.0; // All-pass filter state
            double ap_coeff = 0.5; // All-pass coefficient
            
            // Process the side signal with an all-pass filter
            double allpass_output = -ap_coeff * side + apx1 + ap_coeff * apy1;
            apx1 = side;
            apy1 = allpass_output;
            
            // Apply the all-pass processed signal with amount control
            side = side * (1.0 - amount * 0.5) + allpass_output * amount * 0.5;
            
            // Convert back to stereo
            midSideToStereo(mid, side, leftOutputSignal, rightOutputSignal);
            break;
        }
    }
    
    // Apply tube characteristics if enabled
    if (tubeCharacteristicsEnabled) {
        // Apply soft saturation to simulate tube warmth
        double saturation = 0.8 - 0.3 * tubeSaturation; // Adjust saturation based on setting
        if (leftOutputSignal > saturation) {
            leftOutputSignal = saturation + (1 - saturation) * tanh((leftOutputSignal - saturation) / (1 - saturation));
        } else if (leftOutputSignal < -saturation) {
            leftOutputSignal = -saturation + (1 - saturation) * tanh((leftOutputSignal + saturation) / (1 - saturation));
        }
        
        if (rightOutputSignal > saturation) {
            rightOutputSignal = saturation + (1 - saturation) * tanh((rightOutputSignal - saturation) / (1 - saturation));
        } else if (rightOutputSignal < -saturation) {
            rightOutputSignal = -saturation + (1 - saturation) * tanh((rightOutputSignal + saturation) / (1 - saturation));
        }
        
        // Add subtle harmonic content
        double left_harmonic = 0.02 * tubeSaturation * leftOutputSignal * leftOutputSignal * (leftOutputSignal > 0 ? 1 : -1);
        double right_harmonic = 0.02 * tubeSaturation * rightOutputSignal * rightOutputSignal * (rightOutputSignal > 0 ? 1 : -1);
        
        leftOutputSignal = leftOutputSignal * (1.0 - 0.05 * tubeSaturation) + left_harmonic * 0.05 * tubeSaturation;
        rightOutputSignal = rightOutputSignal * (1.0 - 0.05 * tubeSaturation) + right_harmonic * 0.05 * tubeSaturation;
    }
}

void TubeStereoWidener::setAmount(double amount) {
    this->amount = std::max(0.0, std::min(1.0, amount));
}

void TubeStereoWidener::setDelayTime(double time) {
    this->delayTimeMs = std::max(0.0, std::min(10.0, time));
}

void TubeStereoWidener::setWidth(double width) {
    this->width = std::max(0.0, std::min(3.0, width));
}

void TubeStereoWidener::setMidSideRatio(double ratio) {
    this->midSideRatio = std::max(0.0, std::min(1.0, ratio));
}

void TubeStereoWidener::setFrequencyBand(int band, double freq) {
    if (band >= 0 && band < static_cast<int>(bandFrequencies.size())) {
        bandFrequencies[band] = std::max(20.0, std::min(20000.0, freq));
    }
}


// TubeSideMidSplitter implementation
TubeSideMidSplitter::TubeSideMidSplitter(SplitterType type) : splitterType(type) {
    initializeSplitter(type);
}

void TubeSideMidSplitter::initializeSplitter(SplitterType type) {
    switch (type) {
        case PASSIVE_SPLITTER:
            midSideRatio = 0.5;       // Balanced mid/side
            tubeSaturation = 0.0;     // No saturation for passive
            harmonicContent = 0.0;    // No harmonic enhancement for passive
            gain = 0.5;               // No amplification
            break;
            
        case ACTIVE_SPLITTER:
            midSideRatio = 0.5;       // Balanced mid/side
            tubeSaturation = 0.1;     // Low saturation
            harmonicContent = 0.1;    // Low harmonic enhancement
            gain = 1.0;               // Unity gain
            break;
            
        case TUBE_SPLITTER:
            midSideRatio = 0.5;       // Balanced mid/side
            tubeSaturation = 0.3;     // Medium saturation
            harmonicContent = 0.2;    // Medium harmonic enhancement
            gain = 1.1;               // Slight gain for tube character
            break;
            
        case VARIABLE_SPLITTER:
            midSideRatio = 0.5;       // Start balanced
            tubeSaturation = 0.2;     // Medium saturation
            harmonicContent = 0.15;   // Medium harmonic enhancement
            gain = 1.0;               // Unity gain
            break;
    }
}

bool TubeSideMidSplitter::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeSideMidSplitter::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == leftInput && data_bytes == sizeof(double)) {
        memcpy(&leftInputSignal, data, sizeof(double));
        return true;
    } else if (conn_id == rightInput && data_bytes == sizeof(double)) {
        memcpy(&rightInputSignal, data, sizeof(double));
        return true;
    } else if (conn_id == midSideControl && data_bytes == sizeof(double)) {
        memcpy(&midSideControlSignal, data, sizeof(double));
        setMidSideRatio(0.5 + 0.5 * midSideControlSignal); // Map -1,1 to 0,1
        return true;
    }
    return false;
}

bool TubeSideMidSplitter::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == midOutput && data_bytes == sizeof(double)) {
        memcpy(data, &midOutputSignal, sizeof(double));
        return true;
    } else if (conn_id == sideOutput && data_bytes == sizeof(double)) {
        memcpy(data, &sideOutputSignal, sizeof(double));
        return true;
    }
    return false;
}

bool TubeSideMidSplitter::Tick() {
    processSignal();
    return true;
}

double TubeSideMidSplitter::applyTubeCharacteristics(double signal, double tubeSat) {
    if (!tubeCharacteristicsEnabled) {
        return signal;
    }
    
    // Apply soft clipping to simulate tube saturation
    double saturation = 0.7 + 0.3 * (1.0 - tubeSat);  // Lower value = more saturation
    if (signal > saturation) {
        signal = saturation + (1 - saturation) * tanh((signal - saturation) / (1 - saturation));
    } else if (signal < -saturation) {
        signal = -saturation + (1 - saturation) * tanh((signal + saturation) / (1 - saturation));
    }
    
    // Add harmonic content characteristic of tube circuits
    if (harmonicEnhancementEnabled) {
        double harmonic = tubeSat * 0.1 * signal * signal * (signal > 0 ? 1 : -1);
        signal = signal * (1.0 - 0.05 * tubeSat) + harmonic * 0.05 * tubeSat;
    }
    
    return signal;
}

void TubeSideMidSplitter::processSignal() {
    // Convert stereo L/R to mid/side: Mid = (L+R)/2, Side = (L-R)/2
    double mid = (leftInputSignal + rightInputSignal) * 0.5;
    double side = (leftInputSignal - rightInputSignal) * 0.5;
    
    // Apply gain
    mid *= gain;
    side *= gain;
    
    // Apply tube characteristics if enabled
    if (tubeCharacteristicsEnabled) {
        mid = applyTubeCharacteristics(mid, tubeSaturation);
        side = applyTubeCharacteristics(side, tubeSaturation);
    }
    
    // Apply mid/side ratio control
    double ratio = midSideRatio;
    midOutputSignal = mid * ratio + side * (1.0 - ratio);
    sideOutputSignal = side * ratio + mid * (1.0 - ratio);
    
    // Apply harmonic content if enabled
    if (harmonicEnhancementEnabled) {
        double mid_harmonic = harmonicContent * midOutputSignal * midOutputSignal * (midOutputSignal > 0 ? 1 : -1);
        double side_harmonic = harmonicContent * sideOutputSignal * sideOutputSignal * (sideOutputSignal > 0 ? 1 : -1);
        
        midOutputSignal = midOutputSignal * (1.0 - 0.1 * harmonicContent) + mid_harmonic * 0.1 * harmonicContent;
        sideOutputSignal = sideOutputSignal * (1.0 - 0.1 * harmonicContent) + side_harmonic * 0.1 * harmonicContent;
    }
}

void TubeSideMidSplitter::setMidSideRatio(double ratio) {
    this->midSideRatio = std::max(0.0, std::min(1.0, ratio));
}

void TubeSideMidSplitter::setTubeSaturation(double saturation) {
    this->tubeSaturation = std::max(0.0, std::min(1.0, saturation));
}

void TubeSideMidSplitter::setHarmonicContent(double content) {
    this->harmonicContent = std::max(0.0, std::min(1.0, content));
}

void TubeSideMidSplitter::setGain(double gain) {
    this->gain = std::max(0.1, std::min(2.0, gain));
}


// TubeAutoWah implementation
TubeAutoWah::TubeAutoWah(AutoWahType type) : autoWahType(type) {
    initializeAutoWah(type);
}

void TubeAutoWah::initializeAutoWah(AutoWahType type) {
    switch (type) {
        case CONTOUR_FILTER:
            sensitivity = 0.7;        // Medium sensitivity
            attackTime = 0.03;        // Medium attack
            releaseTime = 0.15;       // Medium release
            minFrequency = 200.0;     // Low Q point
            maxFrequency = 1200.0;    // High Q point
            resonance = 4.0;          // Medium resonance
            wetDryMix = 0.9;          // Most effect
            break;
            
        case TRACKING_FILTER:
            sensitivity = 0.6;        // Lower sensitivity for tracking
            attackTime = 0.02;        // Faster attack for tracking
            releaseTime = 0.1;        // Faster release
            minFrequency = 150.0;     // Lower minimum
            maxFrequency = 1800.0;    // Higher maximum
            resonance = 5.0;          // Higher resonance for tracking
            wetDryMix = 0.85;         // Slightly less effect
            break;
            
        case DUAL_FILTER:
            sensitivity = 0.75;       // Higher sensitivity
            attackTime = 0.04;        // Slower attack
            releaseTime = 0.25;       // Slower release
            minFrequency = 180.0;     // Low minimum
            maxFrequency = 1500.0;    // Higher maximum
            resonance = 3.5;          // Medium resonance
            wetDryMix = 0.95;         // More effect
            break;
            
        case RESONANT_WAH:
            sensitivity = 0.65;       // Medium sensitivity
            attackTime = 0.05;        // Slow attack
            releaseTime = 0.3;        // Slow release
            minFrequency = 100.0;     // Very low minimum
            maxFrequency = 2000.0;    // Very high maximum
            resonance = 8.0;          // Very high resonance
            wetDryMix = 0.8;          // Balanced mix
            break;
    }
    
    // Calculate coefficients for envelope follower
    attackCoeff = exp(-1.0 / (attackTime * sampleRate));
    releaseCoeff = exp(-1.0 / (releaseTime * sampleRate));
    
    // Initialize filter with minimum frequency
    updateFilterCoefficients();
}

bool TubeAutoWah::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeAutoWah::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == inputPin && data_bytes == sizeof(double)) {
        memcpy(&inputSignal, data, sizeof(double));
        return true;
    } else if (conn_id == sensitivityPin && data_bytes == sizeof(double)) {
        memcpy(&sensitivityControl, data, sizeof(double));
        setSensitivity(sensitivity + 0.3 * sensitivityControl); // Modulate around current setting
        return true;
    }
    return false;
}

bool TubeAutoWah::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == outputPin && data_bytes == sizeof(double)) {
        memcpy(data, &outputSignal, sizeof(double));
        return true;
    }
    return false;
}

bool TubeAutoWah::Tick() {
    processSignal();
    return true;
}

void TubeAutoWah::updateEnvelope() {
    // Calculate the absolute value of the input for envelope detection
    double inputLevel = std::abs(inputSignal) * sensitivity;
    
    // Update the envelope follower with attack/release characteristics
    if (inputLevel > envelope) {
        // Attack (follow input quickly)
        envelope = inputLevel * (1.0 - attackCoeff) + envelope * attackCoeff;
    } else {
        // Release (fall back slowly)
        envelope = envelope * releaseCoeff;
    }
    
    // Ensure envelope doesn't go below minimum value
    if (envelope < 0.001) envelope = 0.001;
}

void TubeAutoWah::updateFilterCoefficients() {
    // Calculate the current filter frequency based on the envelope
    // Map envelope (0.0 to 1.0) to frequency range (minFrequency to maxFrequency)
    double normalizedEnvelope = std::min(1.0, std::max(0.0, envelope));
    double currentFreq = minFrequency * pow(maxFrequency/minFrequency, normalizedEnvelope);
    
    // Ensure frequency is within valid range
    currentFreq = std::max(minFrequency, std::min(maxFrequency, currentFreq));
    
    // Calculate coefficients for a resonant low-pass filter (biquad)
    double omega = 2.0 * M_PI * currentFreq / sampleRate;
    double sin_omega = sin(omega);
    double cos_omega = cos(omega);
    double alpha = sin_omega / (2.0 * resonance);  // Alpha with resonance
    
    // Low-pass filter coefficients
    b0 = (1.0 - cos_omega) / 2.0;
    b1 = 1.0 - cos_omega;
    b2 = (1.0 - cos_omega) / 2.0;
    a0 = 1.0 + alpha;
    a1 = -2.0 * cos_omega;
    a2 = 1.0 - alpha;
    
    // Normalize coefficients
    b0 /= a0;
    b1 /= a0;
    b2 /= a0;
    a1 /= a0;
    a2 /= a0;
    // a0 is now effectively 1.0
}

double TubeAutoWah::processFilter(double input) {
    // Direct Form II implementation of the biquad filter
    double output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    
    // Update filter state
    x2 = x1;
    x1 = input;
    y2 = y1;
    y1 = output;
    
    return output;
}

void TubeAutoWah::processSignal() {
    // Update the envelope follower
    updateEnvelope();
    
    // Update filter coefficients based on current envelope
    updateFilterCoefficients();
    
    // Process the input through the filter
    double filteredSignal = processFilter(inputSignal);
    
    // Apply tube characteristics if enabled
    if (tubeCharacteristicsEnabled) {
        // Add harmonic content based on the filter's resonance
        double harmonic = harmonicContent * filteredSignal * filteredSignal * (filteredSignal > 0 ? 1 : -1);
        filteredSignal = filteredSignal * (1.0 - 0.1 * harmonicContent) + harmonic * 0.1 * harmonicContent;
        
        // Apply soft saturation to simulate tube warmth
        double saturation = 0.8 - 0.2 * harmonicContent; // Adjust based on harmonic content
        if (filteredSignal > saturation) {
            filteredSignal = saturation + (1 - saturation) * tanh((filteredSignal - saturation) / (1 - saturation));
        } else if (filteredSignal < -saturation) {
            filteredSignal = -saturation + (1 - saturation) * tanh((filteredSignal + saturation) / (1 - saturation));
        }
    }
    
    // Mix with original signal
    outputSignal = inputSignal * (1.0 - wetDryMix) + filteredSignal * wetDryMix;
}

void TubeAutoWah::setSensitivity(double sensitivity) {
    this->sensitivity = std::max(0.0, std::min(1.0, sensitivity));
}

void TubeAutoWah::setAttackTime(double time) {
    this->attackTime = std::max(0.001, std::min(0.5, time));
    attackCoeff = exp(-1.0 / (attackTime * sampleRate));
}

void TubeAutoWah::setReleaseTime(double time) {
    this->releaseTime = std::max(0.01, std::min(1.0, time));
    releaseCoeff = exp(-1.0 / (releaseTime * sampleRate));
}

void TubeAutoWah::setMinFrequency(double freq) {
    this->minFrequency = std::max(20.0, std::min(500.0, freq));
    // Ensure minFrequency is less than maxFrequency
    if (minFrequency >= maxFrequency) {
        maxFrequency = minFrequency * 2.0;
    }
}

void TubeAutoWah::setMaxFrequency(double freq) {
    this->maxFrequency = std::max(1000.0, std::min(20000.0, freq));
    // Ensure maxFrequency is greater than minFrequency
    if (maxFrequency <= minFrequency) {
        minFrequency = maxFrequency / 2.0;
    }
}

void TubeAutoWah::setResonance(double res) {
    this->resonance = std::max(0.1, std::min(20.0, res));
}

void TubeAutoWah::setWetDryMix(double mix) {
    this->wetDryMix = std::max(0.0, std::min(1.0, mix));
}


// TubeMoogFilter implementation
TubeMoogFilter::TubeMoogFilter(FilterType type) : filterType(type) {
    initializeFilter(type);
}

void TubeMoogFilter::initializeFilter(FilterType type) {
    filterType = type;
    
    switch (filterType) {
        case LOW_PASS:
            cutoffFreq = 1000.0;
            resonance = 0.6;
            drive = 1.2;
            stability = 0.8;
            break;
            
        case HIGH_PASS:
            cutoffFreq = 500.0;
            resonance = 0.5;
            drive = 1.0;
            stability = 0.7;
            break;
            
        case BAND_PASS:
            cutoffFreq = 1000.0;
            resonance = 0.7;
            drive = 1.3;
            stability = 0.75;
            break;
            
        case BAND_REJECT:
            cutoffFreq = 800.0;
            resonance = 0.4;
            drive = 1.1;
            stability = 0.85;
            break;
            
        case ALL_PASS:
            cutoffFreq = 1000.0;
            resonance = 0.3;
            drive = 1.0;
            stability = 0.9;
            break;
    }
}

bool TubeMoogFilter::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeMoogFilter::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == inputPin && data_bytes == sizeof(double)) {
        memcpy(&inputSignal, data, sizeof(double));
        return true;
    } else if (conn_id == cutoffPin && data_bytes == sizeof(double)) {
        memcpy(&cutoffControl, data, sizeof(double));
        setCutoff(cutoffFreq * (1.0 + 0.5 * cutoffControl)); // Modulate around current setting
        return true;
    } else if (conn_id == resonancePin && data_bytes == sizeof(double)) {
        memcpy(&resonanceControl, data, sizeof(double));
        setResonance(resonance + 0.4 * resonanceControl); // Modulate around current setting
        return true;
    }
    return false;
}

bool TubeMoogFilter::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == outputPin && data_bytes == sizeof(double)) {
        memcpy(data, &outputSignal, sizeof(double));
        return true;
    }
    return false;
}

bool TubeMoogFilter::Tick() {
    processSignal();
    return true;
}

double TubeMoogFilter::processMoogFilter(double input) {
    // Apply drive/gain to input
    double processedInput = input * drive;
    
    // Calculate filter coefficients based on cutoff and resonance
    // Using the standard Moog ladder filter approach with non-linear stages
    double f = 2.0 * sin(M_PI * std::max(20.0, std::min(10000.0, cutoffFreq)) / sampleRate);
    double fb = resonance * (1.0 - 0.15 * f * f);  // Feedback amount with stability
    
    // Apply the 4-stage ladder filter with non-linear saturation
    // Stage 1
    inputLP = processedInput - fb * lastOutput;
    stage1 = f * tanh(inputLP) + (1 - f) * stage1;  // Non-linear input stage
    
    // Stage 2
    stage2 = f * tanh(stage1) + (1 - f) * stage2;
    
    // Stage 3
    stage3 = f * tanh(stage2) + (1 - f) * stage3;
    
    // Stage 4
    stage4 = f * tanh(stage3) + (1 - f) * stage4;
    
    // Apply final saturation and return
    lastOutput = 2.0 * stage4;  // 2x gain to compensate for losses
    
    // Apply stability control to prevent oscillation
    lastOutput *= stability;
    
    // Return based on filter type
    switch (filterType) {
        case LOW_PASS:
            return lastOutput;
            
        case HIGH_PASS: {
            // High-pass is input minus low-pass
            return input - lastOutput;
        }
        
        case BAND_PASS: {
            // Band-pass is difference between two low-pass filters at different frequencies
            // This is a simplified approximation
            double f2 = 2.0 * sin(M_PI * std::max(20.0, std::min(10000.0, cutoffFreq * 0.7)) / sampleRate);
            double fb2 = resonance * 0.8 * (1.0 - 0.15 * f2 * f2);
            double inputLP2 = processedInput - fb2 * lastOutput * 0.8;
            double stage1_2 = f2 * tanh(inputLP2) + (1 - f2) * stage1;  // Non-linear input stage
            double stage2_2 = f2 * tanh(stage1_2) + (1 - f2) * stage2;
            double stage3_2 = f2 * tanh(stage2_2) + (1 - f2) * stage3;
            double stage4_2 = f2 * tanh(stage3_2) + (1 - f2) * stage4;
            double hpOutput = 2.0 * stage4_2 * stability * 0.9;
            return lastOutput - hpOutput;  // Difference gives bandpass
        }
        
        case BAND_REJECT: {
            // Band-reject is input minus bandpass
            double f2 = 2.0 * sin(M_PI * std::max(20.0, std::min(10000.0, cutoffFreq * 0.7)) / sampleRate);
            double fb2 = resonance * 0.8 * (1.0 - 0.15 * f2 * f2);
            double inputLP2 = processedInput - fb2 * lastOutput * 0.8;
            double stage1_2 = f2 * tanh(inputLP2) + (1 - f2) * stage1;  // Non-linear input stage
            double stage2_2 = f2 * tanh(stage1_2) + (1 - f2) * stage2;
            double stage3_2 = f2 * tanh(stage2_2) + (1 - f2) * stage3;
            double stage4_2 = f2 * tanh(stage3_2) + (1 - f2) * stage4;
            double hpOutput = 2.0 * stage4_2 * stability * 0.9;
            double bandPass = lastOutput - hpOutput;
            return input - bandPass;
        }
        
        case ALL_PASS: {
            // All-pass maintains same amplitude but changes phase
            // This is a simplified implementation
            return input - 2.0 * lastOutput;
        }
        
        default:
            return lastOutput;
    }
}

void TubeMoogFilter::processSignal() {
    // Process the input through the Moog filter
    outputSignal = processMoogFilter(inputSignal);
    
    // Apply tube characteristics if enabled
    if (tubeCharacteristicsEnabled) {
        // Apply soft saturation to simulate tube warmth
        double saturation = 0.7 + 0.3 * (1.0 - tubeSaturation); // Lower value = more saturation
        
        if (outputSignal > saturation) {
            outputSignal = saturation + (1 - saturation) * tanh((outputSignal - saturation) / (1 - saturation));
        } else if (outputSignal < -saturation) {
            outputSignal = -saturation + (1 - saturation) * tanh((outputSignal + saturation) / (1 - saturation));
        }
        
        // Add harmonic content characteristic of tube circuits
        double harmonic = tubeSaturation * 0.05 * outputSignal * outputSignal * (outputSignal > 0 ? 1 : -1);
        outputSignal = outputSignal * (1.0 - 0.02 * tubeSaturation) + harmonic * 0.02 * tubeSaturation;
    }
}

void TubeMoogFilter::setCutoff(double freq) {
    this->cutoffFreq = std::max(20.0, std::min(20000.0, freq));
}

void TubeMoogFilter::setResonance(double res) {
    this->resonance = std::max(0.0, std::min(0.99, res));
}

void TubeMoogFilter::setDrive(double drive) {
    this->drive = std::max(1.0, std::min(20.0, drive));
}

void TubeMoogFilter::setType(FilterType type) {
    this->filterType = type;
    initializeFilter(type);
}

void TubeMoogFilter::setStability(double stability) {
    this->stability = std::max(0.1, std::min(1.0, stability));
}


// TubeOctave implementation
TubeOctave::TubeOctave(OctaveType type) : octaveType(type) {
    initializeOctave(type);
    
    // Initialize pitch detection buffer
    pitchBuffer.resize(pitchBufferSize, 0.0);
}

void TubeOctave::initializeOctave(OctaveType type) {
    octaveType = type;
    
    switch (octaveType) {
        case SUB_OCTAVE:
            subOctaveAmount = 0.8;
            superOctaveAmount = 0.0;
            dryWetMix = 0.5;
            tracking = 0.85;
            distortion = 0.1;
            break;
            
        case SUPER_OCTAVE:
            subOctaveAmount = 0.0;
            superOctaveAmount = 0.7;
            dryWetMix = 0.4;
            tracking = 0.7;
            distortion = 0.15;
            break;
            
        case DUAL_OCTAVE:
            subOctaveAmount = 0.6;
            superOctaveAmount = 0.4;
            dryWetMix = 0.6;
            tracking = 0.8;
            distortion = 0.2;
            break;
            
        case OCTAVE_FUZZ:
            subOctaveAmount = 0.7;
            superOctaveAmount = 0.2;
            dryWetMix = 0.7;
            tracking = 0.75;
            distortion = 0.6;  // Higher distortion for fuzz effect
            break;
    }
}

bool TubeOctave::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeOctave::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == inputPin && data_bytes == sizeof(double)) {
        memcpy(&inputSignal, data, sizeof(double));
        return true;
    } else if (conn_id == mixPin && data_bytes == sizeof(double)) {
        memcpy(&mixControl, data, sizeof(double));
        setDryWetMix(0.5 + 0.5 * mixControl); // Map -1,1 to 0,1
        return true;
    } else if (conn_id == octavePin && data_bytes == sizeof(double)) {
        memcpy(&octaveControl, data, sizeof(double));
        setSubOctaveAmount(subOctaveAmount + 0.3 * octaveControl); // Modulate octave amount
        return true;
    }
    return false;
}

bool TubeOctave::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == outputPin && data_bytes == sizeof(double)) {
        memcpy(data, &outputSignal, sizeof(double));
        return true;
    }
    return false;
}

bool TubeOctave::Tick() {
    processSignal();
    return true;
}

double TubeOctave::estimateFrequency() {
    // Store current sample in buffer
    pitchBuffer[pitchWritePos] = inputSignal;
    pitchWritePos = (pitchWritePos + 1) % pitchBufferSize;
    
    // Simple zero-crossing based frequency estimation
    bool currentSign = inputSignal >= 0;
    bool lastSign = lastInput >= 0;
    
    double estimatedFreq = estimatedFrequency;
    
    // If we've crossed zero (changed sign)
    if (currentSign != lastSign) {
        if (zeroCrossings > 0) {
            // Calculate period based on samples since last zero crossing
            double period = sampleCounter / zeroCrossings;
            estimatedFreq = sampleRate / (period * 2.0);  // Factor of 2 since we count each full cycle twice
            
            // Apply smoothing to frequency estimate
            estimatedFrequency = 0.7 * estimatedFrequency + 0.3 * estimatedFreq;
            
            // Clamp to reasonable range
            estimatedFrequency = std::max(50.0, std::min(5000.0, estimatedFrequency));
        }
        
        // Reset counter and crossing count
        sampleCounter = 0.0;
        zeroCrossings = 1;
    } else {
        // Increment sample counter
        sampleCounter++;
    }
    
    // Update last input
    lastInput = inputSignal;
    
    return estimatedFrequency;
}

double TubeOctave::generateSubOctave() {
    // Update phase based on half the detected frequency (one octave down)
    phase += 2.0 * M_PI * estimatedFrequency / sampleRate / 2.0;  // Divide by 2 for sub-octave
    if (phase >= 2.0 * M_PI) {
        phase -= 2.0 * M_PI;
    }
    
    // Generate a square wave at the sub-octave frequency
    // This is a simple approach - more sophisticated implementations could use different methods
    double subOctaveSignal = (sin(phase) > 0) ? 1.0 : -1.0;
    
    // Scale based on input signal amplitude
    subOctaveSignal *= std::abs(inputSignal) * tracking;
    
    return subOctaveSignal;
}

double TubeOctave::generateSuperOctave() {
    // For super-octave, we double the frequency
    // Note: This is a simplified approach - real octave-up effects are much more complex
    phase += 2.0 * M_PI * estimatedFrequency / sampleRate * 2.0;  // Multiply by 2 for super-octave
    if (phase >= 2.0 * M_PI) {
        phase -= 2.0 * M_PI;
    }
    
    // Generate a signal at double the frequency
    double superOctaveSignal = sin(phase) * 0.7;  // Reduce amplitude
    
    // Enhance harmonics to simulate octave-up effect
    // This is a simplified approach to generate higher harmonics
    double inputHarmonics = inputSignal * inputSignal * (inputSignal > 0 ? 1 : -1) * 0.3;
    superOctaveSignal += inputHarmonics;
    
    return superOctaveSignal;
}

void TubeOctave::applyOctaveProcessing() {
    // Estimate input frequency
    estimateFrequency();
    
    double subOctave = 0.0;
    double superOctave = 0.0;
    
    // Generate octave signals based on current settings
    if (subOctaveAmount > 0.0) {
        subOctave = generateSubOctave() * subOctaveAmount;
    }
    
    if (superOctaveAmount > 0.0) {
        superOctave = generateSuperOctave() * superOctaveAmount;
    }
    
    // Combine octave signals
    double octaveSignal = subOctave + superOctave;
    
    // Apply to input based on dry/wet mix
    outputSignal = inputSignal * (1.0 - dryWetMix) + octaveSignal * dryWetMix;
    
    // Apply tube characteristics if enabled
    if (tubeCharacteristicsEnabled) {
        // Apply distortion based on setting
        double tubeDistortion = distortion * 0.5;
        if (std::abs(outputSignal) > tubeDistortion) {
            outputSignal = tubeDistortion * (outputSignal > 0 ? 1 : -1) + 
                          (1 - tubeDistortion) * tanh(outputSignal / (1 - tubeDistortion));
        }
        
        // Add tube warmth (even-order harmonics)
        double warmthSignal = tubeWarmth * 0.1 * outputSignal * outputSignal * (outputSignal > 0 ? 1 : -1);
        outputSignal = outputSignal * (1.0 - 0.05 * tubeWarmth) + warmthSignal * 0.05 * tubeWarmth;
    }
}

void TubeOctave::processSignal() {
    applyOctaveProcessing();
}

void TubeOctave::setDryWetMix(double mix) {
    this->dryWetMix = std::max(0.0, std::min(1.0, mix));
}

void TubeOctave::setSubOctaveAmount(double amount) {
    this->subOctaveAmount = std::max(0.0, std::min(1.0, amount));
}

void TubeOctave::setSuperOctaveAmount(double amount) {
    this->superOctaveAmount = std::max(0.0, std::min(1.0, amount));
}

void TubeOctave::setTracking(double tracking) {
    this->tracking = std::max(0.0, std::min(1.0, tracking));
}

void TubeOctave::setType(OctaveType type) {
    this->octaveType = type;
    initializeOctave(type);
}

void TubeOctave::setDistortion(double distortion) {
    this->distortion = std::max(0.0, std::min(1.0, distortion));
}


// TubeRingModulator implementation
TubeRingModulator::TubeRingModulator(ModulationType type) : modulationType(type) {
    initializeModulator(type);
}

void TubeRingModulator::initializeModulator(ModulationType type) {
    modulationType = type;
    
    switch (modulationType) {
        case SINE_MODULATION:
            carrierFrequency = 150.0;
            depth = 1.0;
            symmetry = 0.5;
            tubeCharacteristics = 0.2;
            carrierDistortion = 0.0;
            break;
            
        case TRIANGLE_MODULATION:
            carrierFrequency = 80.0;
            depth = 0.9;
            symmetry = 0.5;
            tubeCharacteristics = 0.3;
            carrierDistortion = 0.1;
            break;
            
        case SQUARE_MODULATION:
            carrierFrequency = 120.0;
            depth = 1.0;
            symmetry = 0.5;
            tubeCharacteristics = 0.4;
            carrierDistortion = 0.2;
            break;
            
        case SAWTOOTH_MODULATION:
            carrierFrequency = 90.0;
            depth = 0.85;
            symmetry = 0.5;
            tubeCharacteristics = 0.35;
            carrierDistortion = 0.15;
            break;
            
        case TUBE_CARRIER:
            carrierFrequency = 200.0;
            depth = 1.0;
            symmetry = 0.6;
            tubeCharacteristics = 0.6;
            carrierDistortion = 0.4;
            break;
    }
    
    // Calculate initial phase increment
    carrierPhaseInc = 2.0 * M_PI * carrierFrequency / sampleRate;
}

bool TubeRingModulator::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeRingModulator::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == inputPin && data_bytes == sizeof(double)) {
        memcpy(&inputSignal, data, sizeof(double));
        return true;
    } else if (conn_id == carrierFreqPin && data_bytes == sizeof(double)) {
        memcpy(&carrierFreqControl, data, sizeof(double));
        setCarrierFrequency(carrierFrequency * (1.0 + 0.5 * carrierFreqControl)); // Modulate frequency
        return true;
    } else if (conn_id == depthPin && data_bytes == sizeof(double)) {
        memcpy(&depthControl, data, sizeof(double));
        setDepth(depth + 0.3 * depthControl); // Modulate depth
        return true;
    }
    return false;
}

bool TubeRingModulator::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == outputPin && data_bytes == sizeof(double)) {
        memcpy(data, &outputSignal, sizeof(double));
        return true;
    }
    return false;
}

bool TubeRingModulator::Tick() {
    processSignal();
    return true;
}

double TubeRingModulator::generateCarrier() {
    // Update the carrier phase
    carrierPhase += carrierPhaseInc;
    if (carrierPhase >= 2.0 * M_PI) {
        carrierPhase -= 2.0 * M_PI;
    }
    
    double carrier = 0.0;
    
    // Generate carrier based on type
    switch (modulationType) {
        case SINE_MODULATION:
            carrier = sin(carrierPhase);
            break;
            
        case TRIANGLE_MODULATION: {
            // Generate triangle wave
            if (carrierPhase < M_PI) {
                carrier = (2.0 / M_PI) * carrierPhase - 1.0;
            } else {
                carrier = -1.0 - (2.0 / M_PI) * (carrierPhase - M_PI);
            }
            break;
        }
            
        case SQUARE_MODULATION: {
            // Generate square wave with adjustable symmetry
            double sym = symmetry; // Adjust symmetry
            if (carrierPhase < 2 * M_PI * sym) {
                carrier = 1.0;
            } else {
                carrier = -1.0;
            }
            break;
        }
            
        case SAWTOOTH_MODULATION: {
            // Generate sawtooth wave
            carrier = (carrierPhase / M_PI) - 1.0;
            break;
        }
            
        case TUBE_CARRIER: {
            // Generate a more complex carrier that simulates tube harmonics
            carrier = sin(carrierPhase);
            // Add 2nd harmonic (even-order, characteristic of tube circuits)
            carrier += 0.3 * tubeCharacteristics * sin(2.0 * carrierPhase);
            // Add 3rd harmonic
            carrier += 0.15 * tubeCharacteristics * sin(3.0 * carrierPhase);
            // Normalize
            carrier *= 1.0 / (1.0 + 0.3 * tubeCharacteristics + 0.15 * tubeCharacteristics);
            break;
        }
    }
    
    // Apply distortion to carrier if enabled
    if (carrierDistortion > 0.0 && tubeCarrierEnabled) {
        double dist = carrierDistortion;
        if (std::abs(carrier) > (1.0 - dist)) {
            carrier = (1.0 - dist) * (carrier > 0 ? 1 : -1) + 
                     dist * tanh(carrier / dist);
        }
    }
    
    return carrier;
}

double TubeRingModulator::applyRingModulation(double input, double carrier) {
    // Standard ring modulation is simply input * carrier
    double modulated = input * carrier * depth;
    
    // Add tube characteristics if enabled
    if (tubeCharacteristics > 0.0) {
        // Add some even-order harmonics to simulate tube warmth
        double harmonic1 = tubeCharacteristics * 0.1 * modulated * modulated * (modulated > 0 ? 1 : -1);
        double harmonic2 = tubeCharacteristics * 0.05 * modulated * modulated * modulated;
        
        modulated = modulated * (1.0 - 0.075 * tubeCharacteristics) + 
                   harmonic1 * 0.05 * tubeCharacteristics + 
                   harmonic2 * 0.025 * tubeCharacteristics;
    }
    
    return modulated;
}

void TubeRingModulator::processSignal() {
    // Generate the carrier signal
    double carrier = generateCarrier();
    
    // Apply ring modulation to the input
    outputSignal = applyRingModulation(inputSignal, carrier);
    
    // Apply soft limiting to prevent clipping
    if (outputSignal > 0.9) outputSignal = 0.9 + 0.1 * tanh((outputSignal - 0.9) / 0.1);
    if (outputSignal < -0.9) outputSignal = -0.9 + 0.1 * tanh((outputSignal + 0.9) / 0.1);
}

void TubeRingModulator::setCarrierFrequency(double freq) {
    this->carrierFrequency = std::max(0.1, std::min(2000.0, freq));
    // Update phase increment based on new frequency
    carrierPhaseInc = 2.0 * M_PI * carrierFrequency / sampleRate;
}

void TubeRingModulator::setModulationType(ModulationType type) {
    this->modulationType = type;
    initializeModulator(type);
}

void TubeRingModulator::setDepth(double depth) {
    this->depth = std::max(0.0, std::min(1.0, depth));
}

void TubeRingModulator::setSymmetry(double symmetry) {
    this->symmetry = std::max(0.0, std::min(1.0, symmetry));
}

void TubeRingModulator::setTubeCharacteristics(double tube) {
    this->tubeCharacteristics = std::max(0.0, std::min(1.0, tube));
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