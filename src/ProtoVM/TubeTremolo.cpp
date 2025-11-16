#include "TubeTremolo.h"
#include "Common.h"
#include <algorithm>
#include <cmath>

// TubeTremolo implementation
TubeTremolo::TubeTremolo(TremoloType type) : tremoloType(type) {
    InitializeTremolo(type);

    // Initialize with tubes for tremolo simulation
    for (int i = 0; i < 2; i++) {
        tubes.push_back(std::make_unique<Triode>(100.0, 100000.0, 1.6e-3));  // Tube simulation
    }
    
    // Initialize LFO for modulation
    modulationLFO = std::make_unique<LFO>(lfoShape, lfoFrequency);
    
    // Initialize envelope follower coefficient
    envelopeCoeff = exp(-1.0 / (0.02 * sampleRate));  // 20ms release time
    
    // Initialize tone coefficient based on tone setting
    toneCoeff = 0.5 + tone * 0.5;  // Range from 0.5 to 1.0
}

void TubeTremolo::InitializeTremolo(TremoloType type) {
    switch (type) {
        case PHOTOCELL_TREMOLO:
            lfoFrequency = 4.0;          // 4Hz classic tremolo speed
            lfoAmount = 1.0;             // Full LFO depth
            lfoShape = LFOType::TRIANGLE; // Triangle for more linear sweep
            depth = 0.7;                 // 70% depth
            tone = 0.4;                  // Slightly dark
            bias = 0.5;                  // Center bias
            envelopeAmount = 0.0;        // No envelope follower
            dryWetMix = 1.0;             // Full wet signal (tremolo is always "on")
            asymmetry = 0.1;             // Slight asymmetry for photcell effect
            tubeGain = 18.0;             // Medium gain for photcell simulation
            break;

        case TUBE_VARIATION:
            lfoFrequency = 6.0;          // 6Hz for tube variation
            lfoAmount = 0.9;             // 90% LFO depth
            lfoShape = LFOType::SINE;    // Sine for smooth tube variation
            depth = 0.9;                 // 90% depth for intensity
            tone = 0.6;                  // Brighter tone for tube effect
            bias = 0.4;                  // Lower bias to emphasize reduction
            envelopeAmount = 0.0;        // No envelope follower
            dryWetMix = 1.0;             // Full effect
            asymmetry = 0.2;             // More asymmetry for tube behavior
            tubeGain = 22.0;             // Higher gain for tube character
            break;

        case RATIO_CHANGER:
            lfoFrequency = 3.5;          // 3.5Hz for ratio changer effect
            lfoAmount = 1.0;             // Full depth
            lfoShape = LFOType::SQUARE;  // Square for on/off effect
            depth = 0.95;                // Near full depth
            tone = 0.5;                  // Neutral tone
            bias = 0.7;                  // High bias to keep signal present
            envelopeAmount = 0.0;        // No envelope follower
            dryWetMix = 1.0;             // Full effect
            asymmetry = 0.0;             // Symmetrical for ratio changer
            tubeGain = 15.0;             // Lower gain for switching effect
            break;

        case VIBRATO_TREMOLO:
            lfoFrequency = 6.5;          // 6.5Hz for vibrato-like effect
            lfoAmount = 0.8;             // 80% LFO depth
            lfoShape = LFOType::SINE;    // Sine for smooth vibrato
            depth = 0.6;                 // Lower depth for vibrato feel
            tone = 0.7;                  // Brighter for vibrato effect
            bias = 0.9;                  // High bias to preserve signal
            envelopeAmount = 0.3;        // Some envelope follower
            dryWetMix = 0.8;             // 80% wet for effect
            asymmetry = 0.05;            // Very slight asymmetry
            envelopeFollowerEnabled = true;
            tubeGain = 20.0;             // Medium gain for vibrato
            break;
    }
}

bool TubeTremolo::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeTremolo::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
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

bool TubeTremolo::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == outputPin && data_bytes == sizeof(double)) {
        memcpy(data, &outputSignal, sizeof(double));
        return true;
    }
    return false;
}

bool TubeTremolo::Tick() {
    ProcessSignal();
    return true;
}

void TubeTremolo::UpdateEnvelopeDetector() {
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
    envelopeDetector = std::min(1.0, envelopeDetector);
}

double TubeTremolo::ApplyToneShaping(double input) {
    // Apply simple high-pass or low-pass filtering based on tone setting
    static double hpState = 0.0, lpState = 0.0;
    
    if (tone < 0.5) {
        // Apply high-cut (low-pass)
        double cutoff = 2000.0 * (1.0 - (tone * 2.0));  // From 2kHz to DC
        double dt = 1.0 / sampleRate;
        double rc = 1.0 / (2.0 * M_PI * cutoff);
        double coeff = dt / (rc + dt);
        
        lpState = lpState + coeff * (input - lpState);
        return lpState;
    } else if (tone > 0.5) {
        // Apply low-cut (high-pass)
        double cutoff = 100.0 * ((tone - 0.5) * 2.0);  // From DC to 100Hz
        double dt = 1.0 / sampleRate;
        double rc = 1.0 / (2.0 * M_PI * cutoff);
        double coeff = rc / (rc + dt);
        
        double inputForHP = input;
        double result = coeff * (input + hpState - inputForHP);
        hpState = result;
        return result;
    }
    
    return input;  // Neutral tone
}

