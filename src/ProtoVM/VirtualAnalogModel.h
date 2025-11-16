#ifndef _ProtoVM_VirtualAnalogModel_h_
#define _ProtoVM_VirtualAnalogModel_h_

#include "AnalogCommon.h"
#include "AnalogDifferentialEquations.h"
#include <vector>

// Enum for types of virtual analog models
enum class VirtualAnalogType {
    MOOG_LADDER_FILTER,     // Classic Moog transistor ladder filter
    DIODE_LADDER_FILTER,    // Diode ladder filter (like in EMS VCS3)
    STATE_VARIABLE_FILTER,  // State variable filter
    TRANSISTOR_OSCILLATOR,  // Classic analog oscillator with non-linearities
    OPERATIONAL_AMPLIFIER,  // Op-amp based circuits
    VINTAGE_DELAY,          // Analog delay using BBDs (Bucket Brigade Devices)
    CUSTOM_ANALOG_MODEL     // Custom virtual analog model
};

// Structure to hold analog modeling parameters
struct AnalogModelParams {
    std::vector<double> circuit_params;  // Component values (R, C, L, etc.)
    std::vector<double> model_params;    // Modeling parameters (non-linear coefficients, etc.)
    double sample_rate;                  // Sample rate for the simulation
    double temperature;                  // Temperature in Celsius (affects transistor behavior)
    
    AnalogModelParams() : sample_rate(44100.0), temperature(25.0) {}  // Standard temperature
};

class VirtualAnalogModel : public AnalogNodeBase {
public:
    typedef VirtualAnalogModel CLASSNAME;

    VirtualAnalogModel(VirtualAnalogType type = VirtualAnalogType::MOOG_LADDER_FILTER,
                      const AnalogModelParams& params = AnalogModelParams());
    virtual ~VirtualAnalogModel() {}

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "VirtualAnalogModel"; }

    void SetType(VirtualAnalogType type);
    VirtualAnalogType GetType() const { return type; }

    void SetInput(double input);
    double GetInput() const { return input_signal; }
    
    void SetControlVoltage(double cv);
    double GetControlVoltage() const { return control_voltage; }

    double GetOutput() const { return output; }

    void SetParams(const AnalogModelParams& params);
    const AnalogModelParams& GetParams() const { return params; }

    // Specific parameter setters for each model type
    void SetCutoffFrequency(double freq);       // For filters
    double GetCutoffFrequency() const;          // For filters
    
    void SetResonance(double res);              // For filters
    double GetResonance() const;                // For filters
    
    void SetOscillatorFrequency(double freq);   // For oscillators
    double GetOscillatorFrequency() const;      // For oscillators

private:
    VirtualAnalogType type;
    AnalogModelParams params;
    double input_signal;
    double control_voltage;
    double output;
    
    // Internal state for the models
    AnalogDifferentialEquation diff_eq_solver;
    
    // State variables for different models
    std::vector<double> state_variables;
    
    // Process each specific type of model
    void ProcessMoogLadderFilter();
    void ProcessDiodeLadderFilter();
    void ProcessStateVariableFilter();
    void ProcessTransistorOscillator();
    void ProcessOperationalAmplifier();
    void ProcessVintageDelay();
    void ProcessCustomAnalogModel();
    
    // Helper functions for modeling
    double TransistorResponse(double base_voltage, double collector_voltage);
    double DiodeResponse(double voltage);
    double OpAmpResponse(double input, double feedback);
    
    // Non-linear functions that model analog behavior
    double TanhSaturation(double input, double saturation_level = 0.95);
    double CubicSaturation(double input);
    double ExponentialResponse(double input);
};

#endif