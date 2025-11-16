#ifndef _ProtoVM_TubeExciter_h_
#define _ProtoVM_TubeExciter_h_

#include "AnalogCommon.h"
#include "TubeModels.h"
#include "AnalogComponents.h"
#include <vector>
#include <memory>
#include <cmath>

// Class for tube-based exciter circuits with formant control for tonal shaping
class TubeExciter : public ElectricNodeBase {
public:
    typedef TubeExciter CLASSNAME;

    enum ExciterType {
        HARMONIC_EXCITER,      // Adds harmonic content
        FORMANT_EXCITER,       // Formant-based tonal shaping
        PRESENCE_BOOSTER,      // Enhances high-frequency presence
        TUBESATURATION_EXCITER // Tube saturation-based exciter
    };

    TubeExciter(ExciterType type = HARMONIC_EXCITER);
    virtual ~TubeExciter();

    virtual bool Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool Tick() override;

    // Configuration methods for exciter parameters
    void SetAmount(double amount);                    // Exciter amount (0.0 to 1.0)
    void SetOddEvenBalance(double balance);           // Odd/Even harmonic balance (-1.0 to 1.0)
    void SetFrequencyRange(double low, double high);  // Frequency range in Hz
    void SetHarmonicOrder(int order);                 // Harmonic order (1 to 8)
    void SetToneControl(double tone);                 // General tone control
    void SetFormantFrequency(double freq);            // Formant frequency (for FORMANT_EXCITER)
    void SetFormantQ(double q);                       // Formant Q factor (for FORMANT_EXCITER)
    void SetDrive(double drive);                      // Drive/saturation amount
    void SetMix(double dryWet);                       // Dry/wet mix (0.0 to 1.0)

    // Get parameters
    double GetAmount() const { return amount; }
    double GetOddEvenBalance() const { return oddEvenBalance; }
    double GetLowFreq() const { return lowFreq; }
    double GetHighFreq() const { return highFreq; }
    int GetHarmonicOrder() const { return harmonicOrder; }
    double GetToneControl() const { return toneControl; }
    double GetFormantFrequency() const { return formantFreq; }
    double GetFormantQ() const { return formantQ; }
    double GetDrive() const { return drive; }
    double GetMix() const { return dryWetMix; }

    // Enable/disable features
    void EnableTubeCharacteristics(bool enable) { tubeCharacteristicsEnabled = enable; }
    void EnableFormantControl(bool enable) { formantControlEnabled = enable; }

private:
    ExciterType exciterType;

    // Exciter parameters
    double amount = 0.5;                // Exciter amount (0.0 to 1.0)
    double oddEvenBalance = 0.0;        // Odd/Even harmonic balance (-1.0 to 1.0)
    double lowFreq = 20.0;              // Low frequency for processing (Hz)
    double highFreq = 20000.0;          // High frequency for processing (Hz)
    int harmonicOrder = 4;              // Harmonic order (1 to 8)
    double toneControl = 0.5;           // General tone control (0.0 to 1.0)
    double formantFreq = 1000.0;        // Formant frequency (Hz) for formant exciter
    double formantQ = 2.0;              // Formant Q factor
    double drive = 0.3;                 // Drive/saturation amount (0.0 to 1.0)
    double dryWetMix = 0.5;             // Dry/wet mix (0.0 to 1.0)

    // Processing parameters
    double currentFreqEstimate = 440.0; // Estimated fundamental frequency
    std::vector<double> harmonics;      // Generated harmonics
    double prevInput = 0.0;             // Previous input for differentiation
    double prevOutput = 0.0;            // Previous output for feedback

    // Tube simulation parameters
    std::vector<std::unique_ptr<Tube>> tubes;
    double tubeGain = 25.0;             // Base tube gain for excitation

    // Formant filter coefficients
    double formant_a1 = 0.0, formant_a2 = 0.0, formant_b0 = 0.0, formant_b1 = 0.0, formant_b2 = 0.0;
    
    // Circuit parameters
    bool tubeCharacteristicsEnabled = true;  // Apply tube characteristics
    bool formantControlEnabled = false;      // Enable formant control
    bool adaptiveProcessing = true;          // Enable adaptive processing

    // Sample rate for calculations
    double sampleRate = 44100.0;

    // Pin connections
    int inputPin = 0;
    int outputPin = 1;
    int controlPin = 2;                  // For external control of excitation

    double inputSignal = 0.0;
    double outputSignal = 0.0;
    double controlSignal = 0.0;

    // Initialize exciter based on type
    void InitializeExciter(ExciterType type);

    // Process signal through excitation algorithm
    void ProcessSignal();

    // Estimate fundamental frequency
    double EstimateFrequency();

    // Generate harmonics based on input and settings
    void GenerateHarmonics();

    // Apply formant filter
    double ApplyFormantFilter(double input);

    // Apply harmonic enhancement
    double ApplyHarmonicEnhancement(double input);

    // Apply tube characteristics
    void ApplyTubeCharacteristics();

    // Calculate formant filter coefficients
    void CalculateFormantCoeffs();
};

#endif