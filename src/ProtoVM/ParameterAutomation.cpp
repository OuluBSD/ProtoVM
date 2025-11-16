#include "ParameterAutomation.h"
#include <fstream>
#include <sstream>
#include <algorithm>

// ParameterAutomator implementation
ParameterAutomator::ParameterAutomator() {
}

ParameterAutomator::~ParameterAutomator() {
}

void ParameterAutomator::AddParameter(int param_id, const ParameterMetadata& metadata) {
    ParameterData& data = param_map[param_id];
    data.metadata = metadata;
    data.current_value = metadata.default_value;
    data.target_value = metadata.default_value;
}

void ParameterAutomator::SetParameterValue(int param_id, double value) {
    // Clamp to valid range
    if (param_map.find(param_id) != param_map.end()) {
        auto& metadata = param_map[param_id].metadata;
        if (value < metadata.min_value) value = metadata.min_value;
        if (value > metadata.max_value) value = metadata.max_value;
        
        param_map[param_id].current_value = value;
        param_map[param_id].target_value = value; // In case we were in a transition
        param_map[param_id].in_transition = false;
    }
}

double ParameterAutomator::GetParameterValue(int param_id) const {
    if (param_map.find(param_id) != param_map.end()) {
        return param_map.at(param_id).current_value;
    }
    return 0.0;
}

void ParameterAutomator::AddAutomationPoint(int param_id, const AutomationPoint& point) {
    auto& param_data = param_map[param_id];
    
    // Insert the point in time-sorted order
    auto& points = param_data.points;
    auto pos = std::lower_bound(points.begin(), points.end(), point, 
                               [](const AutomationPoint& a, const AutomationPoint& b) {
                                   return a.time < b.time;
                               });
    points.insert(pos, point);
}

void ParameterAutomator::AddAutomationPoints(int param_id, const std::vector<AutomationPoint>& points) {
    for (const auto& point : points) {
        AddAutomationPoint(param_id, point);
    }
}

double ParameterAutomator::GetParameterValueAtTime(int param_id, double simulation_time) {
    if (param_map.find(param_id) == param_map.end()) {
        return 0.0;
    }
    
    const auto& param_data = param_map[param_id];
    
    // If we're in a smooth transition, calculate the value
    if (param_data.in_transition) {
        double elapsed = simulation_time - param_data.transition_start_time;
        if (elapsed >= param_data.transition_duration) {
            // Transition complete
            return param_data.target_value;
        } else {
            // In the middle of transition
            double t = elapsed / param_data.transition_duration;
            return InterpolateValue(param_data.current_value, param_data.target_value, t, param_data.interp_mode);
        }
    }
    
    const auto& points = param_data.points;
    if (points.empty()) {
        return param_data.current_value;
    }
    
    // Find the relevant automation points
    auto current_point = points.end();
    for (auto it = points.rbegin(); it != points.rend(); ++it) {
        if (it->time <= simulation_time && it->active) {
            current_point = it.base() - 1;
            break;
        }
    }
    
    if (current_point == points.end()) {
        // No active point before current time, return first point's value or current value
        for (const auto& pt : points) {
            if (pt.active) {
                return pt.value;
            }
        }
        return param_data.current_value;
    }
    
    // If we're exactly at a point, return that value
    if (current_point->time == simulation_time) {
        // Update current value for any further interpolation
        const_cast<ParameterData&>(param_data).current_value = current_point->value;
        return current_point->value;
    }
    
    // Find the next active point
    auto next_point = points.end();
    for (auto it = current_point + 1; it != points.end(); ++it) {
        if (it->active) {
            next_point = it;
            break;
        }
    }
    
    // If no next point, return current point's value
    if (next_point == points.end()) {
        return current_point->value;
    }
    
    // Interpolate between the two points based on interpolation mode
    double t = (simulation_time - current_point->time) / (next_point->time - current_point->time);
    t = std::max(0.0, std::min(1.0, t)); // Clamp t to [0, 1]
    
    double interpolated_value = InterpolateValue(current_point->value, next_point->value, t, param_data.interp_mode);
    
    // Update current value
    const_cast<ParameterData&>(param_data).current_value = interpolated_value;
    
    return interpolated_value;
}

