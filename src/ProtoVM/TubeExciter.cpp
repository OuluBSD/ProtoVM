#include "TubeExciter.h"
#include "Common.h"
#include <algorithm>
#include <cmath>

// TubeExciter implementation
TubeExciter::TubeExciter(ExciterType type) : exciterType(type) {
    InitializeExciter(type);

    // Initialize with tubes for excitation simulation
    for (int i = 0; i < 2; i++) {
        tubes.push_back(std::make_unique<Triode>(100.0, 100000.0, 1.6e-3));  // Tube simulation
    }
    
    // Initialize harmonics vector
    harmonics.resize(8, 0.0);  // Up to 8 harmonics

    // Calculate formant filter coefficients
    CalculateFormantCoeffs();
}

void TubeExciter::InitializeExciter(ExciterType type) {
    switch (type) {
        case HARMONIC_EXCITER:
            amount = 0.6;              // Medium harmonic enhancement
            oddEvenBalance = 0.2;      // Slightly more odd harmonics (tube-like)
            lowFreq = 80.0;            // Start from bass frequencies
            highFreq = 8000.0;         // Up to presence range
            harmonicOrder = 6;         // Use up to 6th harmonics
            toneControl = 0.6;         // Slightly bright
            drive = 0.4;               // Medium drive for harmonic generation
            dryWetMix = 0.4;           // 40% wet signal
            tubeGain = 25.0;           // High tube gain for harmonics
            break;

        case FORMANT_EXCITER:
            amount = 0.7;              // High formant control amount
            oddEvenBalance = 0.0;      // Balanced harmonics
            lowFreq = 200.0;           // Formants typically in vocal range
            highFreq = 4000.0;         // Up to presence
            harmonicOrder = 4;         // Lower harmonic order for formants
            toneControl = 0.5;         // Neutral tone
            formantFreq = 1000.0;      // Default formant frequency
            formantQ = 2.0;            // Medium Q for formant
            drive = 0.2;               // Low drive to preserve formant clarity
            dryWetMix = 0.5;           // Balanced mix
            tubeGain = 20.0;           // Medium tube gain
            formantControlEnabled = true;
            break;

        case PRESENCE_BOOSTER:
            amount = 0.5;              // Medium presence boost
            oddEvenBalance = -0.3;     // More even harmonics for presence
            lowFreq = 3000.0;          // High frequencies only
            highFreq = 15000.0;        // Up to upper presence
            harmonicOrder = 3;         // Lower harmonics for presence
            toneControl = 0.8;         // Bright tone control
            drive = 0.3;               // Low drive to avoid harshness
            dryWetMix = 0.3;           // 30% wet signal to avoid harshness
            tubeGain = 30.0;           // High gain for presence
            break;

        case TUBESATURATION_EXCITER:
            amount = 0.8;              // High saturation amount
            oddEvenBalance = 0.4;      // More odd harmonics for tube character
            lowFreq = 50.0;            // Full frequency range
            highFreq = 10000.0;        // Up to upper midrange
            harmonicOrder = 5;         // Medium harmonic order
            toneControl = 0.5;         // Neutral tone
            drive = 0.6;               // High drive for saturation
            dryWetMix = 0.6;           // Higher wet signal for saturation
            tubeGain = 35.0;           // High tube gain for saturation
            break;
    }
}

bool TubeExciter::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeExciter::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == inputPin && data_bytes == sizeof(double)) {
        memcpy(&inputSignal, data, sizeof(double));
        return true;
    } else if (conn_id == controlPin && data_bytes == sizeof(double)) {
        memcpy(&controlSignal, data, sizeof(double));
        // Control signal can affect excitation amount
        SetAmount(amount + 0.5 * controlSignal);  // Map -1,1 to ±0.5 change in amount
        return true;
    }
    return false;
}

bool TubeExciter::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == outputPin && data_bytes == sizeof(double)) {
        memcpy(data, &outputSignal, sizeof(double));
        return true;
    }
    return false;
}

bool TubeExciter::Tick() {
    ProcessSignal();
    return true;
}

double TubeExciter::EstimateFrequency() {
    // Simple frequency estimation based on zero crossings
    static double prevSignal = 0.0;
    static int zeroCrossings = 0;
    static double timeSinceZeroCrossing = 0.0;
    
    // Count zero crossings
    if ((inputSignal >= 0.0 && prevSignal < 0.0) || (inputSignal < 0.0 && prevSignal >= 0.0)) {
        // Zero crossing detected
        if (timeSinceZeroCrossing > 0.0) {
            // Calculate frequency based on time since last zero crossing
            double estimatedFreq = 0.5 / timeSinceZeroCrossing;
            if (estimatedFreq > 50.0 && estimatedFreq < 3000.0) {  // Valid range for musical tones
                currentFreqEstimate = estimatedFreq;
            }
        }
        timeSinceZeroCrossing = 0.0;
    } else {
        timeSinceZeroCrossing += 1.0 / sampleRate;
    }
    
    prevSignal = inputSignal;
    
    return currentFreqEstimate;
}

