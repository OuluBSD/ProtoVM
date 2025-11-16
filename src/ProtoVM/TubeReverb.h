#ifndef _ProtoVM_TubeReverb_h_
#define _ProtoVM_TubeReverb_h_

#include "AnalogCommon.h"
#include "TubeModels.h"
#include "AnalogComponents.h"  // For basic analog components
#include <vector>
#include <memory>

// Enum for reverb types
enum class ReverbType {
    SPRING,      // Spring reverb
    PLATE,       // Plate reverb
    HALL,        // Hall reverb
    ROOM         // Room reverb
};

// Enum for spring reverb configurations
enum class SpringReverbConfig {
    FENDER_2_SPRING,    // Fender-style 2-spring
    EHX_BIG_MUFF,       // Electro-Harmonix-style
    AMERICAN_STAND,     // American Standard
    CHAMBER             // Echo-Nivel Chamber
};

// Enum for tube reverb driver types
enum class DriverType {
    SINGLE_ENDED,    // Single-ended driver
    PUSH_PULL,       // Push-pull driver
    DIFFERENTIAL     // Differential driver
};

// Tube-based reverb unit
class TubeReverb : public AnalogNodeBase {
public:
    typedef TubeReverb CLASSNAME;

    TubeReverb(ReverbType type = ReverbType::SPRING, 
               SpringReverbConfig config = SpringReverbConfig::FENDER_2_SPRING);
    virtual ~TubeReverb();

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "TubeReverb"; }

    // Set/get input signal
    void SetInputSignal(double signal) { input_signal = signal; }
    double GetInputSignal() const { return input_signal; }
    
    // Get output signal (wet signal)
    double GetOutputSignal() const { return output_signal; }
    
    // Get mixed output (dry + wet)
    double GetMixedOutput() const { return dry_signal * (1.0 - mix_level) + output_signal * mix_level; }
    
    // Get/set reverb parameters
    void SetDecayTime(double time) { decay_time = std::max(0.1, std::min(10.0, time)); }  // 0.1 to 10 seconds
    double GetDecayTime() const { return decay_time; }
    
    void SetPreDelay(double delay) { pre_delay = std::max(0.0, std::min(0.5, delay)); }   // 0 to 500ms
    double GetPreDelay() const { return pre_delay; }
    
    void SetMixLevel(double mix) { mix_level = std::max(0.0, std::min(1.0, mix)); }       // 0% to 100%
    double GetMixLevel() const { return mix_level; }
    
    void SetDamping(double damping) { reverb_damping = std::max(0.0, std::min(1.0, damping)); }
    double GetDamping() const { return reverb_damping; }
    
    void SetDiffusion(double diffusion) { reverb_diffusion = std::max(0.0, std::min(1.0, diffusion)); }
    double GetDiffusion() const { return reverb_diffusion; }
    
    // Get/set input/output gain
    void SetInputGain(double gain) { input_gain = std::max(0.1, std::min(2.0, gain)); }
    void SetOutputGain(double gain) { output_gain = std::max(0.1, std::min(2.0, gain)); }
    double GetInputGain() const { return input_gain; }
    double GetOutputGain() const { return output_gain; }
    
    // Set reverb type
    void SetReverbType(ReverbType type) { reverb_type = type; }
    ReverbType GetReverbType() const { return reverb_type; }
    
    // Set spring configuration (only applicable for spring reverb)
    void SetSpringConfig(SpringReverbConfig config) { spring_config = config; }
    SpringReverbConfig GetSpringConfig() const { return spring_config; }
    
    // Set driver type (for tube reverb driver)
    void SetDriverType(DriverType type) { driver_type = type; }
    DriverType GetDriverType() const { return driver_type; }
    
    // Enable/disable reverb
    void SetEnabled(bool enabled) { is_enabled = enabled; }
    bool IsEnabled() const { return is_enabled; }
    
    // Get reverb impulse response characteristics
    double GetEarlyReflectionsLevel() const { return early_reflections_level; }
    double GetLateReverbLevel() const { return late_reverb_level; }
    
    // Get/set tube driver parameters (for spring reverb)
    void SetTubeDriverGain(double gain) { tube_driver_gain = std::max(1.0, std::min(100.0, gain)); }
    double GetTubeDriverGain() const { return tube_driver_gain; }
    
    // Get/set spring parameters
    void SetSpringTension(double tension) { spring_tension = std::max(0.1, std::min(2.0, tension)); }
    double GetSpringTension() const { return spring_tension; }
    
    void SetSpringLength(double length) { spring_length = std::max(0.1, std::min(2.0, length)); }
    double GetSpringLength() const { return spring_length; }

