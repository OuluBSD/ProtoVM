#ifndef _ProtoVM_TubeAmpSimulation1960s_h_
#define _ProtoVM_TubeAmpSimulation1960s_h_

#include "AnalogCommon.h"
#include "TubeModels.h"
#include "AnalogComponents.h"
#include <vector>
#include <memory>
#include <cmath>

// Class for tube-based amplifier simulation for 1960s era (British blues rock characteristics)
class TubeAmpSimulation1960s : public ElectricNodeBase {
public:
    typedef TubeAmpSimulation1960s CLASSNAME;

    TubeAmpSimulation1960s();
    virtual ~TubeAmpSimulation1960s();

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "TubeAmpSimulation1960s"; }

    // Configuration methods
    void SetGain(double gain);
    void SetToneControls(double bass, double mid, double treble);
    void SetPresence(double presence);
    void SetResonance(double resonance);
    void SetPowerLevel(double level);  // 0.0 to 1.0 scale, affects saturation
    void SetInputLevel(double level);  // Input signal level
    void SetOutputLevel(double level); // Output signal level
    void SetCabSimulation(bool enabled); // Enable/disable cabinet simulation
    void SetClassA(bool class_a);      // Whether to simulate Class A (true) or Class AB (false)
    
    // Getter methods
    double GetGain() const { return gain; }
    double GetBass() const { return bass; }
    double GetMid() const { return mid; }
    double GetTreble() const { return treble; }
    double GetPresence() const { return presence; }
    double GetResonance() const { return resonance; }
    double GetPowerLevel() const { return power_level; }
    double GetInputLevel() const { return input_level; }
    double GetOutputLevel() const { return output_level; }
    bool GetCabSimulation() const { return cab_simulation_enabled; }
    bool GetClassA() const { return class_a_simulation; }

private:
    // Amp parameters for 1960s British era (e.g., Marshall-style)
    double gain;           // Overall gain
    double bass;           // Bass control
    double mid;            // Mid control (1960s amps famous for mid hump)
    double treble;         // Treble control
    double presence;       // Presence control
    double resonance;      // Resonance control (output transformer simulation)
    double power_level;    // Power section saturation level
    double input_level;    // Input signal level
    double output_level;   // Output signal level
    bool cab_simulation_enabled; // Whether to simulate speaker cabinet
    bool class_a_simulation;     // Whether to simulate Class A operation
    
    // Circuit simulation parameters
    std::vector<std::unique_ptr<Tube>> tubes;  // Simulation of tube stages
    std::vector<double> preamp_stage_gains;    // Gain for each preamp stage
    double phase_inverter_gain;                // Gain for phase inverter
    double output_transformer_coupling;        // Output transformer simulation
    
    // Tone stack simulation (classic Marshall-style for 1960s)
    double tone_stack_state[3];  // State for RC tone circuit simulation
    
    // Power amp simulation parameters
    double power_amp_saturation;  // Saturation in power amp section
    double power_amp_compression; // Compression in power amp section
    
    // Cabinet simulation parameters
    std::vector<double> cabinet_response;  // Cabinet frequency response
    std::vector<double> cabinet_delay;     // Cabinet delay/phase characteristics
    
    // Processing state
    double input_signal;
    double output_signal;
    double power_amp_signal;
    
    // Time constants for filtering
    double sample_rate;
    double dt;  // 1/sample_rate
    
    // Constants
    static constexpr int MAX_TUBES = 10;
    static constexpr double MIN_GAIN = 0.1;
    static constexpr double MAX_GAIN = 200.0;  // Higher gain for 1960s rock
    static constexpr double MIN_BASS = 0.0;
    static constexpr double MAX_BASS = 2.0;
    static constexpr double MIN_MID = 0.0;
    static constexpr double MAX_MID = 3.0;  // Higher range for 1960s mid hump
    static constexpr double MIN_TREBLE = 0.0;
    static constexpr double MAX_TREBLE = 2.0;
    static constexpr double MIN_PRESENCE = 0.0;
    static constexpr double MAX_PRESENCE = 1.5;  // More presence for 1960s British sound
    static constexpr double MIN_RESONANCE = 0.0;
    static constexpr double MAX_RESONANCE = 1.5;  // More resonance for British sound
    static constexpr double MIN_POWER_LEVEL = 0.0;
    static constexpr double MAX_POWER_LEVEL = 1.0;
    static constexpr double MIN_LEVEL = 0.0;
    static constexpr double MAX_LEVEL = 2.0;
    
    void InitializeAmp();
    void ProcessSignal();
    void ProcessPreamp();
    void ProcessPhaseInverter();
    void ProcessPowerAmp();
    void ProcessToneStack();
    void ProcessCabinetSimulation();
    void ApplyTubeCharacteristics();
    double CalculateToneStackResponse(double input, double bass, double mid, double treble);
};

#endif