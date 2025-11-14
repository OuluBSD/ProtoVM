#ifndef _ProtoVM_SparkGap_h_
#define _ProtoVM_SparkGap_h_

#include "AnalogCommon.h"

// Spark Gap component with breakdown voltage characteristics
class SparkGap : public AnalogNodeBase {
public:
    typedef SparkGap CLASSNAME;

    // Constructor: breakdown_voltage is in volts, is_arced initially (0=off, 1=arced)
    SparkGap(double breakdown_voltage = 1000.0, bool is_arced = false);
    virtual ~SparkGap() {}

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "SparkGap"; }

    void SetBreakdownVoltage(double voltage);
    double GetBreakdownVoltage() const { return breakdown_voltage; }
    
    // Check if currently arced (conducting)
    bool IsArced() const { return arced; }
    
    // Manually trigger the spark gap to arc
    void TriggerArc();
    
    // Reset the spark gap to non-arced state
    void Reset();

private:
    double breakdown_voltage;     // Voltage at which the gap breaks down (Volts)
    bool arced;                  // Whether the gap is currently conducting
    int arc_duration_ticks;      // How long the arc should persist (in simulation ticks)
    int current_arc_ticks;       // Current counter for arc duration
    bool triggered;              // Whether the spark gap was manually triggered
    
    static constexpr double MIN_BREAKDOWN_VOLTAGE = 1.0;  // 1V minimum
    static constexpr int DEFAULT_ARC_DURATION = 5;        // Default duration of arc in ticks
    static constexpr double ARCED_RESISTANCE = 1.0;       // Resistance when arced (Ohms)
    static constexpr double NON_ARCED_RESISTANCE = 1e9;   // Resistance when not arced (Ohms)
};

#endif