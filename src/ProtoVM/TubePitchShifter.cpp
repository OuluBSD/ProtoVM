#include "TubePitchShifter.h"
#include "Common.h"
#include <algorithm>
#include <cmath>

// TubePitchShifter implementation
TubePitchShifter::TubePitchShifter(PitchShifterType type) : shifterType(type) {
    InitializePitchShifter(type);

    // Initialize processing buffers
    inputBuffer.resize(windowSize, 0.0);
    outputBuffer.resize(windowSize, 0.0);
    delayBuffer.resize(windowSize * 4, 0.0);  // Larger delay buffer for pitch shifting
    phaseBuffer.resize(windowSize, 0.0);
    
    // Initialize with tubes for pitch shifting simulation
    for (int i = 0; i < 2; i++) {
        tubes.push_back(std::make_unique<Triode>(100.0, 100000.0, 1.6e-3));  // Tube simulation
    }
    
    // Initialize harmonics vector
    harmonics.resize(16, 0.0);  // 16 harmonics should be sufficient

    // Calculate initial phase increment based on pitch shift
    UpdatePhaseIncrement();
}

void TubePitchShifter::InitializePitchShifter(PitchShifterType type) {
    switch (type) {
        case MONO_PITCH_SHIFTER:
            pitchShiftSemitones = 0.0;     // No shift initially
            formantPreservation = true;    // Preserve formants by default
            dryWetBlend = 0.7;             // 70% wet signal
            feedback = 0.1;                // Low feedback
            harmonicMix = 0.1;             // Low harmonic content
            windowSize = 1024;             // Standard window size
            tubeGain = 20.0;               // Moderate tube gain
            break;

        case STEREO_PITCH_SHIFTER:
            pitchShiftSemitones = 0.0;     // No shift initially
            formantPreservation = true;    // Preserve formants
            dryWetBlend = 0.5;             // 50% wet signal (balanced)
            feedback = 0.05;               // Very low feedback for stereo
            harmonicMix = 0.15;            // Medium harmonic content
            windowSize = 2048;             // Larger window for stereo processing
            tubeGain = 22.0;               // Higher tube gain for stereo
            break;

        case HARMONIC_GENERATOR:
            pitchShiftSemitones = 0.0;     // No pitch shift
            formantPreservation = false;   // Don't preserve formants for harmonics
            dryWetBlend = 0.3;             // 30% dry, 70% harmonics
            feedback = 0.0;                // No feedback for harmonic generation
            harmonicMix = 0.8;             // High harmonic content
            windowSize = 512;              // Smaller window for real-time harmonic gen
            tubeGain = 25.0;               // High tube gain for harmonic richness
            harmonicGenerationEnabled = true;
            break;

        case PITCH_CORRECTOR:
            pitchShiftSemitones = 0.0;     // No initial shift
            formantPreservation = true;    // Preserve formants during correction
            dryWetBlend = 0.9;             // Mostly corrected signal
            feedback = 0.0;                // No feedback for correction
            harmonicMix = 0.05;            // Low harmonic content
            windowSize = 1024;             // Standard window for correction
            tubeGain = 18.0;               // Lower tube gain for correction
            correctionStrength = 0.7;      // Strong correction
            tuneToNote = 60;               // Tune to middle C (C4)
            break;
    }
}

bool TubePitchShifter::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubePitchShifter::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == inputPin && data_bytes == sizeof(double)) {
        memcpy(&inputSignal, data, sizeof(double));
        return true;
    } else if (conn_id == controlPin && data_bytes == sizeof(double)) {
        memcpy(&controlSignal, data, sizeof(double));
        // Control signal can affect pitch shift amount
        SetPitchShift(pitchShiftSemitones + 12.0 * controlSignal);  // Map -1,1 to ±12 semitones
        return true;
    } else if (conn_id == feedbackPin && data_bytes == sizeof(double)) {
        memcpy(&feedbackSignal, data, sizeof(double));
        return true;
    }
    return false;
}

bool TubePitchShifter::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == outputPin && data_bytes == sizeof(double)) {
        memcpy(data, &outputSignal, sizeof(double));
        return true;
    }
    return false;
}

bool TubePitchShifter::Tick() {
    ProcessSignal();
    return true;
}