void ParameterAutomator::SetParameterAtCurrentTime(int param_id, double value) {
    double current_sim_time = simulation_time;  // Assuming this is available globally
    AddAutomationPoint(param_id, AutomationPoint(current_sim_time, value));
    
    // Also update the current value
    SetParameterValue(param_id, value);
}

void ParameterAutomator::SmoothTransitionTo(int param_id, double target_value, double transition_time) {
    if (param_map.find(param_id) != param_map.end()) {
        auto& data = param_map[param_id];
        data.current_value = GetParameterValue(param_id);  // Get the current value before transition
        data.target_value = target_value;
        data.transition_start_time = simulation_time;  // Assuming global simulation_time
        data.transition_duration = transition_time;
        data.in_transition = true;
    }
}

void ParameterAutomator::SetInterpolationMode(int param_id, InterpolationMode mode) {
    param_map[param_id].interp_mode = mode;
}

ParameterAutomator::InterpolationMode ParameterAutomator::GetInterpolationMode(int param_id) const {
    if (param_map.find(param_id) != param_map.end()) {
        return param_map.at(param_id).interp_mode;
    }
    return InterpolationMode::LINEAR;  // Default
}

void ParameterAutomator::ClearAutomation(int param_id) {
    if (param_map.find(param_id) != param_map.end()) {
        param_map[param_id].points.clear();
        param_map[param_id].in_transition = false;
    }
}

const std::vector<AutomationPoint>& 
ParameterAutomator::GetAutomationPoints(int param_id) const {
    static std::vector<AutomationPoint> empty_points;
    
    if (param_map.find(param_id) != param_map.end()) {
        return param_map.at(param_id).points;
    }
    
    return empty_points;
}

bool ParameterAutomator::LoadAutomationFromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    int current_param_id = -1;
    
    while (std::getline(file, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        std::istringstream iss(line);
        std::string token;
        iss >> token;
        
        if (token == "PARAM") {
            // Format: PARAM <id> <name> <min> <max> <default> <type>
            int id;
            std::string name, type_str;
            double min_val, max_val, default_val;
            iss >> id >> name >> min_val >> max_val >> default_val >> type_str;
            
            ParameterType type = ParameterType::OTHER;
            if (type_str == "GAIN") type = ParameterType::GAIN;
            else if (type_str == "FREQUENCY") type = ParameterType::FREQUENCY;
            else if (type_str == "Q_FACTOR") type = ParameterType::Q_FACTOR;
            else if (type_str == "RATIO") type = ParameterType::RATIO;
            else if (type_str == "THRESHOLD") type = ParameterType::THRESHOLD;
            else if (type_str == "ATTACK") type = ParameterType::ATTACK;
            else if (type_str == "RELEASE") type = ParameterType::RELEASE;
            else if (type_str == "MIX") type = ParameterType::MIX;
            else if (type_str == "TIME") type = ParameterType::TIME;
            else if (type_str == "DISTORTION") type = ParameterType::DISTORTION;
            
            AddParameter(id, ParameterMetadata(name, "", type, min_val, max_val, default_val));
            current_param_id = id;
        } 
        else if (token == "POINT" && current_param_id != -1) {
            // Format: POINT <time> <value>
            double time, value;
            iss >> time >> value;
            AddAutomationPoint(current_param_id, AutomationPoint(time, value));
        }
        else if (token == "INTERP") {
            // Format: INTERP <param_id> <mode>
            int id;
            std::string mode_str;
            iss >> id >> mode_str;
            
            InterpolationMode mode = InterpolationMode::LINEAR;
            if (mode_str == "SMOOTH") mode = InterpolationMode::SMOOTH;
            else if (mode_str == "STEP") mode = InterpolationMode::STEP;
            else if (mode_str == "EXPONENTIAL") mode = InterpolationMode::EXPONENTIAL;
            else if (mode_str == "LOGARITHMIC") mode = InterpolationMode::LOGARITHMIC;
            
            SetInterpolationMode(id, mode);
        }
    }
    
    file.close();
    return true;
}

