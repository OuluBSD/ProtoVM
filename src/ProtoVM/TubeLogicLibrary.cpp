#include "TubeLogicLibrary.h"
#include <algorithm>
#include <cmath>

// TubeStandardLogicComponent implementation
TubeStandardLogicComponent::TubeStandardLogicComponent(StandardLogicIC ic_type) 
    : ic_type(ic_type)
    , is_enabled(true)
{
    // Initialize with default parameters based on IC type
    params.name = "Unknown";
    params.description = "Unknown component";
    params.pin_count = 0;
    params.tech = TubeTechnology::DIRECTLY_HEATED_TRIODE;
    params.plate_voltage = 250.0;  // Typical plate voltage
    params.heater_voltage = 6.3;    // Typical heater voltage
    params.tube_count = 1;
    params.current_draw = 25.0;    // 25mA typical
    params.is_standard_logic = false;
    params.propagation_delay = 0.00005;  // 50µs typical
    params.power_consumption = 6.0;      // 6W typical
    params.supply_voltage = 250.0;       // 250V typical
    
    InitializeComponent();
}

TubeStandardLogicComponent::~TubeStandardLogicComponent() {
    // Cleanup handled by destructors
}

bool TubeStandardLogicComponent::Tick() {
    if (!is_enabled) {
        return true;
    }
    
    // Process the component logic
    ProcessComponent();
    
    // Tick all tubes in the component
    for (auto& tube : component_tubes) {
        tube->Tick();
    }
    
    return true;
}

void TubeStandardLogicComponent::SetPinValue(int pin_id, double value) {
    if (pin_id >= 0 && pin_id < params.pin_count) {
        pin_values[pin_id] = value;
    }
}

double TubeStandardLogicComponent::GetPinValue(int pin_id) const {
    if (pin_id >= 0 && pin_id < params.pin_count) {
        return pin_values[pin_id];
    }
    return 0.0;
}

void TubeStandardLogicComponent::SetPinValue(const std::string& pin_name, double value) {
    int pin_id = GetPinIndex(pin_name);
    if (pin_id >= 0) {
        SetPinValue(pin_id, value);
    }
}

double TubeStandardLogicComponent::GetPinValue(const std::string& pin_name) const {
    int pin_id = GetPinIndex(pin_name);
    if (pin_id >= 0) {
        return GetPinValue(pin_id);
    }
    return 0.0;
}

int TubeStandardLogicComponent::GetPinIndex(const std::string& pin_name) const {
    for (size_t i = 0; i < params.pin_names.size(); i++) {
        if (params.pin_names[i] == pin_name) {
            return static_cast<int>(i);
        }
    }
    return -1;  // Not found
}

void TubeStandardLogicComponent::Reset() {
    for (auto& value : pin_values) {
        value = 0.0;
    }
    
    // Reset all tubes
    for (auto& tube : component_tubes) {
        tube->Reset();
    }
}


// TubeRegisterBank implementation
TubeRegisterBank::TubeRegisterBank(int register_count, int register_width) 
    : TubeStandardLogicComponent(StandardLogicIC::TUBE_REGISTER_BANK)
    , register_count(register_count)
    , register_width(register_width)
    , clock_signal(0.0)
    , enable_signal(1.0)  // Enabled by default
{
    InitializeComponent();
}

