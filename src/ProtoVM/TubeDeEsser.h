#ifndef _ProtoVM_TubeDeEsser_h_
#define _ProtoVM_TubeDeEsser_h_

#include "AnalogCommon.h"
#include "TubeModels.h"
#include "AnalogComponents.h"
#include <vector>
#include <memory>
#include <cmath>

// Class for tube-based de-esser circuits for vocal sibilance control
class TubeDeEsser : public ElectricNodeBase {
public:
    typedef TubeDeEsser CLASSNAME;

    enum DeEsserType {
        BANDPASS_DESSER,        // Uses bandpass filter to detect sibilance
        HIGH_FREQ_DESSER,       // Focuses on high-frequency sibilance
        ADAPTIVE_DESSER,        // Adapts to different vocal characteristics
        PROPORTIONAL_DESSER     // Proportional reduction based on sibilance level
    };

    TubeDeEsser(DeEsserType type = BANDPASS_DESSER);
    virtual ~TubeDeEsser();

    virtual bool Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool Tick() override;

    // Configuration methods for de-esser parameters
    void SetThreshold(double threshold);         // in dB (for sibilance detection)
    void SetRatio(double ratio);                 // reduction ratio (e.g., 4.0 for 4:1)
    void SetAttackTime(double time);             // in seconds
    void SetReleaseTime(double time);            // in seconds
    void SetFrequency(double freq);              // center frequency for sibilance detection (Hz)
    void SetBandwidth(double bandwidth);         // bandwidth for detection (Hz)
    void SetMakeupGain(double gain);             // makeup gain in dB
    void SetSidechainRatio(double ratio);        // sidechain processing ratio
    void SetHighFreqThreshold(double threshold); // high-frequency detection threshold

    // Get parameters
    double GetThreshold() const { return threshold; }
    double GetRatio() const { return ratio; }
    double GetAttackTime() const { return attackTime; }
    double GetReleaseTime() const { return releaseTime; }
    double GetFrequency() const { return centerFreq; }
    double GetBandwidth() const { return bandwidth; }
    double GetMakeupGain() const { return makeupGain; }
    double GetSidechainRatio() const { return sidechainRatio; }
    double GetHighFreqThreshold() const { return highFreqThreshold; }

    // Enable/disable features
    void EnableAdaptiveMode(bool enable) { adaptiveMode = enable; }
    void EnableTubeCharacteristics(bool enable) { tubeCharacteristicsEnabled = enable; }

private:
    DeEsserType desserType;

    // De-esser parameters
    double threshold = -12.0;       // Threshold in dB for sibilance detection
    double ratio = 4.0;             // Reduction ratio (e.g., 4:1)
    double attackTime = 0.002;      // Attack time in seconds (very fast for sibilance)
    double releaseTime = 0.025;     // Release time in seconds (faster than compression)
    double centerFreq = 5000.0;     // Center frequency for sibilance detection (Hz)
    double bandwidth = 1500.0;      // Bandwidth around center frequency (Hz)
    double makeupGain = 0.0;        // Makeup gain in dB
    double sidechainRatio = 0.5;    // Sidechain processing ratio (0.0 to 1.0)
    double highFreqThreshold = -15.0; // High-frequency detection threshold

    // State variables
    double detectorLevel = 0.0;       // Detected sibilance level
    double highFreqDetector = 0.0;    // High-frequency detector level
    double gainReduction = 1.0;       // Current gain reduction factor
    double prevGainReduction = 1.0;   // Previous gain reduction (for smooth transitions)
    double attackCoeff = 0.0;         // Coefficient for attack calculation
    double releaseCoeff = 0.0;        // Coefficient for release calculation
    double sibilanceDetected = 0.0;   // How much sibilance is detected

    // Tube simulation parameters
    std::vector<std::unique_ptr<Tube>> tubes;
    double tubeGain = 18.0;           // Base tube gain for de-essing

    // Bandpass filter coefficients for sibilance detection
    double bp_a1 = 0.0, bp_a2 = 0.0, bp_b0 = 0.0, bp_b1 = 0.0, bp_b2 = 0.0;

    // Circuit type parameters
    bool adaptiveMode = false;           // Adaptive processing mode
    bool tubeCharacteristicsEnabled = true; // Apply tube characteristics

    // Sample rate for time constants
    double sampleRate = 44100.0;

    // Pin connections
    int inputPin = 0;
    int outputPin = 1;
    int controlPin = 2;                 // For external control of de-essing
    int sidechainPin = 3;               // For external sidechain input

    double inputSignal = 0.0;
    double outputSignal = 0.0;
    double controlSignal = 0.0;
    double sidechainSignal = 0.0;

    // Initialize de-esser based on type
    void InitializeDeEsser(DeEsserType type);

    // Process signal through de-essing algorithm
    void ProcessSignal();

    // Update the detector level with bandpass filtering
    void UpdateDetector();

    // Calculate de-essing gain based on sibilance level
    double CalculateDeEsserGain(double inputLevel);

    // Apply bandpass filter to detect sibilance
    double ApplyBandpassFilter(double input);

    // Apply tube characteristics
    void ApplyTubeCharacteristics();
};

#endif