bool ParameterAutomator::SaveAutomationToFile(const std::string& filepath) const {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        return false;
    }
    
    // Write parameters
    for (const auto& pair : param_map) {
        const auto& metadata = pair.second.metadata;
        std::string type_str = "OTHER";
        switch (metadata.type) {
            case ParameterType::GAIN: type_str = "GAIN"; break;
            case ParameterType::FREQUENCY: type_str = "FREQUENCY"; break;
            case ParameterType::Q_FACTOR: type_str = "Q_FACTOR"; break;
            case ParameterType::RATIO: type_str = "RATIO"; break;
            case ParameterType::THRESHOLD: type_str = "THRESHOLD"; break;
            case ParameterType::ATTACK: type_str = "ATTACK"; break;
            case ParameterType::RELEASE: type_str = "RELEASE"; break;
            case ParameterType::MIX: type_str = "MIX"; break;
            case ParameterType::TIME: type_str = "TIME"; break;
            case ParameterType::DISTORTION: type_str = "DISTORTION"; break;
            default: type_str = "OTHER"; break;
        }
        
        file << "PARAM " << pair.first << " " << metadata.name << " " 
             << metadata.min_value << " " << metadata.max_value << " " 
             << metadata.default_value << " " << type_str << std::endl;
        
        // Write automation points
        for (const auto& point : pair.second.points) {
            file << "POINT " << point.time << " " << point.value << std::endl;
        }
        
        // Write interpolation mode
        std::string interp_str = "LINEAR";
        switch (pair.second.interp_mode) {
            case InterpolationMode::SMOOTH: interp_str = "SMOOTH"; break;
            case InterpolationMode::STEP: interp_str = "STEP"; break;
            case InterpolationMode::EXPONENTIAL: interp_str = "EXPONENTIAL"; break;
            case InterpolationMode::LOGARITHMIC: interp_str = "LOGARITHMIC"; break;
            default: interp_str = "LINEAR"; break;
        }
        
        file << "INTERP " << pair.first << " " << interp_str << std::endl;
    }
    
    file.close();
    return true;
}

void ParameterAutomator::ProcessAutomation(double current_time) {
    // Process all parameters to update their values based on automation
    for (auto& pair : param_map) {
        double value = GetParameterValueAtTime(pair.first, current_time);
        pair.second.current_value = value;
    }
}

void ParameterAutomator::ResetToDefaults() {
    for (auto& pair : param_map) {
        pair.second.current_value = pair.second.metadata.default_value;
        pair.second.target_value = pair.second.metadata.default_value;
        pair.second.in_transition = false;
        pair.second.points.clear();
    }
}

double ParameterAutomator::InterpolateValue(double start_val, double end_val, double t, InterpolationMode mode) const {
    switch (mode) {
        case InterpolationMode::LINEAR:
            return start_val + t * (end_val - start_val);
            
        case InterpolationMode::SMOOTH:
            // Smoothstep function: t * t * (3 - 2 * t)
            t = t * t * (3.0 - 2.0 * t);
            return start_val + t * (end_val - start_val);
            
        case InterpolationMode::STEP:
            return (t < 1.0) ? start_val : end_val;
            
        case InterpolationMode::EXPONENTIAL:
            // Exponential interpolation
            if (start_val == 0.0) return end_val * t; // Avoid log(0)
            return start_val * pow(end_val / start_val, t);
            
        case InterpolationMode::LOGARITHMIC:
            // Logarithmic interpolation (for parameters like frequency)
            if (start_val <= 0.0 || end_val <= 0.0) return start_val + t * (end_val - start_val); // Fallback
            double log_start = log(start_val);
            double log_end = log(end_val);
            return exp(log_start + t * (log_end - log_start));
            
        default:
            return start_val + t * (end_val - start_val);
    }
}

// TimeVaryingEffect implementation
TimeVaryingEffect::TimeVaryingEffect(const std::string& name) 
    : bypassed(false), effect_name(name) {
    analog_values.resize(2, 0.0);  // Input and output connectors
}

TimeVaryingEffect::~TimeVaryingEffect() {
}

bool TimeVaryingEffect::Tick() {
    // Get input from analog_values[0]
    double input = analog_values[0];
    
    // Process the effect if not bypassed
    double output;
    if (bypassed) {
        output = input;  // Pass through if bypassed
    } else {
        output = ProcessSample(input, simulation_time);  // Process with time variation
    }
    
    // Update output in analog_values[1]
    analog_values[1] = output;
    UpdateAnalogValue(0, input);   // Update input
    UpdateAnalogValue(1, output);  // Update output
    
    // Process automation for the current time
    automator.ProcessAutomation(simulation_time);
    
    return true;
}
