#include "TubeMultiplexers.h"
#include <algorithm>
#include <cmath>

// TubeMultiplexer implementation
TubeMultiplexer::TubeMultiplexer(int input_count) 
    : input_count(std::max(2, std::min(64, input_count)))
    , selector_count(static_cast<int>(ceil(log2(input_count))))
    , output_signal(0.0)
    , is_enabled(true)
    , selected_input(0)
    , propagation_delay(0.00005)  // 50 microseconds
    , switching_speed(1.0)        // 1x speed (nominal)
    , rise_time(0.00001)         // 10 microseconds
    , fall_time(0.00001)         // 10 microseconds)
{
    mux_type = (input_count == 2) ? MultiplexerType::TWO_TO_ONE :
               (input_count == 4) ? MultiplexerType::FOUR_TO_ONE :
               (input_count == 8) ? MultiplexerType::EIGHT_TO_ONE :
               (input_count == 16) ? MultiplexerType::SIXTEEN_TO_ONE :
               (input_count == 32) ? MultiplexerType::THIRTY_TWO_TO_ONE :
               MultiplexerType::SIXTY_FOUR_TO_ONE;  // Default to largest if not standard size
    
    InitializeMultiplexer();
}

TubeMultiplexer::~TubeMultiplexer() {
    // Cleanup handled by destructors
}

bool TubeMultiplexer::Tick() {
    if (!is_enabled) {
        output_signal = 0.0;
        return true;
    }
    
    // Process the multiplexer logic
    ProcessMultiplexer();
    
    // Apply switching characteristics to the output
    ApplySwitchingCharacteristics();
    
    // Tick all tubes in the multiplexer
    for (auto& tube : mux_tubes) {
        tube->Tick();
    }
    
    return true;
}

void TubeMultiplexer::InitializeMultiplexer() {
    // Resize input and selector signals based on count
    input_signals.resize(input_count, 0.0);
    selector_signals.resize(selector_count, 0.0);
    
    // Initialize tubes for implementing the multiplexer
    mux_tubes.clear();
    
    // For a simple implementation, use triodes for switching logic
    for (int i = 0; i < input_count; i++) {
        mux_tubes.push_back(std::make_unique<Triode>(100.0, 62000.0, 1.6e-3));  // Switching tube for each input
    }
    
    // Additional tubes for selector decoding logic
    for (int i = 0; i < selector_count; i++) {
        mux_tubes.push_back(std::make_unique<Triode>(100.0, 62000.0, 1.6e-3));  // Selector logic
    }
    
    Reset();
}

void TubeMultiplexer::Reset() {
    output_signal = 0.0;
    selected_input = 0;
    
    // Reset all input signals to 0
    for (int i = 0; i < input_count; i++) {
        input_signals[i] = 0.0;
    }
    
    // Reset all selector signals to 0
    for (int i = 0; i < selector_count; i++) {
        selector_signals[i] = 0.0;
    }
    
    // Reset all tubes
    for (auto& tube : mux_tubes) {
        tube->Reset();
    }
}

void TubeMultiplexer::SetInput(int input_id, double signal) {
    if (input_id >= 0 && input_id < input_count) {
        input_signals[input_id] = signal;
    }
}

double TubeMultiplexer::GetInput(int input_id) const {
    if (input_id >= 0 && input_id < input_count) {
        return input_signals[input_id];
    }
    return 0.0;
}

void TubeMultiplexer::SetSelector(int selector_id, double signal) {
    if (selector_id >= 0 && selector_id < selector_count) {
        selector_signals[selector_id] = signal;
    }
}

void TubeMultiplexer::SetSelectorValue(unsigned int value) {
    // Set all selectors from a single integer value
    for (int i = 0; i < selector_count; i++) {
        selector_signals[i] = (value & (1 << i)) ? TUBE_LOGIC_HIGH : TUBE_LOGIC_LOW;
    }
}

double TubeMultiplexer::GetSelector(int selector_id) const {
    if (selector_id >= 0 && selector_id < selector_count) {
        return selector_signals[selector_id];
    }
    return TUBE_LOGIC_LOW;
}

unsigned int TubeMultiplexer::GetSelectorValue() const {
    unsigned int value = 0;
    for (int i = 0; i < selector_count; i++) {
        if (selector_signals[i] >= TUBE_THRESHOLD) {
            value |= (1 << i);
        }
    }
    return value;
}