void TubeRegisterBank::InitializeComponent() {
    params.name = "Tube Register Bank";
    params.description = "An array of tube-based registers with control signals";
    params.pin_count = register_count * register_width + 2;  // Data pins + clock + enable
    params.tech = TubeTechnology::INDIRECTLY_HEATED_TRIODE;
    params.tube_count = register_count;  // Each register needs a tube for each bit or set of tubes
    params.propagation_delay = 0.0001;  // 100µs
    params.power_consumption = register_count * 5.0;  // Approx 5W per register
    params.supply_voltage = 250.0;
    
    // Initialize pin names
    params.pin_names.clear();
    
    // Data pins for each register
    for (int reg = 0; reg < register_count; reg++) {
        for (int bit = 0; bit < register_width; bit++) {
            params.pin_names.push_back("R" + std::to_string(reg) + "_D" + std::to_string(bit));
        }
    }
    
    // Control pins
    params.pin_names.push_back("CLK");
    params.pin_names.push_back("EN");
    
    // Initialize registers
    register_values.resize(register_count, 0);
    write_enables.resize(register_count, true);
    
    for (int i = 0; i < register_count; i++) {
        registers.push_back(std::make_unique<TubeSimpleRegister>(register_width));
    }
    
    // Initialize pin values
    pin_values.resize(params.pin_count, 0.0);
    
    // Initialize component tubes (for control circuitry)
    component_tubes.clear();
    for (int i = 0; i < register_count; i++) {
        component_tubes.push_back(std::make_unique<Triode>(100.0, 62000.0, 1.6e-3));
    }
    
    Reset();
}

void TubeRegisterBank::ProcessComponent() {
    ProcessRegisterBank();
}

void TubeRegisterBank::ProcessRegisterBank() {
    // Process each register in the bank based on control signals
    for (int i = 0; i < register_count; i++) {
        // Get the register's data input from the appropriate pins
        unsigned int data_input = 0;
        for (int bit = 0; bit < register_width; bit++) {
            int pin_index = i * register_width + bit;
            if (pin_values[pin_index] >= TUBE_THRESHOLD) {
                data_input |= (1 << bit);
            }
        }
        
        // Check if this register is enabled
        bool clock_rising = (pin_values[params.pin_count - 2] >= TUBE_THRESHOLD) && clock_signal < TUBE_THRESHOLD;
        bool enabled = (pin_values[params.pin_count - 1] >= TUBE_THRESHOLD);  // Enable signal
        
        // Update the register if conditions are met
        if (enabled && clock_rising && write_enables[i]) {
            registers[i]->SetInputData(data_input);
            registers[i]->SetClockInput(pin_values[params.pin_count - 2]);  // Use the clock signal
        }
        
        // Process the register
        registers[i]->Tick();
        
        // Update our internal register value
        register_values[i] = registers[i]->GetValue();
    }
    
    // Update clock signal for next tick
    clock_signal = pin_values[params.pin_count - 2];
    enable_signal = pin_values[params.pin_count - 1];
}

void TubeRegisterBank::SetRegisterValue(int reg_id, unsigned int value) {
    if (reg_id >= 0 && reg_id < register_count) {
        register_values[reg_id] = value;
        
        // Also update the register component
        if (reg_id < registers.size()) {
            registers[reg_id]->SetValue(value);
        }
        
        // Update the corresponding pins
        for (int bit = 0; bit < register_width; bit++) {
            int pin_index = reg_id * register_width + bit;
            pin_values[pin_index] = (value >> bit) & 1 ? TUBE_LOGIC_HIGH : TUBE_LOGIC_LOW;
        }
    }
}

unsigned int TubeRegisterBank::GetRegisterValue(int reg_id) const {
    if (reg_id >= 0 && reg_id < register_count) {
        return register_values[reg_id];
    }
    return 0;
}

void TubeRegisterBank::SetClockSignal(double signal) {
    int clk_pin = params.pin_count - 2;
    pin_values[clk_pin] = signal;
}

void TubeRegisterBank::SetEnableSignal(double signal) {
    int en_pin = params.pin_count - 1;
    pin_values[en_pin] = signal;
}

void TubeRegisterBank::SetWriteEnable(int reg_id, double signal) {
    if (reg_id >= 0 && reg_id < register_count) {
        write_enables[reg_id] = (signal >= TUBE_THRESHOLD);
    }
}

unsigned int TubeRegisterBank::GetOutputRegisterValue(int reg_id) const {
    if (reg_id >= 0 && reg_id < register_count) {
        return registers[reg_id]->GetValue();
    }
    return 0;
}


// TubeALU implementation
TubeALU::TubeALU(int data_width) 
    : TubeStandardLogicComponent(StandardLogicIC::TUBE_ARITHMETIC_LOGIC_UNIT)
    , data_width(data_width)
    , operand_a(0)
    , operand_b(0)
    , function(0)  // Default to ADD
    , result(0)
    , zero_flag(false)
    , carry_flag(false)
    , overflow_flag(false)
    , sign_flag(false)
{
    InitializeComponent();
}