void TubeExciter::GenerateHarmonics() {
    // Estimate the fundamental frequency
    double fundamental = EstimateFrequency();
    
    // Generate harmonics based on settings
    for (int i = 0; i < harmonicOrder && i < harmonics.size(); i++) {
        double harmonicFreq = fundamental * (i + 1);
        
        // Skip if harmonic is outside the processing range
        if (harmonicFreq < lowFreq || harmonicFreq > highFreq) {
            harmonics[i] = 0.0;
            continue;
        }
        
        // Calculate harmonic amplitude based on balance settings
        double harmonicAmplitude = 1.0 / (i + 1);  // Decreasing with harmonic number
        
        // Apply odd/even balance
        if ((i + 1) % 2 == 1) {
            // Odd harmonic
            harmonicAmplitude *= (1.0 + oddEvenBalance);
        } else {
            // Even harmonic
            harmonicAmplitude *= (1.0 - oddEvenBalance);
        }
        
        harmonics[i] = harmonicAmplitude;
    }
}

double TubeExciter::ApplyFormantFilter(double input) {
    if (!formantControlEnabled) return input;
    
    static double x1 = 0.0, x2 = 0.0, y1 = 0.0, y2 = 0.0;
    
    // Apply biquad filter using formant coefficients
    double output = formant_b0 * input + formant_b1 * x1 + formant_b2 * x2 - formant_a1 * y1 - formant_a2 * y2;
    
    x2 = x1; x1 = input;
    y2 = y1; y1 = output;
    
    return output;
}

double TubeExciter::ApplyHarmonicEnhancement(double input) {
    // Apply harmonic enhancement based on generated harmonics
    double enhancedSignal = input;  // Start with original signal
    
    // Add harmonics
    double inputPhase = 0.0;
    static double accumulatedPhase = 0.0;
    
    // Calculate phase based on estimated frequency
    if (currentFreqEstimate > 0.0) {
        accumulatedPhase += 2.0 * M_PI * currentFreqEstimate / sampleRate;
        if (accumulatedPhase > 2.0 * M_PI) accumulatedPhase -= 2.0 * M_PI;
        inputPhase = accumulatedPhase;
    }
    
    double harmonicSignal = 0.0;
    for (int i = 0; i < harmonicOrder && i < harmonics.size(); i++) {
        if (harmonics[i] > 0.0) {
            // Generate harmonic at appropriate frequency and amplitude
            double harmonic = sin((i + 1) * inputPhase) * harmonics[i];
            harmonicSignal += harmonic * 0.15;  // Scale harmonics appropriately
        }
    }
    
    // Mix original with harmonics based on amount and drive settings
    return input * (1.0 - amount * 0.7) + harmonicSignal * amount * drive;
}

void TubeExciter::CalculateFormantCoeffs() {
    if (!formantControlEnabled) return;
    
    // Calculate biquad coefficients for peak filter (formant)
    double dt = 1.0 / sampleRate;
    double omega = 2.0 * M_PI * formantFreq * dt;
    double sn = sin(omega);
    double cs = cos(omega);
    double alpha = sn / (2.0 * formantQ);

    // Calculate coefficients for peak filter
    double A = sqrt(pow(10.0, amount));  // Convert amount to amplitude ratio
    double b0 = 1.0 + (alpha * A);
    double b1 = -2.0 * cs;
    double b2 = 1.0 - (alpha * A);
    double a0 = 1.0 + (alpha / A);
    double a1 = -2.0 * cs;
    double a2 = 1.0 - (alpha / A);

    formant_b0 = b0 / a0;
    formant_b1 = b1 / a0;
    formant_b2 = b2 / a0;
    formant_a1 = a1 / a0;
    formant_a2 = a2 / a0;
}

