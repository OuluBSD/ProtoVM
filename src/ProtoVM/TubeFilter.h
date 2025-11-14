#ifndef _ProtoVM_TubeFilter_h_
#define _ProtoVM_TubeFilter_h_

#include "AnalogCommon.h"
#include "TriodeTubeModel.h"

enum class TubeFilterType {
    LOWPASS,
    HIGHPASS,
    BANDPASS
};

class TubeFilter : public AnalogNodeBase {
public:
    typedef TubeFilter CLASSNAME;

    TubeFilter(TubeFilterType type = TubeFilterType::LOWPASS, double cutoff_freq = 1000.0);
    virtual ~TubeFilter() {}

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "TubeFilter"; }

    void SetType(TubeFilterType type);
    TubeFilterType GetType() const { return filter_type; }

    void SetCutoffFrequency(double freq);
    double GetCutoffFrequency() const { return cutoff_frequency; }

    void SetInput(double input);
    double GetOutput() const { return output; }

    void SetResonance(double resonance);
    double GetResonance() const { return resonance; }

private:
    TubeFilterType filter_type;
    double cutoff_frequency;
    double resonance;
    double input_signal;
    double output;
    
    // Use triode tube models for the filter implementation
    TriodeTube triode_model;
    
    // Filter state for RC-style filtering with tube characteristics
    double filter_state[4];  // Multiple stages for more complex filtering
    double anode_voltage;    // Anode voltage affects filter characteristics
    double grid_voltage;     // Grid voltage affects filter characteristics

    static constexpr double MIN_CUTOFF = 10.0;     // Minimum cutoff in Hz
    static constexpr double MAX_CUTOFF = 20000.0;  // Maximum cutoff in Hz
    static constexpr double MIN_RESONANCE = 0.1;   // Minimum resonance
    static constexpr double MAX_RESONANCE = 10.0;  // Maximum resonance
    static constexpr double GRID_VOLTAGE_OFFSET = -2.0; // Typical bias voltage
};

#endif