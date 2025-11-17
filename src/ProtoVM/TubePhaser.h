#ifndef _ProtoVM_TubePhaser_h_
#define _ProtoVM_TubePhaser_h_

#include "AnalogCommon.h"
#include "TubeModels.h"
#include "AnalogComponents.h"
#include "LFO.h"
#include <vector>
#include <memory>
#include <cmath>

// Class for tube-based phaser circuits for phase-shift modulation effects
class TubePhaser : public ElectricNodeBase {
public:
    typedef TubePhaser CLASSNAME;

    enum PhaserType {
        TRANSISTOR_STYLE,    // Classic 4-stage transistor phaser
        TUBE_STYLE,          // Tube-based phaser with tube stages
        MULTI_STAGE,         // 8+ stage phaser
        AUTO_WAH_STYLE       // Auto-wah with phaser characteristics
    };

    TubePhaser(PhaserType type = TUBE_STYLE);
    virtual ~TubePhaser();

    virtual bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    /*virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;*/
    virtual bool Tick() override;

    // Configuration methods for phaser parameters
    void SetLFOFrequency(double freq);           // LFO frequency in Hz
    void SetLFOAmount(double amount);            // LFO depth (0.0 to 1.0)
    void SetFeedback(double feedback);           // Feedback amount (-0.9 to 0.9)
    void SetStageCount(int count);               // Number of allpass stages (2 to 12)
    void SetNotchCount(int count);               // Number of notches created
    void SetCenterFrequency(double freq);        // Center frequency of phase shift
    void SetPhaseDepth(double depth);            // Phase depth (0.0 to 1.0)
    void SetMix(double dryWet);                  // Dry/wet mix (0.0 to 1.0)
    void SetEnvelopeAmount(double amount);       // Envelope follower amount

    // Get parameters
    double GetLFOFrequency() const { return lfoFrequency; }
    double GetLFOAmount() const { return lfoAmount; }
    double GetFeedback() const { return feedback; }
    int GetStageCount() const { return stageCount; }
    int GetNotchCount() const { return notchCount; }
    double GetCenterFrequency() const { return centerFrequency; }
    double GetPhaseDepth() const { return phaseDepth; }
    double GetMix() const { return dryWetMix; }
    double GetEnvelopeAmount() const { return envelopeAmount; }

    // Enable/disable features
    void EnableEnvelopeFollower(bool enable) { envelopeFollowerEnabled = enable; }
    void EnableTubeCharacteristics(bool enable) { tubeCharacteristicsEnabled = enable; }

private:
    PhaserType phaserType;

    // Phaser parameters
    double lfoFrequency = 0.5;          // LFO frequency in Hz
    double lfoAmount = 0.7;             // LFO depth (0.0 to 1.0)
    double feedback = 0.3;              // Feedback amount (-0.9 to 0.9)
    int stageCount = 4;                 // Number of allpass stages (2 to 12)
    int notchCount = 4;                 // Number of notches created
    double centerFrequency = 1000.0;    // Center frequency of phase shift (Hz)
    double phaseDepth = 0.8;            // Phase depth (0.0 to 1.0)
    double dryWetMix = 0.5;             // Dry/wet mix (0.0 to 1.0)
    double envelopeAmount = 0.0;        // Envelope follower amount (0.0 to 1.0)

    // Allpass filter stages
    std::vector<double> allpassX;       // Delayed input values for allpass stages
    std::vector<double> allpassY;       // Delayed output values for allpass stages
    std::vector<double> allpassCoeffs;  // Coefficients for each allpass stage
    
    // Modulation components
    std::unique_ptr<LFO> modulationLFO; // For LFO-based phasing
    double lfoPhase = 0.0;              // Current LFO phase
    
    // Envelope follower for auto-wah effects
    double envelopeDetector = 0.0;      // Detected envelope level
    double envelopeCoeff = 0.0;         // Coefficient for envelope tracking
    
    // Feedback parameters
    double feedbackBuffer = 0.0;        // Feedback signal buffer
    
    // Tube simulation parameters
    std::vector<std::unique_ptr<Tube>> tubes;
    double tubeGain = 20.0;             // Base tube gain for phasing

    // Circuit parameters
    bool envelopeFollowerEnabled = false;  // Enable envelope follower
    bool tubeCharacteristicsEnabled = true; // Apply tube characteristics

    // Sample rate for calculations
    double sampleRate = 44100.0;

    // Pin connections
    int inputPin = 0;
    int outputPin = 1;
    int controlPin = 2;                 // For external control of phasing

    double inputSignal = 0.0;
    double outputSignal = 0.0;
    double controlSignal = 0.0;

    // Initialize phaser based on type
    void InitializePhaser(PhaserType type);

    // Process signal through phasing algorithm
    void ProcessSignal();

    // Process a single allpass stage
    double ProcessAllpassStage(int stage, double input, double coefficient);

    // Update allpass coefficients based on LFO
    void UpdateCoefficients();

    // Update envelope detector
    void UpdateEnvelopeDetector();

    // Apply tube characteristics
    void ApplyTubeCharacteristics();
};

#endif