int TubeMultiplexer::SelectorValueToInputIndex() const {
    // Convert selector value to input index
    unsigned int selector_value = GetSelectorValue();
    return static_cast<int>(selector_value) % input_count;
}

void TubeMultiplexer::ApplySwitchingCharacteristics() {
    // Apply rise and fall time characteristics to the output
    static double prev_output = 0.0;
    static bool initialized = false;
    
    if (!initialized) {
        prev_output = output_signal;
        initialized = true;
    }
    
    // Determine if we're rising or falling
    double target = output_signal;
    if (target > prev_output) {
        // Rising - apply rise time
        double rate = (target - prev_output) / rise_time;
        double delta = rate / 44100.0;  // Assuming 44.1kHz sample rate
        prev_output = std::min(target, prev_output + delta);
    } else if (target < prev_output) {
        // Falling - apply fall time
        double rate = (prev_output - target) / fall_time;
        double delta = rate / 44100.0;  // Assuming 44.1kHz sample rate
        prev_output = std::max(target, prev_output - delta);
    } else {
        // Staying the same
        prev_output = target;
    }
    
    output_signal = prev_output;
}


// TubeDemultiplexer implementation
TubeDemultiplexer::TubeDemultiplexer(int output_count) 
    : output_count(std::max(2, std::min(64, output_count)))
    , selector_count(static_cast<int>(ceil(log2(output_count))))
    , input_signal(0.0)
    , is_enabled(true)
    , selected_output(0)
    , propagation_delay(0.00005)  // 50 microseconds
    , switching_speed(1.0)        // 1x speed (nominal)
{
    demux_type = (output_count == 2) ? DemultiplexerType::ONE_TO_TWO :
                 (output_count == 4) ? DemultiplexerType::ONE_TO_FOUR :
                 (output_count == 8) ? DemultiplexerType::ONE_TO_EIGHT :
                 (output_count == 16) ? DemultiplexerType::ONE_TO_SIXTEEN :
                 (output_count == 32) ? DemultiplexerType::ONE_TO_THIRTY_TWO :
                 DemultiplexerType::ONE_TO_SIXTY_FOUR;  // Default to largest if not standard size
    
    InitializeDemultiplexer();
}

TubeDemultiplexer::~TubeDemultiplexer() {
    // Cleanup handled by destructors
}

bool TubeDemultiplexer::Tick() {
    if (!is_enabled) {
        for (int i = 0; i < output_count; i++) {
            output_signals[i] = 0.0;
        }
        return true;
    }
    
    // Process the demultiplexer logic
    ProcessDemultiplexer();
    
    // Apply switching characteristics to the outputs
    ApplySwitchingCharacteristics();
    
    // Tick all tubes in the demultiplexer
    for (auto& tube : demux_tubes) {
        tube->Tick();
    }
    
    return true;
}

void TubeDemultiplexer::InitializeDemultiplexer() {
    // Resize output signals and selector signals based on count
    output_signals.resize(output_count, 0.0);
    selector_signals.resize(selector_count, 0.0);
    
    // Initialize tubes for implementing the demultiplexer
    demux_tubes.clear();
    
    // For a simple implementation, use triodes for switching logic
    for (int i = 0; i < output_count; i++) {
        demux_tubes.push_back(std::make_unique<Triode>(100.0, 62000.0, 1.6e-3));  // Switching tube for each output
    }
    
    // Additional tubes for selector decoding logic
    for (int i = 0; i < selector_count; i++) {
        demux_tubes.push_back(std::make_unique<Triode>(100.0, 62000.0, 1.6e-3));  // Selector logic
    }
    
    Reset();
}

void TubeDemultiplexer::Reset() {
    input_signal = 0.0;
    selected_output = 0;
    
    // Reset all output signals to 0
    for (int i = 0; i < output_count; i++) {
        output_signals[i] = 0.0;
    }
    
    // Reset all selector signals to 0
    for (int i = 0; i < selector_count; i++) {
        selector_signals[i] = 0.0;
    }
    
    // Reset all tubes
    for (auto& tube : demux_tubes) {
        tube->Reset();
    }
}

