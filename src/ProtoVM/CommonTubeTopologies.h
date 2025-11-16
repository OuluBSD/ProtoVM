#ifndef _ProtoVM_CommonTubeTopologies_h_
#define _ProtoVM_CommonTubeTopologies_h_

#include "AnalogCommon.h"
#include "TubeModels.h"
#include <vector>
#include <memory>

// Enum for common tube topologies
enum class TubeTopology {
    CATHODE_FOLLOWER,     // Cathode follower (common grid, output from cathode)
    COMMON_CATHODE,       // Common cathode amplifier (grounded cathode)
    COMMON_GRID,          // Common grid amplifier (grounded grid)
    COMMON_PLATE,         // Common plate amplifier (grounded plate)
    DIFF_AMP,             // Differential amplifier
    LONG_TAIL_PAIR,       // Long-tailed pair phase inverter
    CURRENT_MIRROR,       // Current mirror circuit
    CASCODE,              // Cascode configuration
    WIDROW,               // Widrow (cathode-driven) configuration
    PENTODE_CASCODE       // Pentode cascode
};

// Base class for common tube topologies
class TubeTopologyBase : public AnalogNodeBase {
public:
    typedef TubeTopologyBase CLASSNAME;

    TubeTopologyBase(TubeTopology topology);
    virtual ~TubeTopologyBase();

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "TubeTopologyBase"; }

    // Set/get input signals (some topologies have multiple inputs)
    void SetInputSignal(double signal) { input_signal = signal; }
    virtual void SetInputSignal2(double signal) { input_signal2 = signal; }
    double GetInputSignal() const { return input_signal; }
    double GetInputSignal2() const { return input_signal2; }
    
    // Get output signal
    double GetOutputSignal() const { return output_signal; }
    
    // Get/set gain of the topology
    void SetGain(double gain) { topology_gain = std::max(0.1, std::min(100.0, gain)); }
    double GetGain() const { return topology_gain; }
    
    // Get/set output impedance
    void SetOutputImpedance(double impedance) { output_impedance = std::max(1.0, impedance); }
    double GetOutputImpedance() const { return output_impedance; }
    
    // Get/set input impedance
    void SetInputImpedance(double impedance) { input_impedance = std::max(100.0, impedance); }
    double GetInputImpedance() const { return input_impedance; }
    
    // Set topology type
    void SetTopologyType(TubeTopology type) { topology_type = type; }
    TubeTopology GetTopologyType() const { return topology_type; }
    
    // Enable/disable the topology
    void SetEnabled(bool enabled) { is_enabled = enabled; }
    bool IsEnabled() const { return is_enabled; }

protected:
    TubeTopology topology_type;
    double input_signal;       // Primary input signal
    double input_signal2;      // Secondary input signal (for differential circuits)
    double output_signal;      // Output signal
    double topology_gain;      // Inherent gain of the topology
    double output_impedance;   // Output impedance of the topology
    double input_impedance;    // Input impedance of the topology
    bool is_enabled;           // Whether the topology is enabled
    
    // Vector of tubes used in this topology
    std::vector<std::unique_ptr<Tube>> tubes;
    
    // Internal processing method
    virtual void ProcessSignal() = 0;
    
    // Helper method to configure tubes based on topology
    virtual void ConfigureTubes() = 0;
    
    static constexpr double MIN_GAIN = 0.1;
    static constexpr double MAX_GAIN = 100.0;
    static constexpr double MIN_OUTPUT_IMPEDANCE = 1.0;
    static constexpr double MIN_INPUT_IMPEDANCE = 100.0;
};

// Cathode follower topology
class CathodeFollower : public TubeTopologyBase {
public:
    typedef CathodeFollower CLASSNAME;

    CathodeFollower(double mu = 100.0, double rp = 62000.0, double gm = 1.6e-3);
    virtual ~CathodeFollower() {}

    virtual String GetClassName() const override { return "CathodeFollower"; }
    
    // Set cathode resistor value
    void SetCathodeResistor(double resistance) { cathode_resistor = std::max(100.0, resistance); }
    double GetCathodeResistor() const { return cathode_resistor; }
    
    // Set plate resistor value (for biasing)
    void SetPlateResistor(double resistance) { plate_resistor = std::max(1000.0, resistance); }
    double GetPlateResistor() const { return plate_resistor; }
    
    // Get output impedance (approximately 1/gm for cathode follower)
    double GetTheoreticalOutputImpedance() const;
    
    // Get input impedance (very high for cathode follower)
    double GetTheoreticalInputImpedance() const;

protected:
    double cathode_resistor;    // Cathode resistor value
    double plate_resistor;      // Plate resistor value (for biasing)
    
    virtual void ProcessSignal() override;
    virtual void ConfigureTubes() override;

    static constexpr double MIN_CATHODE_RESISTOR = 100.0;
    static constexpr double MAX_CATHODE_RESISTOR = 100000.0;
    static constexpr double MIN_PLATE_RESISTOR = 1000.0;
    static constexpr double MAX_PLATE_RESISTOR = 1000000.0;
};

// Common cathode amplifier topology
class CommonCathodeAmp : public TubeTopologyBase {
public:
    typedef CommonCathodeAmp CLASSNAME;