void TubeALU::InitializeComponent() {
    params.name = "Tube ALU";
    params.description = "Arithmetic Logic Unit built with tube-based components";
    params.pin_count = data_width * 2 + 4 + data_width;  // A + B + control + result
    params.tech = TubeTechnology::DIRECTLY_HEATED_TRIODE;
    params.tube_count = 10 + data_width * 2;  // Base tubes plus additional for wider operations
    params.propagation_delay = 0.0002;  // 200µs (ALU operations take more time)
    params.power_consumption = 15.0;   // Higher power for complex operations
    params.supply_voltage = 250.0;
    
    // Initialize pin names
    params.pin_names.clear();
    
    // A operand pins
    for (int i = 0; i < data_width; i++) {
        params.pin_names.push_back("A" + std::to_string(i));
    }
    
    // B operand pins
    for (int i = 0; i < data_width; i++) {
        params.pin_names.push_back("B" + std::to_string(i));
    }
    
    // Function select pins (log2(functions) pins)
    int func_pins = static_cast<int>(ceil(log2(8)));  // Support up to 8 functions
    for (int i = 0; i < func_pins; i++) {
        params.pin_names.push_back("FUNC" + std::to_string(i));
    }
    
    // Control pins
    params.pin_names.push_back("CLK");
    params.pin_names.push_back("ENABLE");
    
    // Result pins
    for (int i = 0; i < data_width; i++) {
        params.pin_names.push_back("RESULT" + std::to_string(i));
    }
    
    // Flag pins
    params.pin_names.push_back("ZERO_FLAG");
    params.pin_names.push_back("CARRY_FLAG");
    params.pin_names.push_back("OVERFLOW_FLAG");
    params.pin_names.push_back("SIGN_FLAG");
    
    // Initialize ALU components
    arithmetic_unit = std::make_unique<TubeArithmeticRegister>(data_width);
    
    // Initialize logic units for logical operations
    for (int i = 0; i < data_width; i++) {
        logic_units.push_back(std::make_unique<TubeLogicGate>(LogicGateType::XOR, 2));
    }
    
    // Initialize pin values
    pin_values.resize(params.pin_count, 0.0);
    
    // Initialize component tubes
    component_tubes.clear();
    for (int i = 0; i < 5; i++) {  // Additional tubes for control logic
        component_tubes.push_back(std::make_unique<Triode>(100.0, 62000.0, 1.6e-3));
    }
    
    Reset();
}

void TubeALU::ProcessComponent() {
    ProcessALU();
}