void TubeDemultiplexer::SetSelector(int selector_id, double signal) {
    if (selector_id >= 0 && selector_id < selector_count) {
        selector_signals[selector_id] = signal;
    }
}

void TubeDemultiplexer::SetSelectorValue(unsigned int value) {
    // Set all selectors from a single integer value
    for (int i = 0; i < selector_count; i++) {
        selector_signals[i] = (value & (1 << i)) ? TUBE_LOGIC_HIGH : TUBE_LOGIC_LOW;
    }
}

double TubeDemultiplexer::GetSelector(int selector_id) const {
    if (selector_id >= 0 && selector_id < selector_count) {
        return selector_signals[selector_id];
    }
    return TUBE_LOGIC_LOW;
}

unsigned int TubeDemultiplexer::GetSelectorValue() const {
    unsigned int value = 0;
    for (int i = 0; i < selector_count; i++) {
        if (selector_signals[i] >= TUBE_THRESHOLD) {
            value |= (1 << i);
        }
    }
    return value;
}

double TubeDemultiplexer::GetOutput(int output_id) const {
    if (output_id >= 0 && output_id < output_count) {
        return output_signals[output_id];
    }
    return 0.0;
}

int TubeDemultiplexer::SelectorValueToOutputIndex() const {
    // Convert selector value to output index
    unsigned int selector_value = GetSelectorValue();
    return static_cast<int>(selector_value) % output_count;
}

void TubeDemultiplexer::ApplySwitchingCharacteristics() {
    // Apply switching characteristics to the outputs
    // For simplicity, we'll apply the same characteristics to all outputs
    static bool initialized = false;
    static std::vector<double> prev_outputs(output_count, 0.0);
    
    if (!initialized) {
        prev_outputs.resize(output_count, 0.0);
        initialized = true;
    }
    
    // Apply rise and fall time characteristics to each output
    for (int i = 0; i < output_count; i++) {
        double rise_time = 0.00001;   // 10 microseconds
        double fall_time = 0.00001;   // 10 microseconds
        
        double target = output_signals[i];
        
        if (target > prev_outputs[i]) {
            // Rising
            double rate = (target - prev_outputs[i]) / rise_time;
            double delta = rate / 44100.0;  // Assuming 44.1kHz sample rate
            prev_outputs[i] = std::min(target, prev_outputs[i] + delta);
        } else if (target < prev_outputs[i]) {
            // Falling
            double rate = (prev_outputs[i] - target) / fall_time;
            double delta = rate / 44100.0;  // Assuming 44.1kHz sample rate
            prev_outputs[i] = std::max(target, prev_outputs[i] - delta);
        } else {
            // Staying the same
            prev_outputs[i] = target;
        }
        
        output_signals[i] = prev_outputs[i];
    }
}


// Tube2To1Multiplexer implementation
Tube2To1Multiplexer::Tube2To1Multiplexer() 
    : TubeMultiplexer(2) {
    InitializeMultiplexer();
}

void Tube2To1Multiplexer::InitializeMultiplexer() {
    input_count = 2;
    selector_count = 1;  // Only need 1 selector for 2 inputs
    
    input_signals.resize(2, 0.0);
    selector_signals.resize(1, 0.0);
    
    mux_tubes.clear();
    mux_tubes.push_back(std::make_unique<Triode>(100.0, 62000.0, 1.6e-3));  // Input 0 switch
    mux_tubes.push_back(std::make_unique<Triode>(100.0, 62000.0, 1.6e-3));  // Input 1 switch
    mux_tubes.push_back(std::make_unique<Triode>(100.0, 62000.0, 1.6e-3));  // Selector logic
    
    Reset();
}

void Tube2To1Multiplexer::ProcessMultiplexer() {
    SelectInputBasedOnSelectors();
}

