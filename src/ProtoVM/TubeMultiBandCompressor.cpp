#include "TubeMultiBandCompressor.h"
#include "Common.h"
#include <algorithm>
#include <cmath>

// TubeMultiBandCompressor implementation
TubeMultiBandCompressor::TubeMultiBandCompressor(MultiBandConfig config) : config(config) {
    InitializeMultiBand(config);

    // Initialize with tubes for multi-band compression simulation
    for (int i = 0; i < 3; i++) {
        tubes.push_back(std::make_unique<Triode>(100.0, 100000.0, 1.6e-3));  // Tube simulation
    }
    
    // Initialize filter states
    filterStateX.resize(numBands * 2, 0.0);  // Two filters per band (low/high)
    filterStateY.resize(numBands * 2, 0.0);
    
    // Initialize band signals
    bandSignals.resize(numBands, 0.0);
}

void TubeMultiBandCompressor::InitializeMultiBand(MultiBandConfig config) {
    switch (config) {
        case THREE_BAND:
            numBands = 3;
            thresholds.resize(3, -18.0);           // -18dB threshold for all bands
            ratios.resize(3, 4.0);                 // 4:1 ratio for all bands  
            attackTimes.resize(3, 0.005);          // 5ms attack for all bands
            releaseTimes.resize(3, 0.1);           // 100ms release for all bands
            crossoverFreqs.resize(2, 0.0);         // 2 crossover points needed for 3 bands
            crossoverFreqs[0] = 200.0;             // Low/Mid crossover at 200Hz
            crossoverFreqs[1] = 2000.0;            // Mid/High crossover at 2kHz
            bandGains.resize(3, 1.0);
            prevBandGains.resize(3, 1.0);
            attackCoeffs.resize(3, 0.0);
            releaseCoeffs.resize(3, 0.0);
            bandSolan.resize(3, false);
            bandMuted.resize(3, false);
            
            // Calculate coefficients for each band
            for (int i = 0; i < 3; i++) {
                attackCoeffs[i] = exp(-1.0 / (attackTimes[i] * sampleRate));
                releaseCoeffs[i] = exp(-1.0 / (releaseTimes[i] * sampleRate));
            }
            
            // Calculate filter coefficients
            CalculateFilterCoeffs(0, crossoverFreqs[0]);  // Low-pass for low band
            CalculateFilterCoeffs(1, crossoverFreqs[1]);  // Band-pass for mid band
            break;

        case FIVE_BAND:
            numBands = 5;
            thresholds.resize(5, -20.0);           // -20dB threshold for all bands
            ratios.resize(5, 3.0);                 // 3:1 ratio for all bands
            attackTimes.resize(5, 0.003);          // 3ms attack for all bands
            releaseTimes.resize(5, 0.15);          // 150ms release for all bands
            crossoverFreqs.resize(4, 0.0);         // 4 crossover points needed for 5 bands
            crossoverFreqs[0] = 80.0;              // Very Low / Low crossover
            crossoverFreqs[1] = 250.0;             // Low / Mid crossover
            crossoverFreqs[2] = 800.0;             // Mid / High crossover
            crossoverFreqs[3] = 2500.0;            // High / Very High crossover
            bandGains.resize(5, 1.0);
            prevBandGains.resize(5, 1.0);
            attackCoeffs.resize(5, 0.0);
            releaseCoeffs.resize(5, 0.0);
            bandSolan.resize(5, false);
            bandMuted.resize(5, false);
            
            // Calculate coefficients for each band
            for (int i = 0; i < 5; i++) {
                attackCoeffs[i] = exp(-1.0 / (attackTimes[i] * sampleRate));
                releaseCoeffs[i] = exp(-1.0 / (releaseTimes[i] * sampleRate));
            }
            
            // Calculate filter coefficients for each crossover
            for (int i = 0; i < 4; i++) {
                CalculateFilterCoeffs(i, crossoverFreqs[i]);
            }
            break;

        case SEVEN_BAND:
            numBands = 7;
            thresholds.resize(7, -22.0);           // -22dB threshold for all bands
            ratios.resize(7, 2.5);                 // 2.5:1 ratio for all bands
            attackTimes.resize(7, 0.004);          // 4ms attack for all bands
            releaseTimes.resize(7, 0.2);           // 200ms release for all bands
            crossoverFreqs.resize(6, 0.0);         // 6 crossover points for 7 bands
            crossoverFreqs[0] = 60.0;              // 60Hz
            crossoverFreqs[1] = 150.0;             // 150Hz
            crossoverFreqs[2] = 400.0;             // 400Hz
            crossoverFreqs[3] = 1000.0;            // 1kHz
            crossoverFreqs[4] = 2500.0;            // 2.5kHz
            crossoverFreqs[5] = 6000.0;            // 6kHz
            bandGains.resize(7, 1.0);
            prevBandGains.resize(7, 1.0);
            attackCoeffs.resize(7, 0.0);
            releaseCoeffs.resize(7, 0.0);
            bandSolan.resize(7, false);
            bandMuted.resize(7, false);
            
            // Calculate coefficients for each band
            for (int i = 0; i < 7; i++) {
                attackCoeffs[i] = exp(-1.0 / (attackTimes[i] * sampleRate));
                releaseCoeffs[i] = exp(-1.0 / (releaseTimes[i] * sampleRate));
            }
            
            // Calculate filter coefficients for each crossover
            for (int i = 0; i < 6; i++) {
                CalculateFilterCoeffs(i, crossoverFreqs[i]);
            }
            break;

        case PARAMETRIC_BAND:
            numBands = 4;  // Default parametric configuration
            thresholds.resize(4, -20.0);
            ratios.resize(4, 3.0);
            attackTimes.resize(4, 0.005);
            releaseTimes.resize(4, 0.15);
            crossoverFreqs.resize(3, 0.0);
            crossoverFreqs[0] = 150.0;
            crossoverFreqs[1] = 800.0;
            crossoverFreqs[2] = 3000.0;
            bandGains.resize(4, 1.0);
            prevBandGains.resize(4, 1.0);
            attackCoeffs.resize(4, 0.0);
            releaseCoeffs.resize(4, 0.0);
            bandSolan.resize(4, false);
            bandMuted.resize(4, false);
            
            // Calculate coefficients for each band
            for (int i = 0; i < 4; i++) {
                attackCoeffs[i] = exp(-1.0 / (attackTimes[i] * sampleRate));
                releaseCoeffs[i] = exp(-1.0 / (releaseTimes[i] * sampleRate));
            }
            
            // Calculate filter coefficients
            for (int i = 0; i < 3; i++) {
                CalculateFilterCoeffs(i, crossoverFreqs[i]);
            }
            break;
    }
    
    // Initialize tube gain based on configuration
    tubeGain = 25.0 - (numBands * 1.0);  // Slightly less gain with more bands
}