void TubeALU::ProcessALU() {
    // Read operands from pins
    operand_a = 0;
    operand_b = 0;
    
    for (int i = 0; i < data_width; i++) {
        if (pin_values[i] >= TUBE_THRESHOLD) {  // A operand pins start at index 0
            operand_a |= (1 << i);
        }
        if (pin_values[data_width + i] >= TUBE_THRESHOLD) {  // B operand pins start after A
            operand_b |= (1 << i);
        }
    }
    
    // Read function from function select pins
    int func_start_pin = data_width * 2;  // After A and B operands
    int func_pins = static_cast<int>(ceil(log2(8)));  // Based on how many functions we support
    function = 0;
    for (int i = 0; i < func_pins; i++) {
        if (pin_values[func_start_pin + i] >= TUBE_THRESHOLD) {
            function |= (1 << i);
        }
    }
    
    // Read control signals
    bool clk_signal = (pin_values[func_start_pin + func_pins] >= TUBE_THRESHOLD);
    bool enabled = (pin_values[func_start_pin + func_pins + 1] >= TUBE_THRESHOLD);
    
    // Process based on function
    if (enabled) {
        switch (function) {
            case 0:  // ADD
                arithmetic_unit->SetValue(operand_a);
                arithmetic_unit->SetOperand(operand_b);
                arithmetic_unit->SetOperationMode(3);  // Add mode
                arithmetic_unit->Tick();
                result = arithmetic_unit->GetValue();
                carry_flag = arithmetic_unit->GetCarryOut();
                break;
                
            case 1:  // SUB
                arithmetic_unit->SetValue(operand_a);
                arithmetic_unit->SetOperand(operand_b);
                arithmetic_unit->SetOperationMode(4);  // Subtract mode
                arithmetic_unit->Tick();
                result = arithmetic_unit->GetValue();
                carry_flag = arithmetic_unit->GetBorrowOut();
                break;
                
            case 2:  // AND
                result = operand_a & operand_b;
                break;
                
            case 3:  // OR
                result = operand_a | operand_b;
                break;
                
            case 4:  // XOR
                result = operand_a ^ operand_b;
                break;
                
            case 5:  // NOT A
                result = ~operand_a;
                result &= (1 << data_width) - 1;  // Mask to data width
                break;
                
            case 6:  // SHIFT LEFT
                result = operand_a << 1;
                break;
                
            case 7:  // SHIFT RIGHT
                result = operand_a >> 1;
                break;
                
            default:
                result = 0;
                break;
        }
        
        // Compute flags
        ComputeFlags();
    }
    
    // Update output pins
    int result_start_pin = func_start_pin + func_pins + 2;  // After func pins and control pins
    for (int i = 0; i < data_width; i++) {
        pin_values[result_start_pin + i] = (result >> i) & 1 ? TUBE_LOGIC_HIGH : TUBE_LOGIC_LOW;
    }
    
    // Update flag pins
    pin_values[result_start_pin + data_width] = zero_flag ? TUBE_LOGIC_HIGH : TUBE_LOGIC_LOW;     // ZERO_FLAG
    pin_values[result_start_pin + data_width + 1] = carry_flag ? TUBE_LOGIC_HIGH : TUBE_LOGIC_LOW; // CARRY_FLAG
    pin_values[result_start_pin + data_width + 2] = overflow_flag ? TUBE_LOGIC_HIGH : TUBE_LOGIC_LOW; // OVERFLOW_FLAG
    pin_values[result_start_pin + data_width + 3] = sign_flag ? TUBE_LOGIC_HIGH : TUBE_LOGIC_LOW;  // SIGN_FLAG
}

void TubeALU::ComputeFlags() {
    // Zero flag - true if result is 0
    zero_flag = (result == 0);
    
    // Sign flag - true if most significant bit is set
    sign_flag = (result >> (data_width - 1)) & 1;
    
    // Overflow flag - for now, simplify this
    // In a real implementation, this would be more complex
    overflow_flag = false;
    
    // Actual overflow computation would depend on operation type
    // For now, just set it if the result appears to have overflowed
    if (function == 0) {  // ADD operation
        // Overflow in addition happens when two positives make a negative or two negatives make a positive
        bool a_sign = (operand_a >> (data_width - 1)) & 1;
        bool b_sign = (operand_b >> (data_width - 1)) & 1;
        bool r_sign = (result >> (data_width - 1)) & 1;
        overflow_flag = (a_sign == b_sign) && (a_sign != r_sign);
    } else if (function == 1) {  // SUB operation
        // Overflow in subtraction happens when signs of operands are different 
        // and sign of result differs from sign of first operand
        bool a_sign = (operand_a >> (data_width - 1)) & 1;
        bool b_sign = (operand_b >> (data_width - 1)) & 1;
        bool r_sign = (result >> (data_width - 1)) & 1;
        overflow_flag = (a_sign != b_sign) && (a_sign != r_sign);
    }
}

unsigned int TubeALU::GetResult() const {
    return result;
}


// TubeCounterArray implementation
TubeCounterArray::TubeCounterArray(int counter_count, int counter_width) 
    : TubeStandardLogicComponent(StandardLogicIC::TUBE_COUNTER_ARRAY)
    , counter_count(counter_count)
    , counter_width(counter_width)
    , cascade_enabled(false)
{
    InitializeComponent();
}

