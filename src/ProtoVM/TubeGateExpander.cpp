#include "TubeGateExpander.h"
#include "Common.h"
#include <algorithm>
#include <cmath>

// TubeGateExpander implementation
TubeGateExpander::TubeGateExpander(GateType type) : gateType(type) {
    InitializeGate(type);

    // Calculate coefficients for attack and release
    attackCoeff = exp(-1.0 / (attackTime * sampleRate));
    releaseCoeff = exp(-1.0 / (releaseTime * sampleRate));

    // Initialize with tubes for expansion simulation
    for (int i = 0; i < 2; i++) {
        tubes.push_back(std::make_unique<Triode>(100.0, 100000.0, 1.6e-3));  // Tube simulation
    }

    // Initialize look-ahead buffer if enabled
    if (lookAheadEnabled) {
        int delaySamples = static_cast<int>(0.01 * sampleRate);  // 10ms look-ahead
        lookAheadBufferSize = delaySamples + 10;  // Add some extra space
        lookAheadBuffer.resize(lookAheadBufferSize, 0.0);
        lookAheadWritePos = 0;
    }
}

void TubeGateExpander::InitializeGate(GateType type) {
    switch (type) {
        case NOISE_GATE:
            threshold = -26.0;    // -26dB threshold for noise gate
            ratio = 3.0;          // 3:1 expansion ratio
            attackTime = 0.005;   // 5ms attack
            releaseTime = 0.15;   // 150ms release (adaptive)
            holdTime = 0.1;       // 100ms hold time
            range = -24.0;        // 24dB maximum reduction
            hysteresis = 2.0;     // 2dB hysteresis
            makeupGain = 0.0;     // No makeup gain for noise gate
            tubeGain = 15.0;      // Lower tube gain for noise handling
            break;

        case BANDPASS_GATE:
            threshold = -30.0;    // Lower threshold for bandpass gate
            ratio = 2.5;          // 2.5:1 expansion ratio
            attackTime = 0.003;   // Faster attack for bandpass
            releaseTime = 0.1;    // 100ms release
            holdTime = 0.08;      // 80ms hold time
            range = -18.0;        // 18dB maximum reduction
            hysteresis = 1.5;     // 1.5dB hysteresis
            makeupGain = 0.0;     // No makeup gain
            sidechainFilterFreq = 1000.0;  // 1kHz bandpass
            tubeGain = 17.0;      // Medium tube gain
            break;

        case DOWNWARD_EXPANDER:
            threshold = -18.0;    // -18dB threshold
            ratio = 1.5;          // 1.5:1 expansion ratio (less than 2:1)
            attackTime = 0.010;   // 10ms attack (slightly slower)
            releaseTime = 0.2;    // 200ms release (slower for natural sound)
            holdTime = 0.15;      // 150ms hold time
            range = -15.0;        // 15dB maximum reduction
            hysteresis = 1.0;     // 1dB hysteresis
            makeupGain = 2.0;     // Some makeup gain
            tubeGain = 20.0;      // Higher tube gain for expansion
            break;

        case UPWARD_EXPANDER:
            threshold = -12.0;    // -12dB threshold
            ratio = 0.5;          // Upward expansion (ratio < 1)
            attackTime = 0.015;   // 15ms attack (slower)
            releaseTime = 0.25;   // 250ms release
            holdTime = 0.2;       // 200ms hold time
            range = -10.0;        // 10dB maximum reduction (will expand)
            hysteresis = 1.0;     // 1dB hysteresis
            makeupGain = 3.0;     // More makeup gain for upward expansion
            tubeGain = 22.0;      // Higher tube gain for expansion
            break;
    }
}

bool TubeGateExpander::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeGateExpander::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == inputPin && data_bytes == sizeof(double)) {
        memcpy(&inputSignal, data, sizeof(double));
        
        // Add to look-ahead buffer if enabled
        if (lookAheadEnabled) {
            lookAheadBuffer[lookAheadWritePos] = inputSignal;
            lookAheadWritePos = (lookAheadWritePos + 1) % lookAheadBufferSize;
        }
        return true;
    } else if (conn_id == controlPin && data_bytes == sizeof(double)) {
        memcpy(&controlSignal, data, sizeof(double));
        // Control signal can affect gating parameters
        SetThreshold(-30.0 + 15.0 * (controlSignal + 1.0));  // Map -1,1 to -30 to 0 dB
        return true;
    } else if (conn_id == sidechainPin && data_bytes == sizeof(double)) {
        memcpy(&sidechainSignal, data, sizeof(double));
        return true;
    }
    return false;
}