void TubeMultiBandCompressor::CalculateFilterCoeffs(int band, double freq) {
    if (band >= filterCoeffs.size()) {
        filterCoeffs.resize(band + 1);
    }
    
    // Calculate coefficients for low-pass filter
    double dt = 1.0 / sampleRate;
    double omega = 2.0 * M_PI * freq;
    double sn = sin(omega * dt);
    double cs = cos(omega * dt);
    double alpha = sn / (2.0 * 0.707);  // Q factor of 0.707 (Butterworth)

    double b0 = (1.0 - cs) / 2.0;
    double b1 = 1.0 - cs;
    double b2 = (1.0 - cs) / 2.0;
    double a0 = 1.0 + alpha;
    double a1 = -2.0 * cs;
    double a2 = 1.0 - alpha;

    filterCoeffs[band].low_b0 = b0 / a0;
    filterCoeffs[band].low_b1 = b1 / a0;
    filterCoeffs[band].low_b2 = b2 / a0;
    filterCoeffs[band].low_a1 = a1 / a0;
    filterCoeffs[band].low_a2 = a2 / a0;

    // Calculate coefficients for high-pass filter
    b0 = (1.0 + cs) / 2.0;
    b1 = -(1.0 + cs);
    b2 = (1.0 + cs) / 2.0;

    filterCoeffs[band].high_b0 = b0 / a0;
    filterCoeffs[band].high_b1 = b1 / a0;
    filterCoeffs[band].high_b2 = b2 / a0;
    // a1 and a2 are the same for high-pass
    filterCoeffs[band].high_a1 = a1 / a0;
    filterCoeffs[band].high_a2 = a2 / a0;
}

