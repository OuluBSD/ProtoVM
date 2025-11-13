#ifndef _ProtoVM_AnalogSemiconductors_h_
#define _ProtoVM_AnalogSemiconductors_h_

#include "AnalogCommon.h"

// Analog Diode component
class AnalogDiode : public AnalogNodeBase {
public:
    typedef AnalogDiode CLASSNAME;
    
    AnalogDiode(double saturation_current = 1e-12, double emission_coefficient = 1.0);
    virtual ~AnalogDiode() {}
    
    virtual bool Tick() override;
    virtual String GetClassName() const override { return "AnalogDiode"; }
    
    void SetSaturationCurrent(double is);
    void SetEmissionCoefficient(double n);
    double GetSaturationCurrent() const { return saturation_current; }
    double GetEmissionCoefficient() const { return emission_coefficient; }
    
private:
    double saturation_current;      // IS - Saturation current (typically very small)
    double emission_coefficient;    // n - Emission coefficient (typically 1-2)
    
    // Physical constants
    static constexpr double K = 1.380649e-23;   // Boltzmann constant (J/K)
    static constexpr double Q = 1.602176634e-19; // Elementary charge (C)
    static constexpr double T = 300.0;           // Temperature in Kelvin (room temp)
    static constexpr double VT = (K * T) / Q;    // Thermal voltage (~25.85mV at 300K)
    
    static constexpr double IDEAL_VOLTAGE_TOLERANCE = 1e-9;
};

// Analog NPN Transistor component
class AnalogNPNTransistor : public AnalogNodeBase {
public:
    typedef AnalogNPNTransistor CLASSNAME;
    
    AnalogNPNTransistor(double beta = 100.0);  // Default beta of 100
    virtual ~AnalogNPNTransistor() {}
    
    virtual bool Tick() override;
    virtual String GetClassName() const override { return "AnalogNPNTransistor"; }
    
    void SetBeta(double b);
    double GetBeta() const { return beta; }
    
private:
    double beta;  // Current gain (IC / IB)
    
    // Internal state tracking
    double collector_current;
    double base_current;
    double emitter_current;
    
    // Physical constants
    static constexpr double K = 1.380649e-23;   // Boltzmann constant (J/K)
    static constexpr double Q = 1.602176634e-19; // Elementary charge (C)
    static constexpr double T = 300.0;           // Temperature in Kelvin (room temp)
    static constexpr double VT = (K * T) / Q;    // Thermal voltage (~25.85mV at 300K)
    
    static constexpr double MIN_CURRENT = 1e-15; // Minimum detectable current (1fA)
};

#endif