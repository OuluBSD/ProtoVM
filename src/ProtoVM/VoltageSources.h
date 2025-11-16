#ifndef _ProtoVM_VoltageSources_h_
#define _ProtoVM_VoltageSources_h_

#include "AnalogCommon.h"
#include "Common.h"
#include <cmath>

// DC Voltage Source (2-terminal)
class DcVoltageSource : public AnalogNodeBase {
public:
    typedef DcVoltageSource CLASSNAME;

    DcVoltageSource(double voltage = 5.0);
    virtual ~DcVoltageSource() {}

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "DcVoltageSource"; }

    void SetVoltage(double v);
    double GetVoltage() const { return voltage; }

private:
    double voltage;  // Output voltage in volts
};

// AC Voltage Source (2-terminal) 
class AcVoltageSource : public AnalogNodeBase {
public:
    typedef AcVoltageSource CLASSNAME;

    AcVoltageSource(double amplitude = 1.0, double frequency = 60.0, double offset = 0.0);
    virtual ~AcVoltageSource() {}

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "AcVoltageSource"; }

    void SetAmplitude(double amp);
    double GetAmplitude() const { return amplitude; }
    
    void SetFrequency(double freq);
    double GetFrequency() const { return frequency; }
    
    void SetOffset(double offset);
    double GetOffset() const { return offset; }

private:
    double amplitude;   // Peak amplitude in volts
    double frequency;   // Frequency in Hz
    double offset;      // DC offset in volts
    
    static constexpr double PI = 3.14159265358979323846;
};

// DC Voltage Source (1-terminal) - Single-ended voltage reference
class DcVoltageSource1T : public AnalogNodeBase {
public:
    typedef DcVoltageSource1T CLASSNAME;

    DcVoltageSource1T(double voltage = 5.0);
    virtual ~DcVoltageSource1T() {}

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "DcVoltageSource1T"; }

    void SetVoltage(double v);
    double GetVoltage() const { return voltage; }

private:
    double voltage;  // Output voltage in volts relative to system ground
};

// AC Voltage Source (1-terminal)
class AcVoltageSource1T : public AnalogNodeBase {
public:
    typedef AcVoltageSource1T CLASSNAME;

    AcVoltageSource1T(double amplitude = 1.0, double frequency = 60.0, double offset = 0.0);
    virtual ~AcVoltageSource1T() {}

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "AcVoltageSource1T"; }

    void SetAmplitude(double amp);
    double GetAmplitude() const { return amplitude; }
    
    void SetFrequency(double freq);
    double GetFrequency() const { return frequency; }
    
    void SetOffset(double offset);
    double GetOffset() const { return offset; }

private:
    double amplitude;   // Peak amplitude in volts
    double frequency;   // Frequency in Hz
    double offset;      // DC offset in volts
    
    static constexpr double PI = 3.14159265358979323846;
};

// Square Wave Source (1-terminal)
class SquareWaveSource : public AnalogNodeBase {
public:
    typedef SquareWaveSource CLASSNAME;

    SquareWaveSource(double amplitude = 1.0, double frequency = 1.0, double offset = 0.0);
    virtual ~SquareWaveSource() {}

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "SquareWaveSource"; }

    void SetAmplitude(double amp);
    double GetAmplitude() const { return amplitude; }
    
    void SetFrequency(double freq);
    double GetFrequency() const { return frequency; }
    
    void SetOffset(double offset);
    double GetOffset() const { return offset; }

private:
    double amplitude;   // Peak amplitude in volts
    double frequency;   // Frequency in Hz
    double offset;      // DC offset in volts
    
    static constexpr double PI = 3.14159265358979323846;
};

// Clock Source (1-terminal)
class ClockSource : public AnalogNodeBase {
public:
    typedef ClockSource CLASSNAME;

    ClockSource(double frequency = 1.0, double duty_cycle = 0.5);
    virtual ~ClockSource() {}

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "ClockSource"; }

    void SetFrequency(double freq);
    double GetFrequency() const { return frequency; }
    
    void SetDutyCycle(double duty);
    double GetDutyCycle() const { return duty_cycle; }

private:
    double frequency;      // Frequency in Hz
    double duty_cycle;     // Duty cycle (0.0 to 1.0)
    double phase;          // Current phase of the clock
    
    static constexpr double PI = 3.14159265358979323846;
};

// AC Sweep Source for frequency analysis
class AcSweepSource : public AnalogNodeBase {
public:
    typedef AcSweepSource CLASSNAME;

    AcSweepSource(double start_freq = 1.0, double stop_freq = 10000.0, double amplitude = 1.0, double duration = 1.0);
    virtual ~AcSweepSource() {}

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "AcSweepSource"; }

    void SetStartFrequency(double freq);
    double GetStartFrequency() const { return start_freq; }
    
    void SetStopFrequency(double freq);
    double GetStopFrequency() const { return stop_freq; }
    
    void SetAmplitude(double amp);
    double GetAmplitude() const { return amplitude; }
    
    void SetDuration(double dur);
    double GetDuration() const { return duration; }

private:
    double start_freq;     // Starting frequency in Hz
    double stop_freq;      // Stopping frequency in Hz
    double amplitude;      // Peak amplitude in volts
    double duration;       // Duration of sweep in seconds
    double current_time;   // Current simulation time for sweep
    
    static constexpr double PI = 3.14159265358979323846;
};

// Variable Voltage Source (1-terminal) - Voltage controlled by external parameter
class VariableVoltageSource : public AnalogNodeBase {
public:
    typedef VariableVoltageSource CLASSNAME;

    VariableVoltageSource(double min_voltage = 0.0, double max_voltage = 5.0, double initial_voltage = 2.5);
    virtual ~VariableVoltageSource() {}

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "VariableVoltageSource"; }

    void SetVoltage(double v);
    double GetVoltage() const { return voltage; }
    
    void SetMinVoltage(double v);
    double GetMinVoltage() const { return min_voltage; }
    
    void SetMaxVoltage(double v);
    double GetMaxVoltage() const { return max_voltage; }

private:
    double voltage;        // Current output voltage
    double min_voltage;    // Minimum allowed voltage
    double max_voltage;    // Maximum allowed voltage
};

#endif