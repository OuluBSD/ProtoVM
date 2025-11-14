#ifndef _ProtoVM_VCF_h_
#define _ProtoVM_VCF_h_

#include "AnalogCommon.h"

enum class FilterType {
    LOWPASS,
    HIGHPASS,
    BANDPASS,
    NOTCH,
    ALLPASS
};

enum class FilterImplementation {
    ONE_POLE,      // Simple RC-style filter
    MOOG_LADDER,   // Classic Moog ladder filter
    SVF,           // State Variable Filter
    BUTTERWORTH    // Butterworth filter
};

class VCF : public AnalogNodeBase {
public:
    typedef VCF CLASSNAME;

    VCF(FilterType type = FilterType::LOWPASS, 
        FilterImplementation implementation = FilterImplementation::MOOG_LADDER,
        double cutoff_freq = 1000.0, 
        double resonance = 0.5);
    virtual ~VCF() {}

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "VCF"; }

    void SetType(FilterType type);
    FilterType GetType() const { return filter_type; }

    void SetImplementation(FilterImplementation impl);
    FilterImplementation GetImplementation() const { return implementation; }

    void SetCutoffFrequency(double freq);
    double GetCutoffFrequency() const { return cutoff_frequency; }

    void SetResonance(double resonance);
    double GetResonance() const { return resonance; }

    void SetControlVoltage(double cv);
    double GetControlVoltage() const { return control_voltage; }

    void SetInput(double input);
    double GetOutput() const { return output; }

    void SetEnvelopeAmount(double amount);
    double GetEnvelopeAmount() const { return envelope_amount; }

private:
    FilterType filter_type;
    FilterImplementation implementation;
    double cutoff_frequency;      // Base cutoff frequency
    double resonance;             // Filter resonance/Q
    double control_voltage;       // Control voltage input (0-5V typically)
    double input_signal;          // Input signal to be filtered
    double output;                // Filtered output signal
    double envelope_amount;       // How much envelope affects cutoff

    // Filter state variables (for different implementations)
    double state[4];              // State storage for filters (up to 4th order)
    
    // For Moog ladder filter specifically
    double delay[4];              // Delay elements for Moog filter
    double tanh_output;           // For non-linear saturation

    static constexpr double MIN_CUTOFF = 20.0;     // Minimum cutoff Hz
    static constexpr double MAX_CUTOFF = 20000.0;  // Maximum cutoff Hz
    static constexpr double CV_SENSITIVITY = 1.0;  // Hz per volt sensitivity
    static constexpr double MIN_RESONANCE = 0.1;   // Minimum resonance
    static constexpr double MAX_RESONANCE = 10.0;  // Maximum resonance (self-oscillation starts around 10)
};

#endif