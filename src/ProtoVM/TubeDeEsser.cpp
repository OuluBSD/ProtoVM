#include "TubeDeEsser.h"
#include "Common.h"
#include <algorithm>
#include <cmath>

// TubeDeEsser implementation
TubeDeEsser::TubeDeEsser(DeEsserType type) : desserType(type) {
    InitializeDeEsser(type);

    // Calculate coefficients for attack and release
    attackCoeff = exp(-1.0 / (attackTime * sampleRate));
    releaseCoeff = exp(-1.0 / (releaseTime * sampleRate));

    // Initialize with tubes for de-essing simulation
    for (int i = 0; i < 2; i++) {
        tubes.push_back(std::make_unique<Triode>(100.0, 100000.0, 1.6e-3));  // Tube simulation
    }

    // Calculate bandpass filter coefficients
    double dt = 1.0 / sampleRate;
    double centerOmega = 2.0 * M_PI * centerFreq;
    double bandwidthOmega = 2.0 * M_PI * bandwidth;
    
    double alpha = sin(centerOmega * dt) * sinh(log(2.0) / 2.0 * bandwidthOmega * dt / centerOmega);
    double cosOmega = cos(centerOmega * dt);
    
    double b0 = alpha;
    double b1 = 0.0;
    double b2 = -alpha;
    double a0 = 1.0 + alpha;
    double a1 = -2.0 * cosOmega;
    double a2 = 1.0 - alpha;
    
    bp_b0 = b0 / a0;
    bp_b1 = b1 / a0;
    bp_b2 = b2 / a0;
    bp_a1 = a1 / a0;
    bp_a2 = a2 / a0;
}

void TubeDeEsser::InitializeDeEsser(DeEsserType type) {
    switch (type) {
        case BANDPASS_DESSER:
            threshold = -15.0;      // -15dB threshold for sibilance detection
            ratio = 4.0;            // 4:1 reduction ratio
            attackTime = 0.002;     // 2ms attack (very fast for 's' sounds)
            releaseTime = 0.03;     // 30ms release
            centerFreq = 5000.0;    // 5kHz center frequency for sibilance
            bandwidth = 2000.0;     // 2kHz bandwidth
            makeupGain = 1.0;       // Small makeup gain
            sidechainRatio = 0.6;   // 60% sidechain processing
            highFreqThreshold = -18.0;
            tubeGain = 18.0;        // Medium tube gain
            break;

        case HIGH_FREQ_DESSER:
            threshold = -12.0;      // Lower threshold for high-frequency detection
            ratio = 3.0;            // 3:1 reduction ratio
            attackTime = 0.0015;    // 1.5ms attack (even faster)
            releaseTime = 0.025;    // 25ms release
            centerFreq = 7000.0;    // 7kHz center frequency for high sibilance
            bandwidth = 1500.0;     // 1.5kHz bandwidth
            makeupGain = 0.0;       // No makeup gain
            sidechainRatio = 0.7;   // 70% sidechain processing for high freqs
            highFreqThreshold = -15.0;
            tubeGain = 20.0;        // Higher tube gain for high freqs
            break;

        case ADAPTIVE_DESSER:
            threshold = -10.0;      // -10dB threshold for adaptive mode
            ratio = 5.0;            // 5:1 reduction ratio (higher for adaptive)
            attackTime = 0.003;     // 3ms attack
            releaseTime = 0.04;     // 40ms release (slightly slower for adaptive)
            centerFreq = 6000.0;    // 6kHz center frequency
            bandwidth = 2500.0;     // 2.5kHz bandwidth
            makeupGain = 1.5;       // Slightly more makeup gain
            sidechainRatio = 0.5;   // 50% sidechain processing
            highFreqThreshold = -20.0;
            tubeGain = 16.0;        // Lower tube gain for adaptive processing
            adaptiveMode = true;    // Enable adaptive processing
            break;

        case PROPORTIONAL_DESSER:
            threshold = -14.0;      // -14dB threshold for proportional mode
            ratio = 2.0;            // 2:1 ratio (proportional to sibilance level)
            attackTime = 0.0025;    // 2.5ms attack
            releaseTime = 0.035;    // 35ms release
            centerFreq = 5500.0;    // 5.5kHz center frequency
            bandwidth = 1800.0;     // 1.8kHz bandwidth
            makeupGain = 0.5;       // Small makeup gain
            sidechainRatio = 0.8;   // 80% sidechain processing
            highFreqThreshold = -16.0;
            tubeGain = 22.0;        // Higher tube gain for proportional processing
            break;
    }
}