void TubePitchShifter::UpdatePhaseIncrement() {
    // Calculate phase increment based on desired pitch shift
    // 12 semitones = 1 octave = 2x frequency
    targetPhaseIncrement = pow(2.0, pitchShiftSemitones / 12.0);
    
    // Apply correction strength for pitch corrector mode
    if (shifterType == PITCH_CORRECTOR) {
        // Calculate how much to adjust toward the target note
        double currentNote = 69.0 + 12.0 * log2(fundamentalFreq / 440.0);
        double noteDifference = tuneToNote - currentNote;
        
        // Apply correction based on strength
        double correctionSemitones = noteDifference * correctionStrength;
        targetPhaseIncrement = pow(2.0, correctionSemitones / 12.0);
    }
}

double TubePitchShifter::EstimateFundamentalFrequency() {
    // Simple autocorrelation-based frequency estimation
    // This is a simplified approach - a real implementation would be more sophisticated
    
    // Find the period in the input buffer using a simple autocorrelation
    int maxLag = static_cast<int>(sampleRate / 50);  // Minimum frequency of 50Hz
    int minLag = static_cast<int>(sampleRate / 2000); // Maximum frequency of 2000Hz
    
    double bestCorrelation = 0.0;
    int bestLag = minLag;
    
    double signalPower = 0.0;
    for (int i = 0; i < windowSize && i < inputBuffer.size(); i++) {
        signalPower += inputBuffer[i] * inputBuffer[i];
    }
    
    if (signalPower < 0.0001) {
        return 440.0;  // Return default if signal is too quiet
    }
    
    for (int lag = minLag; lag < maxLag && lag < inputBuffer.size(); lag++) {
        double correlation = 0.0;
        int samplesToCorrelate = std::min(windowSize - lag, static_cast<int>(inputBuffer.size() - lag));
        
        for (int i = 0; i < samplesToCorrelate; i++) {
            correlation += inputBuffer[i] * inputBuffer[i + lag];
        }
        
        // Normalize by the power
        correlation /= (sqrt(signalPower) * sqrt(signalPower) + 0.0001);
        
        if (correlation > bestCorrelation) {
            bestCorrelation = correlation;
            bestLag = lag;
        }
    }
    
    return sampleRate / bestLag;
}

double TubePitchShifter::ApplyPitchShift() {
    // Simple pitch shifting using delay-based approach
    // In a real implementation, this would use a more sophisticated algorithm
    // like phase vocoder or granular synthesis
    
    // Update the delay write position
    delayWritePos = (delayWritePos + 1) % delayBuffer.size();
    delayBuffer[delayWritePos] = inputSignal;
    
    // Calculate read position based on phase increment
    int readPos = static_cast<int>(currentPhase);
    double fractional = currentPhase - readPos;
    
    // Wrap around if needed
    readPos = readPos % delayBuffer.size();
    if (readPos < 0) readPos += delayBuffer.size();
    
    // Linear interpolation for smooth output
    int nextPos = (readPos + 1) % delayBuffer.size();
    double output = delayBuffer[readPos] * (1.0 - fractional) + delayBuffer[nextPos] * fractional;
    
    // Update phase for next sample
    currentPhase += phaseIncrement;
    if (currentPhase >= delayBuffer.size()) {
        currentPhase -= delayBuffer.size();
    } else if (currentPhase < 0) {
        currentPhase += delayBuffer.size();
    }
    
    return output;
}

void TubePitchShifter::GenerateHarmonics() {
    // Generate harmonics based on fundamental frequency
    // This is a simplified harmonic generation
    for (int i = 0; i < harmonics.size(); i++) {
        // Harmonics get progressively weaker
        harmonics[i] = 1.0 / (i + 1);
    }
}

