#ifndef _ProtoVM_LFO_h_
#define _ProtoVM_LFO_h_

#include "AnalogCommon.h"

enum class LFOType {
    SINE,
    SAWTOOTH,
    SQUARE,
    TRIANGLE,
    SAMPLE_HOLD
};

class LFO : public AnalogNodeBase {
public:
    typedef LFO CLASSNAME;

    LFO(LFOType type = LFOType::SINE, double frequency = 1.0);
    virtual ~LFO() {}

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "LFO"; }

    void SetType(LFOType type);
    LFOType GetType() const { return type; }

    void SetFrequency(double freq);
    double GetFrequency() const { return frequency; }

    void SetAmplitude(double amp);
    double GetAmplitude() const { return amplitude; }

    void SetPhase(double phase);
    double GetPhase() const { return phase; }

    double GetOutput() const { return output; }

private:
    LFOType type;
    double frequency;             // Frequency in Hz
    double amplitude;             // Output amplitude (0-1)
    double phase;                 // Current phase
    double output;                // Current output value

    static constexpr double TWO_PI = 2.0 * M_PI;
    static constexpr double MIN_FREQ = 0.01;  // Minimum frequency (0.01 Hz = once per 100 seconds)
    static constexpr double MAX_FREQ = 100.0; // Maximum frequency (100 Hz)
    static constexpr double MIN_AMP = 0.0;    // Minimum amplitude
    static constexpr double MAX_AMP = 10.0;   // Maximum amplitude
};

#endif