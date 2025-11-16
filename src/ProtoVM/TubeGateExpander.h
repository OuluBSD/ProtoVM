#ifndef _ProtoVM_TubeGateExpander_h_
#define _ProtoVM_TubeGateExpander_h_

#include "AnalogCommon.h"
#include "TubeModels.h"
#include "AnalogComponents.h"
#include <vector>
#include <memory>
#include <cmath>

// Class for tube-based gate/expander circuits for noise reduction applications
class TubeGateExpander : public ElectricNodeBase {
public:
    typedef TubeGateExpander CLASSNAME;

    enum GateType {
        NOISE_GATE,          // Standard noise gate
        BANDPASS_GATE,       // Multiband gate with bandpass filtering
        DOWNWARD_EXPANDER,   // Downward expander
        UPWARD_EXPANDER      // Upward expander (rare in practice)
    };

    TubeGateExpander(GateType type = NOISE_GATE);
    virtual ~TubeGateExpander();

    virtual bool Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool Tick() override;

    // Configuration methods for gate/expander parameters
    void SetThreshold(double threshold);     // in dB (typically negative)
    void SetRatio(double ratio);             // expansion ratio (e.g., 2.0 for 2:1)
    void SetAttackTime(double time);         // in seconds
    void SetReleaseTime(double time);        // in seconds
    void SetHoldTime(double time);           // hold time in seconds
    void SetRange(double range);             // maximum gain reduction in dB
    void SetHysteresis(double hysteresis);   // hysteresis in dB for cleaner gating
    void SetMakeupGain(double gain);         // makeup gain in dB
    void SetSidechainFilterFreq(double freq); // Sidechain filter frequency

    // Get parameters
    double GetThreshold() const { return threshold; }
    double GetRatio() const { return ratio; }
    double GetAttackTime() const { return attackTime; }
    double GetReleaseTime() const { return releaseTime; }
    double GetHoldTime() const { return holdTime; }
    double GetRange() const { return range; }
    double GetHysteresis() const { return hysteresis; }
    double GetMakeupGain() const { return makeupGain; }
    double GetSidechainFilterFreq() const { return sidechainFilterFreq; }

    // Enable/disable features
    void EnableAutoRelease(bool enable) { autoReleaseEnabled = enable; }
    void EnableLookAhead(bool enable) { lookAheadEnabled = enable; }
    void EnableTubeCharacteristics(bool enable) { tubeCharacteristicsEnabled = enable; }

private:
    GateType gateType;

    // Gate/expander parameters
    double threshold = -26.0;   // Threshold in dB (typically more negative than compression)
    double ratio = 3.0;         // Expansion ratio (e.g., 3:1)
    double attackTime = 0.005;  // Attack time in seconds (faster than compression)
    double releaseTime = 0.15;  // Release time in seconds (slower than compression)
    double holdTime = 0.1;      // Hold time in seconds (time to stay closed)
    double range = -24.0;       // Maximum gain reduction in dB
    double hysteresis = 2.0;    // Hysteresis for cleaner gating
    double makeupGain = 0.0;    // Makeup gain in dB
    double sidechainFilterFreq = 100.0; // Sidechain filter frequency in Hz

    // State variables
    double detectorLevel = 0.0;     // Detected level for gating
    double gainReduction = 1.0;     // Current gain reduction factor
    double prevGainReduction = 1.0; // Previous gain reduction (for smooth transitions)
    double attackCoeff = 0.0;       // Coefficient for attack calculation
    double releaseCoeff = 0.0;      // Coefficient for release calculation
    double holdTimer = 0.0;         // Timer for hold state
    bool gateOpen = true;           // Whether the gate is currently open

    // Tube simulation parameters
    std::vector<std::unique_ptr<Tube>> tubes;
    double tubeGain = 15.0;         // Base tube gain

    // Circuit type parameters
    bool autoReleaseEnabled = true;    // Adaptive release
    bool lookAheadEnabled = false;     // Enable look-ahead gating
    bool tubeCharacteristicsEnabled = true; // Apply tube characteristics

    // Sample rate for time constants
    double sampleRate = 44100.0;

    // Pin connections
    int inputPin = 0;
    int outputPin = 1;
    int controlPin = 2;           // For external control of gating
    int sidechainPin = 3;         // For external sidechain input

    double inputSignal = 0.0;
    double outputSignal = 0.0;
    double controlSignal = 0.0;
    double sidechainSignal = 0.0;

    // Look-ahead buffer
    std::vector<double> lookAheadBuffer;
    int lookAheadBufferSize = 0;
    int lookAheadWritePos = 0;

    // Initialize gate/expander based on type
    void InitializeGate(GateType type);

    // Process signal through gating/expansion algorithm
    void ProcessSignal();

    // Update the detector level
    void UpdateDetector();

    // Calculate expansion/gating gain based on input level
    double CalculateExpansionGain(double inputLevel);

    // Apply tube characteristics
    void ApplyTubeCharacteristics();
};

#endif