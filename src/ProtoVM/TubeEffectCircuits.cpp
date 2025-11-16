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