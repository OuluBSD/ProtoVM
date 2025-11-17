#include "TubePhaser.h"
#include "Common.h"
#include <algorithm>
#include <cmath>

// TubePhaser implementation
TubePhaser::TubePhaser(PhaserType type) : phaserType(type) {
    InitializePhaser(type);

    // Initialize with tubes for phaser simulation
    for (int i = 0; i < 2; i++) {
        tubes.push_back(std::make_unique<Triode>(100.0, 100000.0, 1.6e-3));  // Tube simulation
    }
    
    // Initialize LFO for modulation
    modulationLFO = std::make_unique<LFO>(LFOType::SINE, lfoFrequency);
    
    // Initialize allpass filter arrays
    allpassX.resize(stageCount, 0.0);
    allpassY.resize(stageCount, 0.0);
    allpassCoeffs.resize(stageCount, 0.0);
    
    // Initialize envelope follower coefficient
    envelopeCoeff = exp(-1.0 / (0.01 * sampleRate));  // 10ms release time
}

void TubePhaser::InitializePhaser(PhaserType type) {
    switch (type) {
        case TRANSISTOR_STYLE:
            lfoFrequency = 0.8;          // 0.8Hz modulation
            lfoAmount = 0.6;             // 60% LFO depth
            feedback = 0.2;              // Low feedback
            stageCount = 4;              // 4 stages (classic)
            notchCount = 4;              // 4 notches
            centerFrequency = 800.0;     // 800Hz center
            phaseDepth = 0.7;            // 70% phase depth
            dryWetMix = 0.6;             // 60% wet signal
            envelopeAmount = 0.0;        // No envelope follower
            tubeGain = 15.0;             // Lower gain for transistor style
            break;

        case TUBE_STYLE:
            lfoFrequency = 0.5;          // 0.5Hz modulation
            lfoAmount = 0.7;             // 70% LFO depth
            feedback = 0.4;              // Medium feedback for tube warmth
            stageCount = 6;              // 6 stages for richer tube response
            notchCount = 6;              // 6 notches
            centerFrequency = 1000.0;    // 1kHz center
            phaseDepth = 0.8;            // 80% phase depth
            dryWetMix = 0.5;             // 50% wet signal (balanced)
            envelopeAmount = 0.0;        // No envelope follower
            tubeGain = 25.0;             // Higher gain for tube character
            break;

        case MULTI_STAGE:
            lfoFrequency = 0.6;          // 0.6Hz modulation
            lfoAmount = 0.8;             // 80% LFO depth
            feedback = 0.5;              // Higher feedback for intense phasing
            stageCount = 8;              // 8 stages for more notches
            notchCount = 8;              // 8 notches
            centerFrequency = 1200.0;    // 1.2kHz center
            phaseDepth = 0.9;            // 90% phase depth
            dryWetMix = 0.7;             // 70% wet for pronounced effect
            envelopeAmount = 0.0;        // No envelope follower
            tubeGain = 20.0;             // Moderate gain
            break;

        case AUTO_WAH_STYLE:
            lfoFrequency = 0.0;          // No LFO for auto-wah
            lfoAmount = 0.0;             // No LFO depth
            feedback = 0.3;              // Medium feedback
            stageCount = 4;              // 4 stages
            notchCount = 4;              // 4 notches
            centerFrequency = 800.0;     // 800Hz center
            phaseDepth = 0.8;            // 80% phase depth
            dryWetMix = 0.4;             // 40% wet for subtle effect
            envelopeAmount = 0.7;        // 70% envelope follower amount
            tubeGain = 18.0;             // Moderate gain
            envelopeFollowerEnabled = true;
            break;
    }
}

bool bool TubePhaser::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
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
    } else if (conn_id == controlPin && data_bytes == sizeof(double)) {
        memcpy(&controlSignal, data, sizeof(double));
        // Control signal can affect LFO frequency
        SetLFOFrequency(lfoFrequency * (1.0 + 0.5 * controlSignal));  // ±50% frequency change
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
    ProcessSignal();
    return true;
}

void TubePhaser::UpdateCoefficients() {
    // Update allpass coefficients based on LFO and other parameters
    for (int i = 0; i < stageCount; i++) {
        // Calculate base coefficient based on stage and center frequency
        double baseFreq = centerFrequency * pow(2.0, (i - stageCount/2) * 0.2); // Stagger frequencies
        double baseCoeff = (1.0 - (2.0 * M_PI * baseFreq / sampleRate)) / 
                          (1.0 + (2.0 * M_PI * baseFreq / sampleRate));
        
        // Apply LFO modulation if LFO is enabled
        if (lfoFrequency > 0.0) {
            double lfoValue = modulationLFO->GetOutput();  // -1 to 1
            double modulation = lfoAmount * 0.5 * (1.0 + lfoValue);  // 0 to lfoAmount
            double coefficient = baseCoeff * (1.0 + phaseDepth * modulation);
            
            // Constrain coefficient to valid range
            allpassCoeffs[i] = std::max(-0.99, std::min(0.99, coefficient));
        } else {
            allpassCoeffs[i] = baseCoeff;
        }
        
        // Apply envelope follower effect if enabled
        if (envelopeFollowerEnabled) {
            // Modulate based on envelope level
            double envCoeff = envelopeAmount * envelopeDetector;
            allpassCoeffs[i] = allpassCoeffs[i] * (1.0 - envCoeff) + 
                              (allpassCoeffs[i] * 0.5) * envCoeff;  // Shift toward center
        }
    }
}