void Tube2To1Multiplexer::SelectInputBasedOnSelectors() {
    // For 2:1 mux, if selector is HIGH, select input 1, otherwise select input 0
    bool selector_logic = (selector_signals[0] >= TUBE_THRESHOLD);
    selected_input = selector_logic ? 1 : 0;
    
    // Set output to the selected input
    output_signal = input_signals[selected_input];
    
    // Apply tube characteristics to the switching
    for (int i = 0; i < 2; i++) {
        if (auto tube = dynamic_cast<Triode*>(mux_tubes[i].get())) {
            // Use selector to control which tube is conducting
            bool current_input_selected = (i == selected_input);
            double grid_voltage = current_input_selected ? -1.0 : -4.0;  // Turn off unselected input
            tube->SetGridVoltage(grid_voltage);
            tube->SetPlateVoltage(250.0);
            tube->SetCathodeVoltage(0.0);
            tube->CalculateTubeBehavior();
            
            // Apply tube's response to the signal
            if (current_input_selected) {
                double tube_output = tube->GetPlateCurrent() * 1000.0;  // Convert current to voltage
                output_signal = input_signals[i] * 0.8 + tube_output * 0.2;  // Mix original with tube characteristics
            }
        }
    }
}


// Tube4To1Multiplexer implementation
Tube4To1Multiplexer::Tube4To1Multiplexer() 
    : TubeMultiplexer(4) {
    InitializeMultiplexer();
}

void Tube4To1Multiplexer::InitializeMultiplexer() {
    input_count = 4;
    selector_count = 2;  // Need 2 selectors for 4 inputs
    
    input_signals.resize(4, 0.0);
    selector_signals.resize(2, 0.0);
    
    mux_tubes.clear();
    for (int i = 0; i < 4; i++) {
        mux_tubes.push_back(std::make_unique<Triode>(100.0, 62000.0, 1.6e-3));  // Input switches
    }
    
    for (int i = 0; i < 4; i++) {
        mux_tubes.push_back(std::make_unique<Triode>(100.0, 62000.0, 1.6e-3));  // Decode logic tubes
    }
    
    Reset();
}

void Tube4To1Multiplexer::ProcessMultiplexer() {
    SelectInputBasedOnSelectors();
}

void Tube4To1Multiplexer::SelectInputBasedOnSelectors() {
    // Convert 2-bit selector to input index
    unsigned int selector_value = GetSelectorValue();
    selected_input = selector_value % 4;
    
    // Set output to the selected input
    output_signal = input_signals[selected_input];
    
    // Apply tube characteristics to the switching
    for (int i = 0; i < 4; i++) {
        if (auto tube = dynamic_cast<Triode*>(mux_tubes[i].get())) {
            // Use selector to control which tube is conducting
            bool current_input_selected = (i == selected_input);
            double grid_voltage = current_input_selected ? -1.0 : -4.0;  // Turn off unselected input
            tube->SetGridVoltage(grid_voltage);
            tube->SetPlateVoltage(250.0);
            tube->SetCathodeVoltage(0.0);
            tube->CalculateTubeBehavior();
            
            // Apply tube's response to the signal
            if (current_input_selected) {
                double tube_output = tube->GetPlateCurrent() * 1000.0;
                output_signal = input_signals[i] * 0.7 + tube_output * 0.3;
            }
        }
    }
}


// Tube8To1Multiplexer implementation
Tube8To1Multiplexer::Tube8To1Multiplexer() 
    : TubeMultiplexer(8) {
    InitializeMultiplexer();
}

void Tube8To1Multiplexer::InitializeMultiplexer() {
    input_count = 8;
    selector_count = 3;  // Need 3 selectors for 8 inputs
    
    input_signals.resize(8, 0.0);
    selector_signals.resize(3, 0.0);
    
    mux_tubes.clear();
    for (int i = 0; i < 8; i++) {
        mux_tubes.push_back(std::make_unique<Triode>(100.0, 62000.0, 1.6e-3));  // Input switches
    }
    
    for (int i = 0; i < 8; i++) {
        mux_tubes.push_back(std::make_unique<Triode>(100.0, 62000.0, 1.6e-3));  // Decode logic tubes
    }
    
    Reset();
}

void Tube8To1Multiplexer::ProcessMultiplexer() {
    SelectInputBasedOnSelectors();
}

void Tube8To1Multiplexer::SelectInputBasedOnSelectors() {
    // Convert 3-bit selector to input index
    unsigned int selector_value = GetSelectorValue();
    selected_input = selector_value % 8;
    
    // Set output to the selected input
    output_signal = input_signals[selected_input];
    
    // Apply tube characteristics to the switching
    for (int i = 0; i < 8; i++) {
        if (auto tube = dynamic_cast<Triode*>(mux_tubes[i].get())) {
            // Use selector to control which tube is conducting
            bool current_input_selected = (i == selected_input);
            double grid_voltage = current_input_selected ? -1.0 : -4.0;
            tube->SetGridVoltage(grid_voltage);
            tube->SetPlateVoltage(250.0);
            tube->SetCathodeVoltage(0.0);
            tube->CalculateTubeBehavior();
            
            // Apply tube's response to the signal
            if (current_input_selected) {
                double tube_output = tube->GetPlateCurrent() * 1000.0;
                output_signal = input_signals[i] * 0.7 + tube_output * 0.3;
            }
        }
    }
}


