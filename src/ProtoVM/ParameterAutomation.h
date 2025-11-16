#ifndef _ProtoVM_ParameterAutomation_h_
#define _ProtoVM_ParameterAutomation_h_

#include "AnalogCommon.h"
#include <vector>
#include <map>
#include <functional>
#include <memory>

// Enum for different parameter types
enum class ParameterType {
    GAIN,
    FREQUENCY,
    Q_FACTOR,
    RATIO,
    THRESHOLD,
    ATTACK,
    RELEASE,
    MIX,
    TIME,
    DISTORTION,
    OTHER
};

// Structure for automation points
struct AutomationPoint {
    double time;        // Time in seconds
    double value;       // Parameter value at that time
    bool active;        // Whether this point is active

    AutomationPoint(double t = 0.0, double v = 0.0) : time(t), value(v), active(true) {}
};

// Structure for parameter metadata
struct ParameterMetadata {
    std::string name;           // Parameter name
    std::string description;    // Parameter description
    ParameterType type;         // Parameter type
    double min_value;          // Minimum value
    double max_value;          // Maximum value
    double default_value;      // Default value
    std::string unit;          // Unit of measurement (e.g., "Hz", "dB", "%")
    
    ParameterMetadata(const std::string& n = "", const std::string& d = "", ParameterType t = ParameterType::OTHER,
                      double min = 0.0, double max = 1.0, double def = 0.5, const std::string& u = "")
        : name(n), description(d), type(t), min_value(min), max_value(max), default_value(def), unit(u) {}
};

// Class for managing automated parameters
class ParameterAutomator {
public:
    ParameterAutomator();
    virtual ~ParameterAutomator();

    // Add a parameter to be automated
    void AddParameter(int param_id, const ParameterMetadata& metadata);
    
    // Set/get parameter values
    void SetParameterValue(int param_id, double value);
    double GetParameterValue(int param_id) const;
    
    // Add an automation point
    void AddAutomationPoint(int param_id, const AutomationPoint& point);
    
    // Bulk add automation points
    void AddAutomationPoints(int param_id, const std::vector<AutomationPoint>& points);
    
    // Get parameter value at a specific time
    double GetParameterValueAtTime(int param_id, double simulation_time);
    
    // Set parameter value at current simulation time
    void SetParameterAtCurrentTime(int param_id, double value);
    
    // Smoothly transition parameter to new value over time
    void SmoothTransitionTo(int param_id, double target_value, double transition_time);

    // Set interpolation mode for parameter changes
    enum class InterpolationMode {
        LINEAR,
        SMOOTH,
        STEP,
        EXPONENTIAL,
        LOGARITHMIC
    };
    
    void SetInterpolationMode(int param_id, InterpolationMode mode);
    InterpolationMode GetInterpolationMode(int param_id) const;

    // Clear automation for a parameter
    void ClearAutomation(int param_id);

    // Get automation points for a parameter
    const std::vector<AutomationPoint>& GetAutomationPoints(int param_id) const;

    // Load automation from a file
    bool LoadAutomationFromFile(const std::string& filepath);
    
    // Save automation to a file
    bool SaveAutomationToFile(const std::string& filepath) const;

    // Process all parameters for the current simulation time
    void ProcessAutomation(double current_time);

    // Reset all parameters to default values
    void ResetToDefaults();

private:
    struct ParameterData {
        ParameterMetadata metadata;
        std::vector<AutomationPoint> points;
        InterpolationMode interp_mode;
        double current_value;
        double target_value;     // For smooth transitions
        double transition_start_time;
        double transition_duration;
        bool in_transition;

        ParameterData() : interp_mode(InterpolationMode::LINEAR), current_value(0.0), 
                         target_value(0.0), transition_start_time(0.0), 
                         transition_duration(0.0), in_transition(false) {}
    };

    std::map<int, ParameterData> param_map;
    
    // Helper for interpolation
    double InterpolateValue(double start_val, double end_val, double t, InterpolationMode mode) const;
};

// Base class for time-varying effects that use parameter automation
class TimeVaryingEffect : public AnalogNodeBase {
public:
    typedef TimeVaryingEffect CLASSNAME;

    TimeVaryingEffect(const std::string& name = "TimeVaryingEffect");
    virtual ~TimeVaryingEffect();

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "TimeVaryingEffect"; }

    // Get reference to the parameter automator
    ParameterAutomator& GetAutomator() { return automator; }
    const ParameterAutomator& GetAutomator() const { return automator; }

    // Set effect bypass state
    void SetBypass(bool bypass) { bypassed = bypass; }
    bool IsBypassed() const { return bypassed; }

    // Process the effect - to be implemented by derived classes
    virtual double ProcessSample(double input, double simulation_time) = 0;

protected:
    ParameterAutomator automator;
    bool bypassed;

private:
    std::string effect_name;
};

#endif