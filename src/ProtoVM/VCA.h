#ifndef _ProtoVM_VCA_h_
#define _ProtoVM_VCA_h_

#include "AnalogCommon.h"

enum class VCACharacteristic {
    LINEAR,      // Linear gain
    EXPONENTIAL, // Exponential gain (audio-taper)
    LOGARITHMIC  // Logarithmic gain
};

class VCA : public AnalogNodeBase {
public:
    typedef VCA CLASSNAME;

    VCA(VCACharacteristic characteristic = VCACharacteristic::EXPONENTIAL, double gain = 1.0);
    virtual ~VCA() {}

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "VCA"; }

    void SetCharacteristic(VCACharacteristic characteristic);
    VCACharacteristic GetCharacteristic() const { return characteristic; }

    void SetGain(double gain);
    double GetGain() const { return gain; }

    void SetControlVoltage(double cv);
    double GetControlVoltage() const { return control_voltage; }

    void SetInput(double input);
    double GetOutput() const { return output; }

    void SetCVSensitivity(double sensitivity);
    double GetCVSensitivity() const { return cv_sensitivity; }

private:
    VCACharacteristic characteristic;
    double gain;                  // Base gain
    double control_voltage;       // Control voltage input (0-5V typically)
    double input_signal;          // Input signal to be amplified
    double output;                // Amplified output signal
    double cv_sensitivity;        // How much CV affects gain

    static constexpr double MIN_GAIN = 0.0;     // Minimum gain (muted)
    static constexpr double MAX_GAIN = 100.0;   // Maximum gain (40dB)
    static constexpr double MIN_CV = 0.0;       // Minimum control voltage
    static constexpr double MAX_CV = 10.0;      // Maximum control voltage
    static constexpr double CV_SENSITIVITY_DEFAULT = 1.0; // Default sensitivity
};

#endif