bool TubeMultiBandCompressor::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeMultiBandCompressor::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == inputPin && data_bytes == sizeof(double)) {
        memcpy(&inputSignal, data, sizeof(double));
        return true;
    } else if (conn_id == controlPin && data_bytes == sizeof(double)) {
        memcpy(&controlSignal, data, sizeof(double));
        // Control signal could affect overall compression parameters
        SetOverallGain(overallGain + 6.0 * controlSignal);  // Map -1,1 to ±6dB gain
        return true;
    }
    return false;
}

bool TubeMultiBandCompressor::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == outputPin && data_bytes == sizeof(double)) {
        memcpy(data, &outputSignal, sizeof(double));
        return true;
    }
    return false;
}

bool TubeMultiBandCompressor::Tick() {
    ProcessSignal();
    return true;
}

double TubeMultiBandCompressor::ProcessBand(int band, double input, double detectionLevel) {
    // Convert detection level to dB
    double inputLevelDB = 20.0 * log10(std::abs(detectionLevel) + 1e-9);
    
    // Apply compression based on threshold and ratio
    double newGain = 1.0;
    if (inputLevelDB > thresholds[band]) {
        // Calculate compression: for every dB above threshold, compress by ratio
        double dbAboveThreshold = inputLevelDB - thresholds[band];
        double gainReductionDB = dbAboveThreshold * (1.0 - 1.0/ratios[band]);
        
        newGain = pow(10.0, -gainReductionDB / 20.0);
    }
    
    // Apply attack/release characteristics to gain reduction
    if (newGain < prevBandGains[band]) {
        // Attacking (applying more compression) - respond immediately
        bandGains[band] = newGain;
    } else {
        // Releasing (reducing compression) - use release coefficient
        bandGains[band] = prevBandGains[band] * releaseCoeffs[band] + newGain * (1.0 - releaseCoeffs[band]);
    }
    
    // Update previous gain for next sample
    prevBandGains[band] = bandGains[band];
    
    // Apply the calculated gain to the input
    return input * bandGains[band];
}