void TubeCounterArray::InitializeComponent() {
    params.name = "Tube Counter Array";
    params.description = "An array of tube-based counters with control signals";
    params.pin_count = counter_count * 3 + 1;  // Clock, Enable, Reset for each counter + cascade
    params.tech = TubeTechnology::INDIRECTLY_HEATED_TRIODE;
    params.tube_count = counter_count * 3;  // Each counter needs several tubes
    params.propagation_delay = 0.0001;  // 100µs
    params.power_consumption = counter_count * 4.0;  // Approx 4W per counter
    params.supply_voltage = 250.0;
    
    // Initialize pin names
    params.pin_names.clear();
    
    // Control pins for each counter
    for (int i = 0; i < counter_count; i++) {
        params.pin_names.push_back("CNT" + std::to_string(i) + "_CLK");
        params.pin_names.push_back("CNT" + std::to_string(i) + "_EN");
        params.pin_names.push_back("CNT" + std::to_string(i) + "_RST");
    }
    
    // Cascade enable pin
    params.pin_names.push_back("CASCADE_EN");
    
    // Initialize counters
    counter_values.resize(counter_count, 0);
    clock_signals.resize(counter_count, 0.0);
    enable_signals.resize(counter_count, 1.0);  // Enabled by default
    reset_signals.resize(counter_count, 0.0);
    
    for (int i = 0; i < counter_count; i++) {
        counters.push_back(std::make_unique<TubeSynchronousBinaryCounter>(counter_width));
    }
    
    // Initialize pin values
    pin_values.resize(params.pin_count, 0.0);
    
    // Initialize component tubes
    component_tubes.clear();
    for (int i = 0; i < counter_count; i++) {
        component_tubes.push_back(std::make_unique<Triode>(100.0, 62000.0, 1.6e-3));
    }
    
    Reset();
}

void TubeCounterArray::ProcessComponent() {
    ProcessCounterArray();
}

void TubeCounterArray::ProcessCounterArray() {
    for (int i = 0; i < counter_count; i++) {
        // Get control signals for this counter from pins
        int base_pin = i * 3;
        double clk_signal = pin_values[base_pin];       // CLK pin
        double en_signal = pin_values[base_pin + 1];   // EN pin
        double rst_signal = pin_values[base_pin + 2];  // RST pin
        
        // Set counter state based on control signals
        if (rst_signal >= TUBE_THRESHOLD) {
            counters[i]->Reset();
            counter_values[i] = 0;
        } else if (en_signal >= TUBE_THRESHOLD) {
            // Only tick if enabled
            counters[i]->SetClockSignal(clk_signal);
            counters[i]->Tick();
            counter_values[i] = counters[i]->GetValue();
        }
        
        // Update internal signals
        clock_signals[i] = clk_signal;
        enable_signals[i] = en_signal;
        reset_signals[i] = rst_signal;
    }
    
    // Handle cascading if enabled
    if (cascade_enabled && pin_values[params.pin_count - 1] >= TUBE_THRESHOLD && counter_count > 1) {
        for (int i = 1; i < counter_count; i++) {
            // If previous counter carries out, increment this counter
            if (counters[i-1]->GetCarryOut()) {
                // For simplicity, we'll directly update the counter's clock input
                counters[i]->SetClockSignal(TUBE_LOGIC_HIGH);
                counters[i]->Tick();
                counter_values[i] = counters[i]->GetValue();
            }
        }
    }
}

void TubeCounterArray::SetCounterValue(int counter_id, unsigned int value) {
    if (counter_id >= 0 && counter_id < counter_count) {
        counter_values[counter_id] = value;
        
        // Also update the counter component
        if (counter_id < counters.size()) {
            counters[counter_id]->SetValue(value);
        }
    }
}

unsigned int TubeCounterArray::GetCounterValue(int counter_id) const {
    if (counter_id >= 0 && counter_id < counter_count) {
        return counter_values[counter_id];
    }
    return 0;
}

void TubeCounterArray::SetClockSignal(int counter_id, double signal) {
    if (counter_id >= 0 && counter_id < counter_count) {
        int pin_index = counter_id * 3;  // Clock pin is first of the three control pins
        pin_values[pin_index] = signal;
    }
}