void TubePhaser::UpdateEnvelopeDetector() {
    if (!envelopeFollowerEnabled) return;
    
    // Simple peak detector with fast attack and slower release
    double inputLevel = std::abs(inputSignal);
    
    if (inputLevel > envelopeDetector) {
        // Fast attack
        envelopeDetector = inputLevel;
    } else {
        // Slower release
        envelopeDetector = envelopeDetector * envelopeCoeff + inputLevel * (1.0 - envelopeCoeff);
    }
    
    // Normalize to 0-1 range (assuming input is around -1 to 1)
    envelopeDetector = std::min(1.0, envelopeDetector * 2.0);  // Scale since input is typically ±1
}

double TubePhaser::ProcessAllpassStage(int stage, double input, double coefficient) {
    // Standard allpass filter implementation
    // y[n] = -a * x[n] + x[n-1] + a * y[n-1]
    double output = -coefficient * input + allpassX[stage] + coefficient * allpassY[stage];
    
    // Update delay elements
    allpassX[stage] = input;
    allpassY[stage] = output;
    
    return output;
}

void TubePhaser::ProcessSignal() {
    // Update envelope detector if enabled
    if (envelopeFollowerEnabled) {
        UpdateEnvelopeDetector();
    }
    
    // Update LFO for modulation
    if (lfoFrequency > 0.0) {
        modulationLFO->SetFrequency(lfoFrequency);
        modulationLFO->Tick();
    }
    
    // Update allpass coefficients based on current parameters
    UpdateCoefficients();
    
    // Apply input with feedback
    double signal = inputSignal + feedbackBuffer * feedback;
    
    // Process through all allpass stages
    for (int i = 0; i < stageCount; i++) {
        signal = ProcessAllpassStage(i, signal, allpassCoeffs[i]);
    }
    
    // Add original signal to create phase cancellation notches
    double phasedSignal = inputSignal + signal;
    
    // Apply tube characteristics if enabled
    if (tubeCharacteristicsEnabled && !tubes.empty()) {
        auto& tube = tubes[0];
        tube->SetGridVoltage(-1.0 + phasedSignal * 0.1);  // Apply signal to grid
        tube->SetPlateVoltage(250.0);  // Set plate voltage
        tube->SetCathodeVoltage(0.0);
        tube->CalculateTubeBehavior();

        // Use the tube's plate current to modify the signal
        double plateCurrent = tube->GetPlateCurrent();
        double tubeEffect = plateCurrent * 0.001;  // Scale appropriately
        
        // Add subtle harmonic content characteristic of tubes
        double harmonicContent = 0.02 * phasedSignal * phasedSignal * (phasedSignal > 0 ? 1 : -1);
        phasedSignal = phasedSignal * (1.0 - tubeGain * 0.01) + 
                      tubeEffect * tubeGain * 0.005 + 
                      harmonicContent * tubeGain * 0.005;
    }
    
    // Update feedback buffer with current output
    feedbackBuffer = phasedSignal;
    
    // Apply dry/wet mix
    outputSignal = inputSignal * (1.0 - dryWetMix) + phasedSignal * dryWetMix;
    
    // Apply final limiting to prevent clipping
    if (outputSignal > 5.0) outputSignal = 5.0;
    if (outputSignal < -5.0) outputSignal = -5.0;
}

void TubePhaser::ApplyTubeCharacteristics() {
    // This is called by ProcessSignal but the tube characteristics are already applied there
    // This function is just for the interface consistency
}

void TubePhaser::SetLFOFrequency(double freq) {
    this->lfoFrequency = std::max(0.0, std::min(10.0, freq));
}

void TubePhaser::SetLFOAmount(double amount) {
    this->lfoAmount = std::max(0.0, std::min(1.0, amount));
}

void TubePhaser::SetFeedback(double feedback) {
    this->feedback = std::max(-0.9, std::min(0.9, feedback));
}

void TubePhaser::SetStageCount(int count) {
    this->stageCount = std::max(2, std::min(12, count));
    this->notchCount = stageCount;  // Typically same as stage count
    
    // Resize arrays to match new stage count
    allpassX.resize(stageCount, 0.0);
    allpassY.resize(stageCount, 0.0);
    allpassCoeffs.resize(stageCount, 0.0);
}

void TubePhaser::SetNotchCount(int count) {
    this->notchCount = std::max(2, std::min(12, count));
}

void TubePhaser::SetCenterFrequency(double freq) {
    this->centerFrequency = std::max(100.0, std::min(10000.0, freq));
}

void TubePhaser::SetPhaseDepth(double depth) {
    this->phaseDepth = std::max(0.0, std::min(1.0, depth));
}

void TubePhaser::SetMix(double dryWet) {
    this->dryWetMix = std::max(0.0, std::min(1.0, dryWet));
}

void TubePhaser::SetEnvelopeAmount(double amount) {
    this->envelopeAmount = std::max(0.0, std::min(1.0, amount));
    envelopeFollowerEnabled = (amount > 0.0);
}

TubePhaser::~TubePhaser() {
    // Destructor handled by member destructors
}