void TubeTremolo::ProcessSignal() {
    // Update envelope detector if enabled
    if (envelopeFollowerEnabled) {
        UpdateEnvelopeDetector();
    }
    
    // Update LFO for tremolo modulation
    modulationLFO->SetFrequency(lfoFrequency);
    modulationLFO->SetAmplitude(lfoAmount);
    modulationLFO->SetType(lfoShape);
    modulationLFO->Tick();
    
    // Get LFO value (-1.0 to 1.0) and adjust for asymmetry
    double lfoValue = modulationLFO->GetOutput();
    
    // Apply asymmetry if needed
    if (asymmetry > 0.0) {
        // Modify the waveform to add asymmetry
        if (lfoValue > 0) {
            lfoValue = lfoValue * (1.0 + asymmetry);
        } else {
            lfoValue = lfoValue * (1.0 - asymmetry);
        }
        // Normalize back to -1 to 1 range
        if (abs(lfoValue) > 1.0) lfoValue = lfoValue > 0 ? 1.0 : -1.0;
    }
    
    // Calculate modulation depth based on settings
    double modulationDepth = depth * 0.5;  // Convert to 0-1 range for depth
    currentModulation = bias + (lfoValue * modulationDepth);
    
    // Apply envelope follower if enabled
    if (envelopeFollowerEnabled) {
        currentModulation = currentModulation * (1.0 - envelopeAmount) + 
                          (bias + envelopeDetector * envelopeAmount * 0.5) * envelopeAmount;
    }
    
    // Constrain modulation to valid range
    currentModulation = std::max(0.05, std::min(1.0, currentModulation));
    
    // Apply tone shaping to input
    double shapedInput = ApplyToneShaping(inputSignal);
    
    // Apply tremolo modulation (amplitude modulation)
    double tremoloSignal = shapedInput * currentModulation;
    
    // Apply tube characteristics if enabled
    if (tubeCharacteristicsEnabled && !tubes.empty()) {
        auto& tube = tubes[0];
        tube->SetGridVoltage(-1.0 + tremoloSignal * 0.1);  // Apply signal to grid
        tube->SetPlateVoltage(250.0);  // Set plate voltage
        tube->SetCathodeVoltage(0.0);
        tube->CalculateTubeBehavior();

        // Use the tube's plate current to modify the signal
        double plateCurrent = tube->GetPlateCurrent();
        double tubeEffect = plateCurrent * 0.001;  // Scale appropriately
        
        // Add subtle harmonic content characteristic of tubes
        double harmonicContent = 0.01 * tremoloSignal * tremoloSignal * (tremoloSignal > 0 ? 1 : -1);
        tremoloSignal = tremoloSignal * (1.0 - tubeGain * 0.005) + 
                       tubeEffect * tubeGain * 0.0025 + 
                       harmonicContent * tubeGain * 0.0025;
    }
    
    // Apply dry/wet mix
    outputSignal = inputSignal * (1.0 - dryWetMix) + tremoloSignal * dryWetMix;
    
    // Apply final limiting to prevent clipping
    if (outputSignal > 5.0) outputSignal = 5.0;
    if (outputSignal < -5.0) outputSignal = -5.0;
}

void TubeTremolo::ApplyTubeCharacteristics() {
    // This is called by ProcessSignal but the tube characteristics are already applied there
    // This function is just for the interface consistency
}

void TubeTremolo::SetLFOFrequency(double freq) {
    this->lfoFrequency = std::max(0.1, std::min(20.0, freq));
}

void TubeTremolo::SetLFOAmount(double amount) {
    this->lfoAmount = std::max(0.0, std::min(1.0, amount));
}

void TubeTremolo::SetLFOShape(LFOType shape) {
    this->lfoShape = shape;
}

void TubeTremolo::SetDepth(double depth) {
    this->depth = std::max(0.0, std::min(1.0, depth));
}

void TubeTremolo::SetTone(double tone) {
    this->tone = std::max(0.0, std::min(1.0, tone));
    // Update tone coefficient based on new setting
    toneCoeff = 0.5 + tone * 0.5;  // Range from 0.5 to 1.0
}

void TubeTremolo::SetBias(double bias) {
    this->bias = std::max(0.05, std::min(1.0, bias));
}

void TubeTremolo::SetEnvelopeAmount(double amount) {
    this->envelopeAmount = std::max(0.0, std::min(1.0, amount));
    envelopeFollowerEnabled = (amount > 0.0);
}

void TubeTremolo::SetMix(double dryWet) {
    this->dryWetMix = std::max(0.0, std::min(1.0, dryWet));
}

void TubeTremolo::SetAsymmetry(double asym) {
    this->asymmetry = std::max(0.0, std::min(1.0, asym));
}

TubeTremolo::~TubeTremolo() {
    // Destructor handled by member destructors
}