void TubeCounterArray::SetEnableSignal(int counter_id, double signal) {
    if (counter_id >= 0 && counter_id < counter_count) {
        int pin_index = counter_id * 3 + 1;  // Enable pin is second of the three control pins
        pin_values[pin_index] = signal;
    }
}

void TubeCounterArray::SetResetSignal(int counter_id, double signal) {
    if (counter_id >= 0 && counter_id < counter_count) {
        int pin_index = counter_id * 3 + 2;  // Reset pin is third of the three control pins
        pin_values[pin_index] = signal;
    }
}

unsigned int TubeCounterArray::GetOutputCounterValue(int counter_id) const {
    if (counter_id >= 0 && counter_id < counter_count) {
        return counters[counter_id]->GetValue();
    }
    return 0;
}


// TubeCompositeLogic template implementations
template<int InputCount, int OutputCount>
TubeCompositeLogic<InputCount, OutputCount>::TubeCompositeLogic(StandardLogicIC ic_type) 
    : TubeStandardLogicComponent(ic_type) {
    InitializeComponent();
}

template<int InputCount, int OutputCount>
void TubeCompositeLogic<InputCount, OutputCount>::SetInput(int input_id, double value) {
    if (input_id >= 0 && input_id < InputCount) {
        input_pins[input_id] = value;
    }
}

template<int InputCount, int OutputCount>
double TubeCompositeLogic<InputCount, OutputCount>::GetInput(int input_id) const {
    if (input_id >= 0 && input_id < InputCount) {
        return input_pins[input_id];
    }
    return 0.0;
}

template<int InputCount, int OutputCount>
void TubeCompositeLogic<InputCount, OutputCount>::SetOutput(int output_id, double value) {
    if (output_id >= 0 && output_id < OutputCount) {
        output_pins[output_id] = value;
    }
}

template<int InputCount, int OutputCount>
double TubeCompositeLogic<InputCount, OutputCount>::GetOutput(int output_id) const {
    if (output_id >= 0 && output_id < OutputCount) {
        return output_pins[output_id];
    }
    return 0.0;
}

template<int InputCount, int OutputCount>
void TubeCompositeLogic<InputCount, OutputCount>::ProcessComponent() {
    auto digital_inputs = AnalogToDigital();
    auto digital_outputs = ComputeOutputs(digital_inputs);
    DigitalToAnalog(digital_outputs);
}

template<int InputCount, int OutputCount>
std::array<bool, InputCount> TubeCompositeLogic<InputCount, OutputCount>::AnalogToDigital() const {
    std::array<bool, InputCount> digital;
    for (int i = 0; i < InputCount; i++) {
        digital[i] = input_pins[i] >= TUBE_THRESHOLD;
    }
    return digital;
}

template<int InputCount, int OutputCount>
void TubeCompositeLogic<InputCount, OutputCount>::DigitalToAnalog(const std::array<bool, OutputCount>& outputs) {
    for (int i = 0; i < OutputCount; i++) {
        output_pins[i] = outputs[i] ? TUBE_LOGIC_HIGH : TUBE_LOGIC_LOW;
    }
}

template<int InputCount, int OutputCount>
void TubeCompositeLogic<InputCount, OutputCount>::InitializeComponent() {
    params.name = "Tube Composite Logic";
    params.description = std::to_string(InputCount) + "-input, " + std::to_string(OutputCount) + "-output composite logic";
    params.pin_count = InputCount + OutputCount;
    params.tech = TubeTechnology::DIRECTLY_HEATED_TRIODE;
    params.tube_count = InputCount * OutputCount;  // Estimate based on complexity
    params.propagation_delay = 0.00005;  // 50µs typical
    params.power_consumption = InputCount * 0.5 + OutputCount * 0.2;  // Estimate
    params.supply_voltage = 250.0;
    
    // Initialize pin names
    params.pin_names.clear();
    
    // Input pins
    for (int i = 0; i < InputCount; i++) {
        params.pin_names.push_back("IN" + std::to_string(i));
    }
    
    // Output pins
    for (int i = 0; i < OutputCount; i++) {
        params.pin_names.push_back("OUT" + std::to_string(i));
    }
    
    // Initialize pin values
    pin_values.resize(params.pin_count, 0.0);
    
    // Initialize component tubes
    component_tubes.clear();
    for (int i = 0; i < params.tube_count; i++) {
        component_tubes.push_back(std::make_unique<Triode>(100.0, 62000.0, 1.6e-3));
    }
    
    Reset();
}