bool TubeGateExpander::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == outputPin && data_bytes == sizeof(double)) {
        memcpy(data, &outputSignal, sizeof(double));
        return true;
    }
    return false;
}

bool TubeGateExpander::Tick() {
    ProcessSignal();
    return true;
}

double TubeGateExpander::CalculateExpansionGain(double inputLevel) {
    // Convert to dB
    double inputLevelDB = 20.0 * log10(std::abs(inputLevel) + 1e-9);
    
    // Calculate gain based on gate type
    switch (gateType) {
        case NOISE_GATE:
        case BANDPASS_GATE:
            // For noise gates, apply simple gating: below threshold = reduced gain
            if (inputLevelDB < threshold) {
                // Calculate expansion: for every dB below threshold, reduce by (ratio-1)/ratio
                double dbBelowThreshold = threshold - inputLevelDB;
                double gainReductionDB = dbBelowThreshold * (1.0 - 1.0/ratio);
                
                // Apply the range limitation
                gainReductionDB = std::min(gainReductionDB, std::abs(range));
                
                return pow(10.0, -gainReductionDB / 20.0);
            } else {
                // Above threshold, full signal passes through
                return 1.0;
            }
            
        case DOWNWARD_EXPANDER:
            if (inputLevelDB < threshold) {
                // For downward expander, reduce gain below threshold
                double dbBelowThreshold = threshold - inputLevelDB;
                double gainReductionDB = dbBelowThreshold * (1.0 - 1.0/ratio);
                
                gainReductionDB = std::min(gainReductionDB, std::abs(range));
                
                return pow(10.0, -gainReductionDB / 20.0);
            } else {
                return 1.0;
            }
            
        case UPWARD_EXPANDER:
            // For upward expander, gain increases below threshold (ratio < 1)
            if (inputLevelDB < threshold) {
                double dbBelowThreshold = threshold - inputLevelDB;
                
                // For upward expansion, gain is increased (ratio < 1)
                if (ratio < 1.0) {
                    double gainIncreaseDB = dbBelowThreshold * (1.0 - ratio);
                    gainIncreaseDB = std::min(gainIncreaseDB, std::abs(range));
                    return pow(10.0, gainIncreaseDB / 20.0);
                } else {
                    // Fallback if ratio >= 1
                    return 1.0;
                }
            } else {
                return 1.0;
            }
    }
    
    return 1.0;  // Default to unity gain
}

void TubeGateExpander::UpdateDetector() {
    // Get input level from input or sidechain
    double level = std::abs(inputSignal);
    if (sidechainSignal != 0.0) {
        level = std::abs(sidechainSignal);
    }
    
    // Apply sidechain filtering if enabled
    if (sidechainFilterFreq > 0.0) {
        // Simple single-pole low-pass filter simulation
        static double filteredLevel = 0.0;
        double dt = 1.0 / sampleRate;
        double rc = 1.0 / (2.0 * M_PI * sidechainFilterFreq);
        double alpha = dt / (rc + dt);
        
        filteredLevel = filteredLevel + alpha * (level - filteredLevel);
        level = filteredLevel;
    }
    
    // Update detector with attack/release characteristics appropriate for gating
    if (level > detectorLevel) {
        // Fast attack to catch signal immediately
        detectorLevel = detectorLevel * (1.0 - attackCoeff) + level * attackCoeff;
    } else {
        // Release with configured release time
        detectorLevel = detectorLevel * releaseCoeff + level * (1.0 - releaseCoeff);
    }
}