void TubePitchShifter::ProcessSignal() {
    // Add input to the buffer
    inputBuffer[bufferWritePos] = inputSignal;
    bufferWritePos = (bufferWritePos + 1) % windowSize;
    
    // Estimate fundamental frequency periodically
    if (bufferWritePos == 0) {  // Every window
        fundamentalFreq = EstimateFundamentalFrequency();
        UpdatePhaseIncrement();
    }
    
    // Apply pitch shifting
    double shiftedSignal = ApplyPitchShift();
    
    // Apply feedback if enabled
    if (feedback > 0.0) {
        shiftedSignal = shiftedSignal + feedbackSignal * feedback;
    }
    
    // Apply harmonic generation if enabled
    if (harmonicGenerationEnabled) {
        GenerateHarmonics();
        
        // Add harmonics to the signal
        double harmonicSignal = 0.0;
        for (int i = 0; i < harmonics.size(); i++) {
            // This is a simplified approach - real harmonic generation would be more complex
            double harmonicFreq = fundamentalFreq * (i + 1);
            double harmonic = sin(currentPhase * 2.0 * M_PI * harmonicFreq / sampleRate) * harmonics[i];
            harmonicSignal += harmonic * 0.1;  // Scale harmonic content
        }
        
        shiftedSignal = shiftedSignal * (1.0 - harmonicMix) + harmonicSignal * harmonicMix;
    }
    
    // Apply tube characteristics
    double processedSignal = shiftedSignal;
    if (tubeCharacteristicsEnabled && !tubes.empty()) {
        auto& tube = tubes[0];
        tube->SetGridVoltage(-0.5 + processedSignal * 0.1);  // Apply signal to grid
        tube->SetPlateVoltage(250.0);  // Set plate voltage
        tube->SetCathodeVoltage(0.0);
        tube->CalculateTubeBehavior();

        // Use the tube's plate current to modify the signal
        double plateCurrent = tube->GetPlateCurrent();
        double tubeFactor = 1.0 + 0.05 * plateCurrent * 0.001;  // Subtle tube effect
        processedSignal = processedSignal * tubeFactor;
        
        // Add subtle harmonic content characteristic of tubes
        processedSignal += 0.01 * processedSignal * processedSignal * (processedSignal > 0 ? 1 : -1);
    }
    
    // Apply dry/wet mix
    outputSignal = inputSignal * (1.0 - dryWetBlend) + processedSignal * dryWetBlend;
    
    // Apply formant preservation if enabled (simplified implementation)
    if (formantPreservation) {
        // This would involve more complex processing in a real implementation
        // For now, just apply a slight EQ to maintain tonal balance
        static double bassBoost = 0.0, trebleCut = 0.0;
        double adjustedOutput = outputSignal * 1.05;  // Slight bass boost
        
        // Apply the adjustment
        outputSignal = adjustedOutput;
    }
    
    // Apply final limiting to prevent clipping
    if (outputSignal > 5.0) outputSignal = 5.0;
    if (outputSignal < -5.0) outputSignal = -5.0;
}

void TubePitchShifter::ApplyTubeCharacteristics() {
    // This is called by ProcessSignal but the tube characteristics are already applied there
    // This function is just for the interface consistency
}

void TubePitchShifter::SetPitchShift(double semitones) {
    this->pitchShiftSemitones = std::max(-24.0, std::min(24.0, semitones));
    UpdatePhaseIncrement();
}

void TubePitchShifter::SetFormantPreservation(bool preserve) {
    this->formantPreservation = preserve;
}

void TubePitchShifter::SetBlend(double dryWet) {
    this->dryWetBlend = std::max(0.0, std::min(1.0, dryWet));
}

void TubePitchShifter::SetFeedback(double feedback) {
    this->feedback = std::max(0.0, std::min(0.95, feedback));
}

void TubePitchShifter::SetHarmonicMix(double mix) {
    this->harmonicMix = std::max(0.0, std::min(1.0, mix));
}

void TubePitchShifter::SetOctaveDivision(int divisions) {
    this->octaveDivision = std::max(1, std::min(4, divisions));
}

void TubePitchShifter::SetTuneToNote(int note) {
    this->tuneToNote = std::max(0, std::min(127, note));  // MIDI note range
}

void TubePitchShifter::SetCorrectionStrength(double strength) {
    this->correctionStrength = std::max(0.0, std::min(1.0, strength));
}

void TubePitchShifter::SetWindowSize(int samples) {
    this->windowSize = std::max(256, std::min(4096, samples));
    inputBuffer.resize(windowSize, 0.0);
    outputBuffer.resize(windowSize, 0.0);
    phaseBuffer.resize(windowSize, 0.0);
}

TubePitchShifter::~TubePitchShifter() {
    // Destructor handled by member destructors
}