// TubeLogicLibrary implementation
TubeLogicLibrary::TubeLogicLibrary() {
    InitializeLibrary();
}

TubeLogicLibrary::~TubeLogicLibrary() {
    // Cleanup handled by destructors
}

void TubeLogicLibrary::InitializeLibrary() {
    // Initialize the supported components map
    supported_components.clear();
    
    // Add basic gates
    AddStandardComponent(StandardLogicIC::IC7400, "7400 NAND Gate", "Quad 2-input NAND gate", 14,
                         {"1A", "1B", "1Y", "2A", "2B", "2Y", "GND", "3Y", "3A", "3B", "4Y", "4A", "4B", "VCC"},
                         TubeTechnology::DIRECTLY_HEATED_TRIODE, 4, 0.00005, 2.0);
    
    AddStandardComponent(StandardLogicIC::IC7404, "7404 NOT Gate", "Hex inverter", 14,
                         {"1A", "1Y", "2A", "2Y", "3A", "3Y", "GND", "4Y", "4A", "5Y", "5A", "6Y", "6A", "VCC"},
                         TubeTechnology::DIRECTLY_HEATED_TRIODE, 6, 0.00005, 3.0);
    
    AddStandardComponent(StandardLogicIC::IC7432, "7432 OR Gate", "Quad 2-input OR gate", 14,
                         {"1A", "1B", "1Y", "2A", "2B", "2Y", "GND", "3Y", "3A", "3B", "4Y", "4A", "4B", "VCC"},
                         TubeTechnology::DIRECTLY_HEATED_TRIODE, 4, 0.00005, 2.0);
    
    // Add flip-flop components
    AddStandardComponent(StandardLogicIC::IC7474, "7474 D Flip-Flop", "Dual D-type flip-flop", 14,
                         {"1CLK", "1CLR", "1D", "1Q", "1Q'", "1PRE", "GND", "2Q'", "2Q", "2PRE", "2D", "2CLR", "2CLK", "VCC"},
                         TubeTechnology::INDIRECTLY_HEATED_TRIODE, 2, 0.0001, 3.0);
    
    // Add counter components
    AddStandardComponent(StandardLogicIC::IC7493, "7493 Counter", "4-bit binary ripple counter", 14,
                         {"CP1", "Q0", "Q1", "CP0", "MR1", "MR2", "GND", "Q3", "Q2", "NC", "NC", "NC", "NC", "VCC"},
                         TubeTechnology::INDIRECTLY_HEATED_TRIODE, 4, 0.0002, 2.5);
    
    // Add multiplexer components
    AddStandardComponent(StandardLogicIC::IC74151, "74151 Multiplexer", "8-to-1 multiplexer", 16,
                         {"A", "B", "C", "I0", "I1", "I2", "I3", "GND", "I4", "I5", "I6", "I7", "Y", "Y'", "S", "VCC"},
                         TubeTechnology::INDIRECTLY_HEATED_TRIODE, 8, 0.0001, 3.5);
    
    // Add ALU component
    AddStandardComponent(StandardLogicIC::IC74181, "74181 ALU", "4-bit arithmetic/logic unit", 24,
                         {"A0", "A1", "A2", "A3", "B0", "B1", "B2", "B3", "S0", "S1", "S2", "S3",
                          "C", "M", "VCC", "F0", "F1", "F2", "F3", "CN", "G", "P", "Q", "GND"},
                         TubeTechnology::BEAM_POWER_TUBE, 16, 0.0003, 8.0);
    
    // Add custom components
    AddStandardComponent(StandardLogicIC::TUBE_LOGIC_GATE_COMPOSITE, "Tube Composite Gate", 
                         "Composite logic gate using multiple tubes", 8,
                         {"IN0", "IN1", "IN2", "IN3", "OUT0", "OUT1", "VCC", "GND"},
                         TubeTechnology::DIRECTLY_HEATED_TRIODE, 5, 0.0001, 3.0);
    
    AddStandardComponent(StandardLogicIC::TUBE_REGISTER_BANK, "Tube Register Bank", 
                         "Bank of tube-based registers", 20,
                         {"D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7", "CLK", "EN", 
                          "Q0", "Q1", "Q2", "Q3", "Q4", "Q5", "Q6", "Q7", "VCC", "GND"},
                         TubeTechnology::INDIRECTLY_HEATED_TRIODE, 8, 0.0002, 5.0);
    
    AddStandardComponent(StandardLogicIC::TUBE_ARITHMETIC_LOGIC_UNIT, "Tube ALU", 
                         "Arithmetic Logic Unit using tube circuits", 20,
                         {"A0", "A1", "A2", "A3", "B0", "B1", "B2", "B3", "FUNC0", "FUNC1",
                          "RESULT0", "RESULT1", "RESULT2", "RESULT3", "CLK", "EN", 
                          "ZERO", "CARRY", "OVERFLOW", "GND"},
                         TubeTechnology::BEAM_POWER_TUBE, 15, 0.0004, 10.0);
}