bool TubeDeEsser::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeDeEsser::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == inputPin && data_bytes == sizeof(double)) {
        memcpy(&inputSignal, data, sizeof(double));
        return true;
    } else if (conn_id == controlPin && data_bytes == sizeof(double)) {
        memcpy(&controlSignal, data, sizeof(double));
        // Control signal can affect de-essing parameters
        SetThreshold(-20.0 + 10.0 * (controlSignal + 1.0));  // Map -1,1 to -20 to -10 dB
        return true;
    } else if (conn_id == sidechainPin && data_bytes == sizeof(double)) {
        memcpy(&sidechainSignal, data, sizeof(double));
        return true;
    }
    return false;
}

bool TubeDeEsser::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == outputPin && data_bytes == sizeof(double)) {
        memcpy(data, &outputSignal, sizeof(double));
        return true;
    }
    return false;
}

bool TubeDeEsser::Tick() {
    ProcessSignal();
    return true;
}

double TubeDeEsser::ApplyBandpassFilter(double input) {
    static double x1 = 0.0, x2 = 0.0, y1 = 0.0, y2 = 0.0;
    
    // Apply biquad bandpass filter
    double output = bp_b0 * input + bp_b1 * x1 + bp_b2 * x2 - bp_a1 * y1 - bp_a2 * y2;
    
    x2 = x1; x1 = input;
    y2 = y1; y1 = output;
    
    return output;
}

double TubeDeEsser::CalculateDeEsserGain(double inputLevel) {
    // Convert to dB
    double inputLevelDB = 20.0 * log10(std::abs(inputLevel) + 1e-9);
    
    // For de-esser, apply reduction when sibilance is detected (level above threshold)
    if (inputLevelDB > threshold) {
        // Calculate de-essing: for every dB above threshold, reduce by ratio
        double dbAboveThreshold = inputLevelDB - threshold;
        double gainReductionDB = dbAboveThreshold * (1.0 - 1.0/ratio);
        
        // Apply proportional reduction in proportional mode
        if (desserType == PROPORTIONAL_DESSER) {
            gainReductionDB = gainReductionDB * (dbAboveThreshold / 3.0);  // Make more proportional
        }
        
        // Apply adaptive behavior if enabled
        if (adaptiveMode) {
            // Adaptive processing - adjust based on characteristics
            gainReductionDB = std::min(gainReductionDB * 1.2, 12.0);  // Limit adaptive gain
        }
        
        return pow(10.0, -gainReductionDB / 20.0);
    } else {
        // Below threshold, no de-essing needed
        return 1.0;
    }
}

void TubeDeEsser::UpdateDetector() {
    // Get input signal to analyze for sibilance
    double signal = inputSignal;
    if (sidechainSignal != 0.0) {
        signal = sidechainSignal;
    }
    
    // Apply bandpass filter to isolate sibilance frequencies
    double bandpassSignal = ApplyBandpassFilter(signal);
    detectorLevel = std::abs(bandpassSignal);  // Detected sibilance level
    
    // Also measure high-frequency content for additional detection
    double highFreqSignal = signal - bandpassSignal;  // Everything not in the bandpass
    highFreqDetector = std::abs(highFreqSignal);
    
    // Update detector with fast attack for sibilance
    static double prevDetector = 0.0;
    static double prevHighFreq = 0.0;
    
    if (detectorLevel > prevDetector) {
        // Fast attack to catch sibilance immediately
        prevDetector = detectorLevel;
    } else {
        // Release with configured release time
        prevDetector = prevDetector * releaseCoeff + detectorLevel * (1.0 - releaseCoeff);
    }
    
    // Update high-frequency detector
    if (highFreqDetector > prevHighFreq) {
        prevHighFreq = highFreqDetector;
    } else {
        prevHighFreq = prevHighFreq * releaseCoeff + highFreqDetector * (1.0 - releaseCoeff);
    }
    
    detectorLevel = prevDetector;
    highFreqDetector = prevHighFreq;
}