// TubeAnalogSwitchMultiplexer implementation
TubeAnalogSwitchMultiplexer::TubeAnalogSwitchMultiplexer(int input_count) 
    : TubeMultiplexer(input_count)
    , channel_isolation(60.0)  // 60dB isolation
    , on_resistance(10.0)      // 10 Ohms ON resistance
{
    mux_type = MultiplexerType::ANALOG_SWITCH;
    InitializeMultiplexer();
}

void TubeAnalogSwitchMultiplexer::InitializeMultiplexer() {
    // Initialize with the parent class
    TubeMultiplexer::InitializeMultiplexer();
    
    // Additional initialization for analog switching
    selector_count = static_cast<int>(ceil(log2(input_count)));
    input_signals.resize(input_count, 0.0);
    selector_signals.resize(selector_count, 0.0);
    
    // Initialize tubes for analog switching
    mux_tubes.clear();
    for (int i = 0; i < input_count; i++) {
        // Use triodes to simulate analog switching behavior
        mux_tubes.push_back(std::make_unique<Triode>(50.0, 4700.0, 6.0e-3));  // Lower mu tubes for better analog switching
    }
    
    // Additional tubes for decode logic
    for (int i = 0; i < selector_count; i++) {
        mux_tubes.push_back(std::make_unique<Triode>(100.0, 62000.0, 1.6e-3));
    }
    
    Reset();
}

void TubeAnalogSwitchMultiplexer::ProcessMultiplexer() {
    ApplyAnalogSwitching();
}

void TubeAnalogSwitchMultiplexer::ApplyAnalogSwitching() {
    // Convert selector to input index
    unsigned int selector_value = GetSelectorValue();
    selected_input = static_cast<int>(selector_value) % input_count;
    
    // Get the selected input signal
    double selected_signal = input_signals[selected_input];
    
    // Simulate analog switching behavior
    // Account for on-resistance and channel isolation
    double attenuation = 1.0;  // No attenuation for selected channel
    
    // For unselected channels, apply isolation
    double isolation_factor = pow(10.0, -channel_isolation / 20.0);
    
    // Combine all inputs with appropriate attenuation
    output_signal = 0.0;
    for (int i = 0; i < input_count; i++) {
        double input_attenuation = (i == selected_input) ? 1.0 : isolation_factor;
        output_signal += input_signals[i] * input_attenuation;
    }
    
    // Apply tube characteristics to the switching
    for (int i = 0; i < input_count; i++) {
        if (auto tube = dynamic_cast<Triode*>(mux_tubes[i].get())) {
            // Use selector to control which tube is conducting
            bool current_input_selected = (i == selected_input);
            double grid_voltage = current_input_selected ? -1.0 : -4.0;
            tube->SetGridVoltage(grid_voltage);
            tube->SetPlateVoltage(250.0);
            tube->SetCathodeVoltage(0.0);
            tube->CalculateTubeBehavior();
            
            // Apply tube's response to the signal
            if (current_input_selected) {
                double tube_output = tube->GetPlateCurrent() * 1000.0;
                // For analog switching, we want to preserve signal characteristics as much as possible
                output_signal = selected_signal * 0.9 + tube_output * 0.1;  // Minimal tube effect for analog
            }
        }
    }
}


// Tube1To4Demultiplexer implementation
Tube1To4Demultiplexer::Tube1To4Demultiplexer() 
    : TubeDemultiplexer(4) {
    InitializeDemultiplexer();
}