void TubeMultiBandCompressor::ApplyCrossoverFilters() {
    // Apply crossover filters to separate the signal into frequency bands
    // This is a simplified implementation - a real multi-band compressor would have 
    // more sophisticated crossover networks with steeper slopes
    
    double tempSignal = inputSignal;
    std::vector<double> bandOutputs(numBands, 0.0);
    
    if (numBands == 3) {
        // Three-band crossover: Low, Mid, High
        // First, get the low band (low-pass below crossoverFreqs[0])
        static double lp1_x1 = 0.0, lp1_x2 = 0.0, lp1_y1 = 0.0, lp1_y2 = 0.0;
        double lowBand = filterCoeffs[0].low_b0 * tempSignal + 
                        filterCoeffs[0].low_b1 * lp1_x1 + 
                        filterCoeffs[0].low_b2 * lp1_x2 - 
                        filterCoeffs[0].low_a1 * lp1_y1 - 
                        filterCoeffs[0].low_a2 * lp1_y2;
        lp1_x2 = lp1_x1; lp1_x1 = tempSignal;
        lp1_y2 = lp1_y1; lp1_y1 = lowBand;
        
        // Mid band is band-pass (low-pass at crossoverFreqs[1] minus low-pass at crossoverFreqs[0])
        static double lp2_x1 = 0.0, lp2_x2 = 0.0, lp2_y1 = 0.0, lp2_y2 = 0.0;
        double midThrough = filterCoeffs[1].low_b0 * tempSignal + 
                           filterCoeffs[1].low_b1 * lp2_x1 + 
                           filterCoeffs[1].low_b2 * lp2_x2 - 
                           filterCoeffs[1].low_a1 * lp2_y1 - 
                           filterCoeffs[1].low_a2 * lp2_y2;
        lp2_x2 = lp2_x1; lp2_x1 = tempSignal;
        lp2_y2 = lp2_y1; lp2_y1 = midThrough;
        
        double midBand = midThrough - lowBand;
        
        // High band is high-pass above crossoverFreqs[1]
        static double hp_x1 = 0.0, hp_x2 = 0.0, hp_y1 = 0.0, hp_y2 = 0.0;
        double highBand = filterCoeffs[1].high_b0 * tempSignal + 
                         filterCoeffs[1].high_b1 * hp_x1 + 
                         filterCoeffs[1].high_b2 * hp_x2 - 
                         filterCoeffs[1].high_a1 * hp_y1 - 
                         filterCoeffs[1].high_a2 * hp_y2;
        hp_x2 = hp_x1; hp_x1 = tempSignal;
        hp_y2 = hp_y1; hp_y1 = highBand;
        
        // Store in band outputs
        bandOutputs[0] = lowBand;    // Low band
        bandOutputs[1] = midBand;    // Mid band
        bandOutputs[2] = highBand;   // High band
    } else if (numBands == 5) {
        // Five-band crossover: Very Low, Low, Mid, High, Very High
        // This is a simplified implementation for demonstration
        // In practice, this would require cascaded filters
        bandOutputs[0] = tempSignal * 0.2; // Very low band (simplified)
        bandOutputs[1] = tempSignal * 0.2; // Low band (simplified)
        bandOutputs[2] = tempSignal * 0.2; // Mid band (simplified)
        bandOutputs[3] = tempSignal * 0.2; // High band (simplified)
        bandOutputs[4] = tempSignal * 0.2; // Very high band (simplified)
    } else {
        // For other configurations, just distribute equally (simplified)
        for (int i = 0; i < numBands; i++) {
            bandOutputs[i] = tempSignal / numBands;
        }
    }
    
    // Process each band separately
    for (int band = 0; band < numBands; band++) {
        // Apply compression to each band with its own parameters
        double detectionLevel = std::abs(bandOutputs[band]);  // Use the band signal for detection
        bandSignals[band] = ProcessBand(band, bandOutputs[band], detectionLevel);
        
        // Apply solo/mute settings
        if (bandMuted[band]) {
            bandSignals[band] = 0.0;
        } else if (bandSolan[band]) {
            // If solo is enabled, zero out other bands (this happens later in combination)
        }
    }
}

void TubeMultiBandCompressor::ProcessSignal() {
    // Apply crossover filters to separate into bands
    ApplyCrossoverFilters();
    
    // Combine the bands back together
    double combinedSignal = 0.0;
    bool anySolo = false;
    
    // Check if any band is soloed
    for (int band = 0; band < numBands; band++) {
        if (bandSolan[band]) {
            anySolo = true;
            break;
        }
    }
    
    // Combine bands based on solo/mute status
    for (int band = 0; band < numBands; band++) {
        if (anySolo && !bandSolan[band]) {
            // If any band is soloed, only output soloed bands
            continue;
        }
        combinedSignal += bandSignals[band];
    }
    
    // Apply tube characteristics to the combined signal
    double processedSignal = combinedSignal;
    if (tubeCharacteristicsEnabled && !tubes.empty()) {
        auto& tube = tubes[0];
        tube->SetGridVoltage(-1.0 + processedSignal * 0.05);  // Apply signal to grid
        tube->SetPlateVoltage(250.0);  // Set plate voltage
        tube->SetCathodeVoltage(0.0);
        tube->CalculateTubeBehavior();

        // Use the tube's plate current to modify the signal
        double plateCurrent = tube->GetPlateCurrent();
        double tubeFactor = 1.0 + 0.05 * plateCurrent * 0.001;  // Subtle tube effect
        processedSignal = processedSignal * tubeFactor;
        
        // Add subtle harmonic content characteristic of tubes
        processedSignal += 0.02 * processedSignal * processedSignal * (processedSignal > 0 ? 1 : -1);
    }
    
    // Apply overall gain and makeup gain
    double gainMultiplier = pow(10.0, overallGain / 20.0) * pow(10.0, makeupGain / 20.0);
    outputSignal = processedSignal * gainMultiplier;
    
    // Apply final limiting to prevent clipping
    if (outputSignal > 5.0) outputSignal = 5.0;
    if (outputSignal < -5.0) outputSignal = -5.0;
}