void TubeLogicLibrary::AddStandardComponent(StandardLogicIC ic_type, const std::string& name, 
                                           const std::string& description, int pin_count, 
                                           const std::vector<std::string>& pin_names, 
                                           TubeTechnology tech, int tube_count, 
                                           double prop_delay, double power_cons) {
    TubeComponentParams params;
    params.name = name;
    params.description = description;
    params.pin_count = pin_count;
    params.pin_names = pin_names;
    params.tech = tech;
    params.tube_count = tube_count;
    params.propagation_delay = prop_delay;
    params.power_consumption = power_cons;
    params.is_standard_logic = true;
    params.plate_voltage = 250.0;
    params.supply_voltage = 250.0;
    params.current_draw = 25.0 * tube_count;  // Approximate current draw based on tube count
    
    component_params[ic_type] = params;
    supported_components.push_back(ic_type);
}

std::unique_ptr<TubeStandardLogicComponent> TubeLogicLibrary::CreateComponent(StandardLogicIC ic_type) {
    auto it = component_params.find(ic_type);
    if (it != component_params.end()) {
        // For now, we'll return a basic component - in a real implementation,
        // we would have specific implementations for each IC type
        switch (ic_type) {
            case StandardLogicIC::IC7404:
                // Create a NOT gate implementation
                break;
            case StandardLogicIC::IC7474:
                // Create a D flip-flop implementation
                break;
            case StandardLogicIC::TUBE_REGISTER_BANK:
                return std::make_unique<TubeRegisterBank>();
            case StandardLogicIC::TUBE_ARITHMETIC_LOGIC_UNIT:
                return std::make_unique<TubeALU>();
            case StandardLogicIC::TUBE_COUNTER_ARRAY:
                return std::make_unique<TubeCounterArray>();
            default:
                break;
        }
        
        // Return a generic component if specific implementation is not available
        return std::make_unique<TubeStandardLogicComponent>(ic_type);
    }
    
    return nullptr;
}

const TubeComponentParams* TubeLogicLibrary::GetComponentParams(StandardLogicIC ic_type) const {
    auto it = component_params.find(ic_type);
    if (it != component_params.end()) {
        return &(it->second);
    }
    return nullptr;
}

std::vector<StandardLogicIC> TubeLogicLibrary::GetSupportedComponents() const {
    return supported_components;
}

void TubeLogicLibrary::RegisterCustomComponent(StandardLogicIC ic_type, const TubeComponentParams& params) {
    component_params[ic_type] = params;
    supported_components.push_back(ic_type);
}

int TubeLogicLibrary::GetTubeCountForIC(StandardLogicIC ic_type) const {
    auto it = component_params.find(ic_type);
    if (it != component_params.end()) {
        return it->second.tube_count;
    }
    return 0;
}