void TubeDeEsser::ProcessSignal() {
    // Update the detector
    UpdateDetector();

    // Calculate gain based on detected sibilance level and high frequencies
    double totalDetector = detectorLevel + (sidechainRatio * highFreqDetector);
    double newGain = CalculateDeEsserGain(totalDetector);

    // Apply adaptive behavior if enabled
    if (adaptiveMode) {
        // Calculate adaptive ratio based on signal characteristics
        double freqRatio = centerFreq / 5000.0;  // Adjust for different center frequencies
        newGain = newGain * (0.8 + 0.4 * freqRatio);  // Adaptive adjustment
    }

    // Apply attack/release characteristics to gain reduction
    if (newGain < prevGainReduction) {
        // Attacking (applying more reduction) - respond immediately
        prevGainReduction = newGain;
    } else {
        // Releasing (reducing reduction) - use release coefficient for smooth release
        prevGainReduction = prevGainReduction * releaseCoeff + newGain * (1.0 - releaseCoeff);
    }

    // Apply tube characteristics to de-essing
    // For de-essing, we want to preserve the effect while adding subtle harmonic content
    double tubeFactor = 1.0;  // Start with unity
    if (tubeCharacteristicsEnabled && !tubes.empty()) {
        auto& tube = tubes[0];
        // Apply a small amount of the processed signal to the tube input
        double tubeInput = inputSignal * prevGainReduction * 0.05;  // Small signal
        tube->SetGridVoltage(-1.0 + tubeInput);  // Apply signal to grid
        tube->SetPlateVoltage(250.0);  // Set plate voltage
        tube->SetCathodeVoltage(0.0);
        tube->CalculateTubeBehavior();

        // Use the tube's plate current to subtly modify the signal
        double plateCurrent = tube->GetPlateCurrent();
        tubeFactor = 1.0 + 0.02 * plateCurrent * 0.001;  // Very subtle tube effect
    }

    // Apply de-essing to input signal
    double processedSignal = inputSignal * prevGainReduction * tubeFactor;

    // Apply makeup gain
    double makeupMultiplier = pow(10.0, makeupGain / 20.0);
    outputSignal = processedSignal * makeupMultiplier;

    // Apply final limiting to prevent clipping
    if (outputSignal > 5.0) outputSignal = 5.0;
    if (outputSignal < -5.0) outputSignal = -5.0;
}

void TubeDeEsser::ApplyTubeCharacteristics() {
    // This is called by ProcessSignal but the tube characteristics are already applied there
    // This function is just for the interface consistency
}

void TubeDeEsser::SetThreshold(double threshold) {
    this->threshold = std::max(-30.0, std::min(0.0, threshold));
}

void TubeDeEsser::SetRatio(double ratio) {
    this->ratio = std::max(1.0, std::min(10.0, ratio));  // 1:1 to 10:1 ratio
}

void TubeDeEsser::SetAttackTime(double time) {
    this->attackTime = std::max(0.0001, std::min(0.01, time)); // 0.1ms to 10ms
    attackCoeff = exp(-1.0 / (attackTime * sampleRate));
}

void TubeDeEsser::SetReleaseTime(double time) {
    this->releaseTime = std::max(0.001, std::min(0.1, time)); // 1ms to 100ms
    releaseCoeff = exp(-1.0 / (releaseTime * sampleRate));
}

void TubeDeEsser::SetFrequency(double freq) {
    this->centerFreq = std::max(1000.0, std::min(12000.0, freq)); // 1kHz to 12kHz
    
    // Recalculate bandpass filter coefficients with new frequency
    double dt = 1.0 / sampleRate;
    double centerOmega = 2.0 * M_PI * centerFreq;
    double bandwidthOmega = 2.0 * M_PI * bandwidth;
    
    double alpha = sin(centerOmega * dt) * sinh(log(2.0) / 2.0 * bandwidthOmega * dt / centerOmega);
    double cosOmega = cos(centerOmega * dt);
    
    double b0 = alpha;
    double b1 = 0.0;
    double b2 = -alpha;
    double a0 = 1.0 + alpha;
    double a1 = -2.0 * cosOmega;
    double a2 = 1.0 - alpha;
    
    bp_b0 = b0 / a0;
    bp_b1 = b1 / a0;
    bp_b2 = b2 / a0;
    bp_a1 = a1 / a0;
    bp_a2 = a2 / a0;
}

void TubeDeEsser::SetBandwidth(double bandwidth) {
    this->bandwidth = std::max(500.0, std::min(4000.0, bandwidth)); // 500Hz to 4kHz
    
    // Recalculate bandpass filter coefficients with new bandwidth
    double dt = 1.0 / sampleRate;
    double centerOmega = 2.0 * M_PI * centerFreq;
    double bandwidthOmega = 2.0 * M_PI * bandwidth;
    
    double alpha = sin(centerOmega * dt) * sinh(log(2.0) / 2.0 * bandwidthOmega * dt / centerOmega);
    double cosOmega = cos(centerOmega * dt);
    
    double b0 = alpha;
    double b1 = 0.0;
    double b2 = -alpha;
    double a0 = 1.0 + alpha;
    double a1 = -2.0 * cosOmega;
    double a2 = 1.0 - alpha;
    
    bp_b0 = b0 / a0;
    bp_b1 = b1 / a0;
    bp_b2 = b2 / a0;
    bp_a1 = a1 / a0;
    bp_a2 = a2 / a0;
}

void TubeDeEsser::SetMakeupGain(double gain) {
    this->makeupGain = std::max(-20.0, std::min(20.0, gain));
}

void TubeDeEsser::SetSidechainRatio(double ratio) {
    this->sidechainRatio = std::max(0.0, std::min(1.0, ratio));  // 0.0 to 1.0
}

void TubeDeEsser::SetHighFreqThreshold(double threshold) {
    this->highFreqThreshold = std::max(-40.0, std::min(0.0, threshold));
}

TubeDeEsser::~TubeDeEsser() {
    // Destructor handled by member destructors
}