void TubeMultiBandCompressor::ApplyTubeCharacteristics() {
    // This is called by ProcessSignal but the tube characteristics are already applied there
    // This function is just for the interface consistency
}

void TubeMultiBandCompressor::SetBandThreshold(int band, double threshold) {
    if (band >= 0 && band < numBands) {
        thresholds[band] = std::max(-40.0, std::min(0.0, threshold));
    }
}

void TubeMultiBandCompressor::SetBandRatio(int band, double ratio) {
    if (band >= 0 && band < numBands) {
        ratios[band] = std::max(1.0, std::min(20.0, ratio));  // 1:1 to 20:1 ratio
    }
}

void TubeMultiBandCompressor::SetBandAttackTime(int band, double time) {
    if (band >= 0 && band < numBands) {
        attackTimes[band] = std::max(0.0001, std::min(0.1, time)); // 0.1ms to 100ms
        attackCoeffs[band] = exp(-1.0 / (attackTimes[band] * sampleRate));
    }
}

void TubeMultiBandCompressor::SetBandReleaseTime(int band, double time) {
    if (band >= 0 && band < numBands) {
        releaseTimes[band] = std::max(0.001, std::min(1.0, time)); // 1ms to 1s
        releaseCoeffs[band] = exp(-1.0 / (releaseTimes[band] * sampleRate));
    }
}

void TubeMultiBandCompressor::SetMakeupGain(double gain) {
    this->makeupGain = std::max(-20.0, std::min(20.0, gain));
}

void TubeMultiBandCompressor::SetCrossoverFreq(int band, double freq) {
    if (band >= 0 && band < numBands - 1) {  // There are numBands-1 crossover points
        crossoverFreqs[band] = std::max(20.0, std::min(20000.0, freq));
        CalculateFilterCoeffs(band, freq);
    }
}

void TubeMultiBandCompressor::SetBandSolo(int band, bool solo) {
    if (band >= 0 && band < numBands) {
        bandSolan[band] = solo;
    }
}

void TubeMultiBandCompressor::SetBandMute(int band, bool mute) {
    if (band >= 0 && band < numBands) {
        bandMuted[band] = mute;
    }
}

void TubeMultiBandCompressor::SetOverallGain(double gain) {
    this->overallGain = std::max(-20.0, std::min(20.0, gain));
}

double TubeMultiBandCompressor::GetBandThreshold(int band) const {
    if (band >= 0 && band < numBands) {
        return thresholds[band];
    }
    return 0.0;
}

double TubeMultiBandCompressor::GetBandRatio(int band) const {
    if (band >= 0 && band < numBands) {
        return ratios[band];
    }
    return 0.0;
}

double TubeMultiBandCompressor::GetBandAttackTime(int band) const {
    if (band >= 0 && band < numBands) {
        return attackTimes[band];
    }
    return 0.0;
}

double TubeMultiBandCompressor::GetBandReleaseTime(int band) const {
    if (band >= 0 && band < numBands) {
        return releaseTimes[band];
    }
    return 0.0;
}

double TubeMultiBandCompressor::GetCrossoverFreq(int band) const {
    if (band >= 0 && band < numBands - 1) {
        return crossoverFreqs[band];
    }
    return 0.0;
}

bool TubeMultiBandCompressor::GetBandSolo(int band) const {
    if (band >= 0 && band < numBands) {
        return bandSolan[band];
    }
    return false;
}

bool TubeMultiBandCompressor::GetBandMute(int band) const {
    if (band >= 0 && band < numBands) {
        return bandMuted[band];
    }
    return false;
}

TubeMultiBandCompressor::~TubeMultiBandCompressor() {
    // Destructor handled by member destructors
}