    CommonCathodeAmp(double mu = 100.0, double rp = 62000.0, double gm = 1.6e-3);
    virtual ~CommonCathodeAmp() {}

    virtual String GetClassName() const override { return "CommonCathodeAmp"; }
    
    // Set plate resistor value
    void SetPlateResistor(double resistance) { plate_resistor = std::max(1000.0, resistance); }
    double GetPlateResistor() const { return plate_resistor; }
    
    // Set cathode resistor value (for feedback)
    void SetCathodeResistor(double resistance) { cathode_resistor = std::max(0.0, resistance); }
    double GetCathodeResistor() const { return cathode_resistor; }
    
    // Get theoretical gain (approximately -gm * Rp for no cathode degeneration)
    double GetTheoreticalGain() const;

protected:
    double plate_resistor;      // Plate resistor value
    double cathode_resistor;    // Cathode resistor value (for feedback/cathode degeneration)
    
    virtual void ProcessSignal() override;
    virtual void ConfigureTubes() override;

    static constexpr double MIN_PLATE_RESISTOR = 1000.0;
    static constexpr double MAX_PLATE_RESISTOR = 1000000.0;
    static constexpr double MAX_CATHODE_RESISTOR = 100000.0;
};

// Differential amplifier (long-tailed pair)
class DifferentialAmp : public TubeTopologyBase {
public:
    typedef DifferentialAmp CLASSNAME;

    DifferentialAmp(double mu = 100.0, double rp = 62000.0, double gm = 1.6e-3);
    virtual ~DifferentialAmp() {}

    virtual String GetClassName() const override { return "DifferentialAmp"; }
    
    // Set resistor values
    void SetLoadResistor(double resistance) { load_resistor = std::max(1000.0, resistance); }
    double GetLoadResistor() const { return load_resistor; }
    
    void SetTailResistor(double resistance) { tail_resistor = std::max(1000.0, resistance); }
    double GetTailResistor() const { return tail_resistor; }
    
    // Get differential and common-mode gains
    double GetDifferentialGain() const { return differential_gain; }
    double GetCommonModeRejectionRatio() const { return cmrr; }

protected:
    double load_resistor;       // Load resistor value
    double tail_resistor;       // Tail resistor value (current source)
    double differential_gain;   // Differential gain
    double cmrr;                // Common mode rejection ratio
    
    virtual void ProcessSignal() override;
    virtual void ConfigureTubes() override;
    virtual double CalculateDifferentialOutput() const;

    static constexpr double MIN_LOAD_RESISTOR = 1000.0;
    static constexpr double MAX_LOAD_RESISTOR = 1000000.0;
    static constexpr double MIN_TAIL_RESISTOR = 1000.0;
    static constexpr double MAX_TAIL_RESISTOR = 1000000.0;
};

// Cascode configuration
class CascodeAmp : public TubeTopologyBase {
public:
    typedef CascodeAmp CLASSNAME;

    CascodeAmp(double mu = 100.0, double rp = 62000.0, double gm = 1.6e-3, 
               double mu2 = 100.0, double rp2 = 62000.0, double gm2 = 1.6e-3);
    virtual ~CascodeAmp() {}

    virtual String GetClassName() const override { return "CascodeAmp"; }
    
    // Set plate resistor value
    void SetPlateResistor(double resistance) { plate_resistor = std::max(1000.0, resistance); }
    double GetPlateResistor() const { return plate_resistor; }
    
    // Get benefits of cascode (higher gain, higher bandwidth, lower noise)
    double GetTheoreticalGain() const;
    double GetImprovedBandwidthFactor() const;
    double GetReducedNoiseFactor() const;

protected:
    double plate_resistor;      // Plate resistor value
    double mu1, rp1, gm1;       // Parameters for first tube
    double mu2, rp2, gm2;       // Parameters for second tube (shield/upper tube)
    
    virtual void ProcessSignal() override;
    virtual void ConfigureTubes() override;

    static constexpr double MIN_PLATE_RESISTOR = 1000.0;
    static constexpr double MAX_PLATE_RESISTOR = 1000000.0;
};

// Current mirror
class CurrentMirror : public TubeTopologyBase {
public:
    typedef CurrentMirror CLASSNAME;

    CurrentMirror(double mu = 100.0, double rp = 62000.0, double gm = 1.6e-3);
    virtual ~CurrentMirror() {}

    virtual String GetClassName() const override { return "CurrentMirror"; }
    
    // Set reference current
    void SetReferenceCurrent(double current) { reference_current = std::max(1e-6, std::min(1e-1, current)); }
    double GetReferenceCurrent() const { return reference_current; }
    
    // Get output current
    double GetOutputCurrent() const { return output_current; }

protected:
    double reference_current;   // Reference current
    double output_current;      // Output current (should match reference)
    
    virtual void ProcessSignal() override;
    virtual void ConfigureTubes() override;

    static constexpr double MIN_REFERENCE_CURRENT = 1e-6;  // 1μA
    static constexpr double MAX_REFERENCE_CURRENT = 1e-1;  // 100mA
};

#endif