void TubeExciter::ProcessSignal() {
    // Generate harmonics based on input
    GenerateHarmonics();
    
    // Apply harmonic enhancement
    double enhancedSignal = ApplyHarmonicEnhancement(inputSignal);
    
    // Apply formant filter if enabled
    if (formantControlEnabled) {
        enhancedSignal = ApplyFormantFilter(enhancedSignal);
    }
    
    // Apply tube characteristics
    double tubeProcessed = enhancedSignal;
    if (tubeCharacteristicsEnabled && !tubes.empty()) {
        auto& tube = tubes[0];
        tube->SetGridVoltage(-1.0 + tubeProcessed * drive * 0.1);  // Apply signal to grid with drive
        tube->SetPlateVoltage(250.0);  // Set plate voltage
        tube->SetCathodeVoltage(0.0);
        tube->CalculateTubeBehavior();

        // Use the tube's plate current to modify the signal
        double plateCurrent = tube->GetPlateCurrent();
        double tubeFactor = tubeGain * 0.001;  // Scale tube gain appropriately
        tubeProcessed = tubeProcessed + plateCurrent * tubeFactor;
        
        // Add subtle harmonic content characteristic of tubes (even-order harmonics)
        double tubeHarmonics = tubeProcessed * tubeProcessed * (tubeProcessed > 0 ? 1 : -1) * 0.02;
        tubeProcessed = tubeProcessed * (1.0 - amount * 0.3) + tubeHarmonics * amount * 0.3;
    }
    
    // Apply tone control (EQ-style adjustment)
    double toneAdjusted = tubeProcessed;
    if (toneControl < 0.5) {
        // Apply high-cut if tone is less than neutral
        static double lpState = 0.0;
        double cutoff = 5000.0 * (1.0 - (toneControl * 2.0));  // From 5kHz to DC
        double dt = 1.0 / sampleRate;
        double rc = 1.0 / (2.0 * M_PI * cutoff);
        double coeff = dt / (rc + dt);
        
        lpState = lpState + coeff * (toneAdjusted - lpState);
        toneAdjusted = lpState;
    } else if (toneControl > 0.5) {
        // Apply presence boost if tone is more than neutral
        static double hpState = 0.0;
        double cutoff = 2000.0 * ((toneControl - 0.5) * 2.0);  // From DC to 2kHz
        double dt = 1.0 / sampleRate;
        double rc = 1.0 / (2.0 * M_PI * cutoff);
        double coeff = rc / (rc + dt);
        
        double inputForHP = toneAdjusted;
        toneAdjusted = coeff * (toneAdjusted + hpState - inputForHP);
        hpState = toneAdjusted;
    }
    
    // Apply dry/wet mix
    outputSignal = inputSignal * (1.0 - dryWetMix) + toneAdjusted * dryWetMix;
    
    // Apply final limiting to prevent clipping
    if (outputSignal > 5.0) outputSignal = 5.0;
    if (outputSignal < -5.0) outputSignal = -5.0;
    
    // Update previous values for differentiation
    prevInput = inputSignal;
    prevOutput = outputSignal;
}

void TubeExciter::ApplyTubeCharacteristics() {
    // This is called by ProcessSignal but the tube characteristics are already applied there
    // This function is just for the interface consistency
}

void TubeExciter::SetAmount(double amount) {
    this->amount = std::max(0.0, std::min(1.0, amount));
    if (formantControlEnabled) {
        CalculateFormantCoeffs();  // Update formant coefficients if needed
    }
}

void TubeExciter::SetOddEvenBalance(double balance) {
    this->oddEvenBalance = std::max(-1.0, std::min(1.0, balance));
}

void TubeExciter::SetFrequencyRange(double low, double high) {
    this->lowFreq = std::max(20.0, std::min(20000.0, low));
    this->highFreq = std::max(lowFreq, std::min(20000.0, high));
}

void TubeExciter::SetHarmonicOrder(int order) {
    this->harmonicOrder = std::max(1, std::min(8, order));
}

void TubeExciter::SetToneControl(double tone) {
    this->toneControl = std::max(0.0, std::min(1.0, tone));
}

void TubeExciter::SetFormantFrequency(double freq) {
    this->formantFreq = std::max(100.0, std::min(8000.0, freq));
    if (formantControlEnabled) {
        CalculateFormantCoeffs();  // Update formant coefficients
    }
}

void TubeExciter::SetFormantQ(double q) {
    this->formantQ = std::max(0.5, std::min(10.0, q));
    if (formantControlEnabled) {
        CalculateFormantCoeffs();  // Update formant coefficients
    }
}

void TubeExciter::SetDrive(double drive) {
    this->drive = std::max(0.0, std::min(1.0, drive));
}

void TubeExciter::SetMix(double dryWet) {
    this->dryWetMix = std::max(0.0, std::min(1.0, dryWet));
}

TubeExciter::~TubeExciter() {
    // Destructor handled by member destructors
}