void Tube1To4Demultiplexer::InitializeDemultiplexer() {
    output_count = 4;
    selector_count = 2;  // Need 2 selectors for 4 outputs
    
    output_signals.resize(4, 0.0);
    selector_signals.resize(2, 0.0);
    
    demux_tubes.clear();
    for (int i = 0; i < 4; i++) {
        demux_tubes.push_back(std::make_unique<Triode>(100.0, 62000.0, 1.6e-3));  // Output switches
    }
    
    for (int i = 0; i < 4; i++) {
        demux_tubes.push_back(std::make_unique<Triode>(100.0, 62000.0, 1.6e-3));  // Decode logic tubes
    }
    
    Reset();
}

void Tube1To4Demultiplexer::ProcessDemultiplexer() {
    RouteInputToSelectedOutput();
}

void Tube1To4Demultiplexer::RouteInputToSelectedOutput() {
    // Convert 2-bit selector to output index
    unsigned int selector_value = GetSelectorValue();
    selected_output = selector_value % 4;
    
    // Clear all outputs except the selected one
    for (int i = 0; i < 4; i++) {
        output_signals[i] = (i == selected_output) ? input_signal : 0.0;
    }
    
    // Apply tube characteristics to the switching
    for (int i = 0; i < 4; i++) {
        if (auto tube = dynamic_cast<Triode*>(demux_tubes[i].get())) {
            // Use selector to control which tube is conducting
            bool current_output_selected = (i == selected_output);
            double grid_voltage = current_output_selected ? -1.0 : -4.0;
            tube->SetGridVoltage(grid_voltage);
            tube->SetPlateVoltage(250.0);
            tube->SetCathodeVoltage(0.0);
            tube->CalculateTubeBehavior();
            
            // Apply tube's response to the signal
            if (current_output_selected) {
                double tube_output = tube->GetPlateCurrent() * 1000.0;
                output_signals[i] = input_signal * 0.7 + tube_output * 0.3;
            } else {
                output_signals[i] = 0.0;  // Unselected outputs remain off
            }
        }
    }
}


// Tube1To8Demultiplexer implementation
Tube1To8Demultiplexer::Tube1To8Demultiplexer() 
    : TubeDemultiplexer(8) {
    InitializeDemultiplexer();
}

void Tube1To8Demultiplexer::InitializeDemultiplexer() {
    output_count = 8;
    selector_count = 3;  // Need 3 selectors for 8 outputs
    
    output_signals.resize(8, 0.0);
    selector_signals.resize(3, 0.0);
    
    demux_tubes.clear();
    for (int i = 0; i < 8; i++) {
        demux_tubes.push_back(std::make_unique<Triode>(100.0, 62000.0, 1.6e-3));  // Output switches
    }
    
    for (int i = 0; i < 8; i++) {
        demux_tubes.push_back(std::make_unique<Triode>(100.0, 62000.0, 1.6e-3));  // Decode logic tubes
    }
    
    Reset();
}

void Tube1To8Demultiplexer::ProcessDemultiplexer() {
    RouteInputToSelectedOutput();
}

void Tube1To8Demultiplexer::RouteInputToSelectedOutput() {
    // Convert 3-bit selector to output index
    unsigned int selector_value = GetSelectorValue();
    selected_output = selector_value % 8;
    
    // Clear all outputs except the selected one
    for (int i = 0; i < 8; i++) {
        output_signals[i] = (i == selected_output) ? input_signal : 0.0;
    }
    
    // Apply tube characteristics to the switching
    for (int i = 0; i < 8; i++) {
        if (auto tube = dynamic_cast<Triode*>(demux_tubes[i].get())) {
            // Use selector to control which tube is conducting
            bool current_output_selected = (i == selected_output);
            double grid_voltage = current_output_selected ? -1.0 : -4.0;
            tube->SetGridVoltage(grid_voltage);
            tube->SetPlateVoltage(250.0);
            tube->SetCathodeVoltage(0.0);
            tube->CalculateTubeBehavior();
            
            // Apply tube's response to the signal
            if (current_output_selected) {
                double tube_output = tube->GetPlateCurrent() * 1000.0;
                output_signals[i] = input_signal * 0.7 + tube_output * 0.3;
            } else {
                output_signals[i] = 0.0;  // Unselected outputs remain off
            }
        }
    }
}


// TubeMuxDemux implementation
TubeMuxDemux::TubeMuxDemux(int channel_count) 
    : channel_count(std::max(2, std::min(16, channel_count)))  // Limit to reasonable size
    , selector_count(static_cast<int>(ceil(log2(channel_count))))
    , is_enabled(true)
{
    InitializeMuxDemux();
}