protected:
    ReverbType reverb_type;
    SpringReverbConfig spring_config;
    DriverType driver_type;
    double input_signal;              // Input signal to the reverb
    double dry_signal;                // Original signal (for mix)
    double output_signal;             // Wet reverb signal
    double decay_time;                // Reverb decay time in seconds
    double pre_delay;                 // Pre-delay in seconds
    double mix_level;                 // Mix level (0.0 = dry, 1.0 = wet)
    double reverb_damping;            // High-frequency damping
    double reverb_diffusion;          // Reverb diffusion
    double input_gain;                // Input amplification
    double output_gain;               // Output amplification
    double early_reflections_level;   // Level of early reflections
    double late_reverb_level;         // Level of late reverb
    double tube_driver_gain;          // Gain of tube driver circuit
    double spring_tension;            // Spring tension (affects resonance)
    double spring_length;             // Spring length (affects delay)
    bool is_enabled;                  // Whether reverb is enabled
    
    // Internal delay lines for reverb simulation
    std::vector<double> delay_line;
    size_t write_index;
    size_t read_index;
    
    // Filter coefficients for damping simulation
    double damping_coefficient;
    
    // Vector of tubes used in the reverb driver
    std::vector<std::unique_ptr<Tube>> driver_tubes;
    
    // Internal processing methods
    virtual void ProcessReverbSignal();
    virtual void UpdateDelayLine(double input);
    virtual double GetFromDelayLine(size_t delay_samples);
    virtual void ApplyDamping(double& signal);
    
    // Spring reverb specific methods
    virtual void ProcessSpringReverb();
    virtual void ProcessPlateReverb();
    
    // Initialize the reverb based on type and configuration
    virtual void InitializeReverb();
    
    static constexpr size_t DELAY_LINE_SIZE = 44100 * 2;  // 2 seconds at 44.1kHz
    static constexpr double MIN_DECAY = 0.1;              // 0.1 seconds
    static constexpr double MAX_DECAY = 10.0;             // 10 seconds
    static constexpr double MIN_DAMPING = 0.0;            // No damping
    static constexpr double MAX_DAMPING = 1.0;            // Maximum damping
};

// Fender-style spring reverb with tube driver
class FenderStyleReverb : public TubeReverb {
public:
    typedef FenderStyleReverb CLASSNAME;

    FenderStyleReverb();
    virtual ~FenderStyleReverb() {}

    virtual String GetClassName() const override { return "FenderStyleReverb"; }

protected:
    virtual void ProcessReverbSignal() override;
    virtual void InitializeReverb() override;
};

// Plate reverb simulation using tube circuits
class TubePlateReverb : public TubeReverb {
public:
    typedef TubePlateReverb CLASSNAME;

    TubePlateReverb();
    virtual ~TubePlateReverb() {}

    virtual String GetClassName() const override { return "TubePlateReverb"; }

protected:
    virtual void ProcessReverbSignal() override;
    virtual void InitializeReverb() override;
    
    // Plate reverb specific parameters
    double plate_size;             // Virtual plate size
    double plate_material;         // Virtual plate material density
    std::vector<std::vector<double>> plate_grid;  // 2D grid simulation of plate
};

#endif