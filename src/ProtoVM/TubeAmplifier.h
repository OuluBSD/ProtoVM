#ifndef _ProtoVM_TubeAmplifier_h_
#define _ProtoVM_TubeAmplifier_h_

#include "AnalogCommon.h"
#include "TubeModels.h"
#include <vector>

// Enum for different amplifier classes
enum class AmplifierClass {
    CLASS_A,      // Single-ended, always conducting
    CLASS_AB,     // Push-pull, conducts >180 degrees
    CLASS_B,      // Push-pull, conducts 180 degrees
    CLASS_C       // Conducts <180 degrees, high efficiency
};

// Enum for tube configuration
enum class TubeConfiguration {
    SINGLE_ENDED,     // Single tube configuration
    PUSH_PULL,        // Two tubes in push-pull
    DIFFERENTIAL,     // Two tubes in differential pair
    CASCADE,          // Multiple gain stages
    CASCODE           // Common emitter followed by common base
};

// Tube amplifier circuit
class TubeAmplifier : public AnalogNodeBase {
public:
    typedef TubeAmplifier CLASSNAME;

    TubeAmplifier(int num_tubes = 1, 
                  AmplifierClass amp_class = AmplifierClass::CLASS_A,
                  TubeConfiguration config = TubeConfiguration::SINGLE_ENDED);
    virtual ~TubeAmplifier();

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "TubeAmplifier"; }

    // Add a tube to the amplifier
    void AddTube(std::unique_ptr<Tube> tube);
    
    // Get/set input signal
    void SetInputSignal(double signal) { input_signal = signal; }
    double GetInputSignal() const { return input_signal; }
    
    // Get output signal
    double GetOutputSignal() const { return output_signal; }
    
    // Get/set gain
    void SetGain(double gain) { amplifier_gain = std::max(0.1, std::min(100.0, gain)); }
    double GetGain() const { return amplifier_gain; }
    
    // Get/set distortion characteristics
    void SetDistortion(double distortion) { harmonic_distortion = std::max(0.0, std::min(1.0, distortion)); }
    double GetDistortion() const { return harmonic_distortion; }
    
    // Set amplifier class
    void SetAmplifierClass(AmplifierClass cls) { amp_class = cls; }
    AmplifierClass GetAmplifierClass() const { return amp_class; }
    
    // Set configuration
    void SetConfiguration(TubeConfiguration config) { configuration = config; }
    TubeConfiguration GetConfiguration() const { return configuration; }
    
    // Get/set load resistance (in ohms)
    void SetLoadResistance(double resistance) { load_resistance = std::max(100.0, resistance); }
    double GetLoadResistance() const { return load_resistance; }
    
    // Get/set plate voltage
    void SetPlateVoltage(double volts) { plate_voltage = std::max(50.0, std::min(500.0, volts)); }
    double GetPlateVoltage() const { return plate_voltage; }
    
    // Get/set bias voltage
    void SetBiasVoltage(double volts) { bias_voltage = std::max(-10.0, std::min(0.0, volts)); }
    double GetBiasVoltage() const { return bias_voltage; }
    
    // Enable/disable distortion modeling
    void EnableDistortion(bool enable) { distortion_enabled = enable; }
    bool IsDistortionEnabled() const { return distortion_enabled; }
    
    // Get harmonic content
    std::vector<double> GetHarmonicContent() const { return harmonic_content; }
    
    // Calculate total harmonic distortion (THD)
    double CalculateTHD() const;
    
    // Apply tone controls (simplified)
    void SetBassControl(double value) { bass_control = std::max(-1.0, std::min(1.0, value)); }
    void SetMidControl(double value) { mid_control = std::max(-1.0, std::min(1.0, value)); }
    void SetTrebleControl(double value) { treble_control = std::max(-1.0, std::min(1.0, value)); }
    
    double GetBassControl() const { return bass_control; }
    double GetMidControl() const { return mid_control; }
    double GetTrebleControl() const { return treble_control; }

protected:
    std::vector<std::unique_ptr<Tube>> tubes;  // The tubes in this amplifier
    double input_signal;                       // Input signal to the amplifier
    double output_signal;                      // Output signal from the amplifier
    double amplifier_gain;                     // Overall amplifier gain
    double harmonic_distortion;                // Amount of harmonic distortion (0.0 to 1.0)
    AmplifierClass amp_class;                  // Amplifier class (A, AB, B, C)
    TubeConfiguration configuration;           // Tube configuration
    double load_resistance;                    // Load resistance in ohms
    double plate_voltage;                      // Plate voltage
    double bias_voltage;                       // Bias voltage
    bool distortion_enabled;                   // Whether distortion modeling is enabled
    std::vector<double> harmonic_content;      // Harmonic content analysis
    double bass_control;                       // Tone controls
    double mid_control;
    double treble_control;
    
    // Internal method to process the signal through all tubes
    virtual void ProcessSignal();
    
    // Apply distortion to the signal
    virtual double ApplyDistortion(double signal);
    
    // Apply tone controls to the signal
    virtual double ApplyToneControls(double signal);
    
    // Calculate harmonic content
    virtual void CalculateHarmonicContent(double signal);
    
    // Simulate the amplifier class behavior
    virtual void ApplyAmplifierClassCharacteristics();
    
    static constexpr double MIN_LOAD_RESISTANCE = 100.0;  // Minimum load resistance in ohms
    static constexpr double MAX_GAIN = 100.0;             // Maximum gain
    static constexpr int MAX_HARMONICS = 10;              // Number of harmonics to track
};

// Common tube amplifier topologies
class SingleEndedAmp : public TubeAmplifier {
public:
    typedef SingleEndedAmp CLASSNAME;
    
    SingleEndedAmp();
    virtual ~SingleEndedAmp() {}
    
    virtual String GetClassName() const override { return "SingleEndedAmp"; }
    
protected:
    virtual void ProcessSignal() override;
};

class PushPullAmp : public TubeAmplifier {
public:
    typedef PushPullAmp CLASSNAME;
    
    PushPullAmp();
    virtual ~PushPullAmp() {}
    
    virtual String GetClassName() const override { return "PushPullAmp"; }
    
protected:
    virtual void ProcessSignal() override;
};

class ClassAChampAmp : public TubeAmplifier {
public:
    typedef ClassAChampAmp CLASSNAME;
    
    // Fender Champ-style amplifier (single-ended Class A)
    ClassAChampAmp();
    virtual ~ClassAChampAmp() {}
    
    virtual String GetClassName() const override { return "ClassAChampAmp"; }
    
protected:
    virtual void ProcessSignal() override;
};

class ClassABFenderTwinAmp : public TubeAmplifier {
public:
    typedef ClassABFenderTwinAmp CLASSNAME;
    
    // Fender Twin-style amplifier (push-pull Class AB)
    ClassABFenderTwinAmp();
    virtual ~ClassABFenderTwinAmp() {}
    
    virtual String GetClassName() const override { return "ClassABFenderTwinAmp"; }
    
protected:
    virtual void ProcessSignal() override;
};

#endif