TubeMuxDemux::~TubeMuxDemux() {
    // Cleanup handled by destructors
}

bool TubeMuxDemux::Tick() {
    if (!is_enabled) {
        return true;
    }
    
    // Process the combined mux/demux
    ProcessMuxDemux();
    
    // Tick internal components
    if (mux) {
        mux->Tick();
    }
    if (demux) {
        demux->Tick();
    }
    
    return true;
}

void TubeMuxDemux::InitializeMuxDemux() {
    // Create a multiplexer and demultiplexer
    mux = std::make_unique<Tube4To1Multiplexer>();  // Use 4:1 as a base
    demux = std::make_unique<Tube1To4Demultiplexer>();  // Use 1:4 as a base
    
    // Initialize vectors
    mux_inputs.resize(channel_count, 0.0);
    mux_selectors.resize(selector_count, 0.0);
    demux_outputs.resize(channel_count, 0.0);
    demux_selectors.resize(selector_count, 0.0);
    
    // Set initial values
    for (int i = 0; i < channel_count; i++) {
        mux_inputs[i] = 0.0;
        demux_outputs[i] = 0.0;
    }
    
    for (int i = 0; i < selector_count; i++) {
        mux_selectors[i] = 0.0;
        demux_selectors[i] = 0.0;
    }
}

void TubeMuxDemux::SetMuxInput(int input_id, double signal) {
    if (input_id >= 0 && input_id < channel_count) {
        mux_inputs[input_id] = signal;
        if (mux) {
            mux->SetInput(input_id, signal);
        }
    }
}

double TubeMuxDemux::GetMuxInput(int input_id) const {
    if (input_id >= 0 && input_id < channel_count) {
        return mux_inputs[input_id];
    }
    return 0.0;
}

void TubeMuxDemux::SetMuxSelector(int selector_id, double signal) {
    if (selector_id >= 0 && selector_id < selector_count) {
        mux_selectors[selector_id] = signal;
        if (mux) {
            mux->SetSelector(selector_id, signal);
        }
    }
}

void TubeMuxDemux::SetMuxSelectorValue(unsigned int value) {
    for (int i = 0; i < selector_count; i++) {
        bool selector_bit = (value & (1 << i)) != 0;
        mux_selectors[i] = selector_bit ? TUBE_LOGIC_HIGH : TUBE_LOGIC_LOW;
        if (mux) {
            mux->SetSelector(i, mux_selectors[i]);
        }
    }
}

void TubeMuxDemux::SetDemuxInput(double signal) {
    demux_input = signal;
    if (demux) {
        demux->SetInput(signal);
    }
}

double TubeMuxDemux::GetDemuxOutput(int output_id) const {
    if (output_id >= 0 && output_id < channel_count) {
        return demux_outputs[output_id];
    }
    return 0.0;
}

void TubeMuxDemux::SetDemuxSelector(int selector_id, double signal) {
    if (selector_id >= 0 && selector_id < selector_count) {
        demux_selectors[selector_id] = signal;
        if (demux) {
            demux->SetSelector(selector_id, signal);
        }
    }
}

void TubeMuxDemux::SetDemuxSelectorValue(unsigned int value) {
    for (int i = 0; i < selector_count; i++) {
        bool selector_bit = (value & (1 << i)) != 0;
        demux_selectors[i] = selector_bit ? TUBE_LOGIC_HIGH : TUBE_LOGIC_LOW;
        if (demux) {
            demux->SetSelector(i, demux_selectors[i]);
        }
    }
}

void TubeMuxDemux::SetCombinedSelector(int selector_id, double signal) {
    SetMuxSelector(selector_id, signal);
    SetDemuxSelector(selector_id, signal);
}

void TubeMuxDemux::SetCombinedSelectorValue(unsigned int value) {
    SetMuxSelectorValue(value);
    SetDemuxSelectorValue(value);
}

void TubeMuxDemux::ProcessMuxDemux() {
    // Connect the multiplexer output to the demultiplexer input
    if (mux && demux) {
        double mux_output_val = mux->GetOutput();
        demux->SetInput(mux_output_val);
        
        // Update our internal output vector
        for (int i = 0; i < channel_count; i++) {
            demux_outputs[i] = demux->GetOutput(i);
        }
    }
}