void TubeGateExpander::ProcessSignal() {
    // Update the detector
    UpdateDetector();

    // Calculate gain based on detected level
    double newGain = CalculateExpansionGain(detectorLevel);

    // Apply hysteresis for cleaner gating
    double effectiveThreshold = threshold;
    if (gateOpen) {
        // When gate is open, use higher threshold (hysteresis up)
        effectiveThreshold += hysteresis / 2.0;
    } else {
        // When gate is closed, use lower threshold (hysteresis down)
        effectiveThreshold -= hysteresis / 2.0;
    }
    
    // Recalculate gain with hysteresis-adjusted threshold
    if (20.0 * log10(std::abs(detectorLevel) + 1e-9) < effectiveThreshold) {
        // Signal is below threshold
        if (gateOpen) {
            // Gate was open, now closing
            gateOpen = false;
            holdTimer = holdTime;  // Start hold timer
        }
    } else {
        // Signal is above threshold
        if (!gateOpen) {
            // Gate was closed, now opening
            gateOpen = true;
        }
    }
    
    // Apply hold time if gate is closing
    if (!gateOpen && holdTimer > 0) {
        holdTimer -= 1.0 / sampleRate;
        if (holdTimer <= 0) {
            // Hold time expired, gate stays closed
        }
    } else if (!gateOpen) {
        // Gate is closed
        newGain = 0.0;  // Completely gate the signal
    }

    // Apply adaptive release if enabled
    if (autoReleaseEnabled) {
        // Adaptive release - faster when gain reduction is more
        double adaptiveReleaseCoeff = releaseCoeff * (0.7 + 0.3 * newGain); // Faster release when less reduction
        if (newGain < prevGainReduction) {
            // Attacking (applying more reduction)
            prevGainReduction = newGain;
        } else {
            // Releasing (reducing reduction) - use adaptive coefficient
            prevGainReduction = prevGainReduction * adaptiveReleaseCoeff + newGain * (1.0 - adaptiveReleaseCoeff);
        }
    } else {
        // Standard attack/release
        if (newGain < prevGainReduction) {
            // Attacking (applying more reduction) - respond immediately
            prevGainReduction = newGain;
        } else {
            // Releasing (reducing reduction) - use release coefficient
            prevGainReduction = prevGainReduction * releaseCoeff + newGain * (1.0 - releaseCoeff);
        }
    }

    // Apply tube characteristics to gating
    // For gating, we want to preserve the gating characteristic while adding tube warmth
    double tubeFactor = 1.0;  // Start with unity
    if (tubeCharacteristicsEnabled && !tubes.empty()) {
        auto& tube = tubes[0];
        tube->SetGridVoltage(-1.0 + inputSignal * 0.1);  // Apply signal to grid
        tube->SetPlateVoltage(250.0);  // Set plate voltage
        tube->SetCathodeVoltage(0.0);
        tube->CalculateTubeBehavior();

        // Use the tube's plate current to subtly modify the signal
        double plateCurrent = tube->GetPlateCurrent();
        tubeFactor = 1.0 + 0.05 * plateCurrent * 0.001;  // Subtle tube effect
    }

    // Apply gating/expansion to input
    double processedSignal = inputSignal * prevGainReduction * tubeFactor;

    // Apply makeup gain
    double makeupMultiplier = pow(10.0, makeupGain / 20.0);
    outputSignal = processedSignal * makeupMultiplier;

    // Apply final limiting to prevent clipping
    if (outputSignal > 5.0) outputSignal = 5.0;
    if (outputSignal < -5.0) outputSignal = -5.0;
}

void TubeGateExpander::ApplyTubeCharacteristics() {
    // This is called by ProcessSignal but the tube characteristics are already applied there
    // This function is just for the interface consistency
}

void TubeGateExpander::SetThreshold(double threshold) {
    this->threshold = std::max(-60.0, std::min(0.0, threshold));
}

void TubeGateExpander::SetRatio(double ratio) {
    this->ratio = std::max(1.0, std::min(20.0, ratio));  // For downward expansion
    if (gateType == UPWARD_EXPANDER) {
        this->ratio = std::max(0.1, std::min(1.0, ratio));  // For upward expansion
    }
}

void TubeGateExpander::SetAttackTime(double time) {
    this->attackTime = std::max(0.0001, std::min(0.1, time)); // 0.1ms to 100ms
    attackCoeff = exp(-1.0 / (attackTime * sampleRate));
}

void TubeGateExpander::SetReleaseTime(double time) {
    this->releaseTime = std::max(0.001, std::min(1.0, time)); // 1ms to 1s
    releaseCoeff = exp(-1.0 / (releaseTime * sampleRate));
}

void TubeGateExpander::SetHoldTime(double time) {
    this->holdTime = std::max(0.0, std::min(1.0, time)); // 0s to 1s
}

void TubeGateExpander::SetRange(double range) {
    this->range = std::max(-48.0, std::min(0.0, range)); // -48dB to 0dB
}

void TubeGateExpander::SetHysteresis(double hysteresis) {
    this->hysteresis = std::max(0.0, std::min(6.0, hysteresis)); // 0dB to 6dB
}

void TubeGateExpander::SetMakeupGain(double gain) {
    this->makeupGain = std::max(-20.0, std::min(20.0, gain));
}

void TubeGateExpander::SetSidechainFilterFreq(double freq) {
    this->sidechainFilterFreq = std::max(20.0, std::min(20000.0, freq)); // 20Hz to 20kHz
}

TubeGateExpander::~TubeGateExpander() {
    // Destructor handled by member destructors
}