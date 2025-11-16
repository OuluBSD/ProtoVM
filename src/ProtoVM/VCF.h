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
    ONE_POLE,         // Simple RC-style filter
    MOOG_LADDER,      // Classic Moog ladder filter (transistor-based)
    DIODE_LADDER,     // Diode ladder filter (like in EMS VCS3)
    SVF,              // State Variable Filter
    BUTTERWORTH,      // Butterworth filter
    MODIFIED_MOOG,    // Enhanced Moog with better non-linear modeling
    KENDON_CUTOFF     // Ken Donnelly's cutoff frequency algorithm
};

class VCF : public AnalogNodeBase {
public:
    typedef VCF CLASSNAME;

    VCF(FilterType type = FilterType::LOWPASS,
        FilterImplementation implementation = FilterImplementation::MODIFIED_MOOG,
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
    
    // Setters for additional parameters
    void SetDrive(double drive);      // Drive/harmonic saturation
    double GetDrive() const { return drive; }
    
    void SetSaturation(double sat);   // Output saturation level
    double GetSaturation() const { return saturation; }

private:
    FilterType filter_type;
    FilterImplementation implementation;
    double cutoff_frequency;          // Base cutoff frequency
    double resonance;                 // Filter resonance/Q
    double control_voltage;           // Control voltage input (0-5V typically)
    double input_signal;              // Input signal to be filtered
    double output;                    // Filtered output signal
    double envelope_amount;           // How much envelope affects cutoff
    double drive;                     // Harmonic saturation/drive
    double saturation;                // Output saturation level

    // Filter state variables (for different implementations)
    double state[4];                  // State storage for filters (up to 4th order)

    // For Moog ladder filter specifically
    double delay[4];                  // Delay elements for Moog filter
    double tanh_output;               // For non-linear saturation
    double transistor_state[4];       // Transistor emulation state for Moog
    double diode_state[4];            // Diode emulation state for Diode ladder

    // For Ken Donnelly's cutoff algorithm
    double cutoff_scale_factor;       // Scale factor for cutoff frequencies
    
    // For numerical integration of differential equations
    double integration_state[4];      // State for differential equation solving

    static constexpr double MIN_CUTOFF = 20.0;     // Minimum cutoff Hz
    static constexpr double MAX_CUTOFF = 20000.0;  // Maximum cutoff Hz
    static constexpr double CV_SENSITIVITY = 1.0;  // Hz per volt sensitivity
    static constexpr double MIN_RESONANCE = 0.1;   // Minimum resonance
    static constexpr double MAX_RESONANCE = 10.0;  // Maximum resonance (self-oscillation starts around 10)
    static constexpr double MIN_DRIVE = 0.0;       // Minimum drive
    static constexpr double MAX_DRIVE = 2.0;       // Maximum drive
    static constexpr double MIN_SATURATION = 0.1;  // Minimum saturation
    static constexpr double MAX_SATURATION = 1.0;  // Maximum saturation

    // Processing methods for different implementations
    double ProcessMoogLadderFilter();
    double ProcessDiodeLadderFilter();
    double ProcessStateVariableFilter();
    double ProcessOnePoleFilter();
    double ProcessButterworthFilter();
    double ProcessModifiedMoogFilter();
    double ProcessKendonCutoffFilter();
    
    // Helper methods for non-linear processing
    double TanhSaturation(double input, double saturation_level = 0.95);
    double TransistorResponse(double input, double drive);
    double DiodeResponse(double input);
};

#endif