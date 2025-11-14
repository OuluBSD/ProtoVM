#ifndef _ProtoVM_AnalogComponents_h_
#define _ProtoVM_AnalogComponents_h_

#include "AnalogCommon.h"
#include "TransmissionLine.h"
#include "SparkGap.h"
#include "Fuse.h"

// Analog Resistor component
class AnalogResistor : public AnalogNodeBase {
public:
    typedef AnalogResistor CLASSNAME;
    
    AnalogResistor(double resistance = 1000.0);  // Default 1kΩ
    virtual ~AnalogResistor() {}
    
    virtual bool Tick() override;
    virtual String GetClassName() const override { return "AnalogResistor"; }
    
    void SetResistance(double r);
    double GetResistance() const { return resistance; }
    
private:
    double resistance;
    static constexpr double MIN_RESISTANCE = 0.001;  // 1mΩ minimum to avoid division by zero
};

// Analog Capacitor component
class AnalogCapacitor : public AnalogNodeBase {
public:
    typedef AnalogCapacitor CLASSNAME;
    
    AnalogCapacitor(double capacitance = 1e-6);  // Default 1μF
    virtual ~AnalogCapacitor() {}
    
    virtual bool Tick() override;
    virtual String GetClassName() const override { return "AnalogCapacitor"; }
    
    void SetCapacitance(double c);
    double GetCapacitance() const { return capacitance; }
    
private:
    double capacitance;
    double voltage_across_capacitor;  // Voltage stored in capacitor
    double charge;  // Charge stored in capacitor
    
    static constexpr double MIN_CAPACITANCE = 1e-12;  // 1pF minimum
    static constexpr double IDEAL_VOLTAGE_TOLERANCE = 1e-9;  // For numerical stability
};

// Analog Inductor component
class AnalogInductor : public AnalogNodeBase {
public:
    typedef AnalogInductor CLASSNAME;
    
    AnalogInductor(double inductance = 1e-3);  // Default 1mH
    virtual ~AnalogInductor() {}
    
    virtual bool Tick() override;
    virtual String GetClassName() const override { return "AnalogInductor"; }
    
    void SetInductance(double l);
    double GetInductance() const { return inductance; }
    
private:
    double inductance;
    double current_through_inductor;  // Current flowing through inductor
    
    static constexpr double MIN_INDUCTANCE = 1e-12;  // 1pH minimum
};

#endif