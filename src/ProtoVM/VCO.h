#ifndef _ProtoVM_VCO_h_
#define _ProtoVM_VCO_h_

#include "AnalogCommon.h"

enum class VCOType {
    SINE,
    SAWTOOTH,
    SQUARE,
    TRIANGLE,
    NOISE,
    PULSE,
    S_H,
    MORSE_CODE,
    CUSTOM
};

class VCO : public AnalogNodeBase {
public:
    typedef VCO CLASSNAME;

    VCO(VCOType type = VCOType::SAWTOOTH, double base_frequency = 440.0);
    virtual ~VCO() {}

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "VCO"; }

    void SetType(VCOType type);
    VCOType GetType() const { return type; }

    void SetBaseFrequency(double freq);
    double GetBaseFrequency() const { return base_frequency; }

    void SetControlVoltage(double cv);
    double GetControlVoltage() const { return control_voltage; }

    void SetAmplitude(double amp);
    double GetAmplitude() const { return amplitude; }

    void SetFMModulation(double fm);
    double GetFMModulation() const { return fm_modulation; }

    void SetPWM(double duty_cycle);
    double GetPWM() const { return pwm_duty_cycle; }

    void SetSampleRate(double rate);
    double GetSampleRate() const { return sample_rate; }

    void EnableAntiAliasing(bool enable);
    bool IsAntiAliasingEnabled() const { return anti_aliasing_enabled; }

    double GetOutput() const { return output; }

private:
    VCOType type;
    double base_frequency;        // Base frequency when no control voltage
    double control_voltage;       // Control voltage input (0-5V typically)
    double amplitude;             // Output amplitude
    double fm_modulation;         // Frequency modulation amount
    double pwm_duty_cycle;        // PWM duty cycle (for square wave)
    double phase;                 // Current phase of the oscillator
    double output;                // Current output value
    double last_input_sample;     // For interpolation/filtering
    double sample_rate;           // Sample rate for calculations
    bool anti_aliasing_enabled;   // Whether anti-aliasing is enabled

    std::mt19937 random_gen;      // Random number generator for noise

    // Waveform generation functions
    double GenerateSineWave();
    double GenerateSawtoothWave();
    double GenerateTriangleWave();
    double GenerateSquareWave();
    double GeneratePulseWave();
    double GenerateNoise();
    double GenerateSampleAndHold();
    double GenerateMorseCode();
    double GenerateCustomWave();
    
    // Band-limited waveform generation functions
    double GenerateBandLimitedSawtooth();
    double GenerateBandLimitedTriangle();
    double GenerateBandLimitedSquare();
    double GenerateBandLimitedPulse();

    // Utility functions
    void InitializeWaveformTables();

    static constexpr double TWO_PI = 2.0 * M_PI;
    static constexpr double MIN_FREQ = 0.01;  // Minimum frequency in Hz
    static constexpr double MAX_FREQ = 20000; // Maximum frequency in Hz
    static constexpr double CV_SENSITIVITY = 1.0; // Hz per volt sensitivity
};

#endif