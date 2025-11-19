#ifndef _ProtoVM_TubeLogicLibrary_h_
#define _ProtoVM_TubeLogicLibrary_h_

#include "AnalogCommon.h"
#include "TubeLogicGates.h"
#include "TubeFlipFlops.h"
#include "TubeCountersRegisters.h"
#include "TubeMultiplexers.h"
#include "TubeArithmeticUnits.h"
#include "TubeClockOscillators.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include <string>

// Forward declarations for standard tube components
class TubeCounterRegister;

// Structure for tube information
struct TubeInfo {
    std::string type;                    // Type of tube (e.g., "12AX7", "6SN7", etc.)
    std::string description;             // Description of the tube
    int pin_count;                       // Number of pins
    std::vector<std::string> pin_names;  // Names of pins
    double mu;                           // Amplification factor
    double gm;                           // Transconductance
    double rp;                           // Plate resistance
    double c_out;                        // Output capacitance
    double c_in;                         // Input capacitance
    double c_grid;                       // Grid-to-plate capacitance
    double max_plate_voltage;            // Maximum plate voltage
    double max_plate_dissipation;        // Maximum plate dissipation
    double heater_voltage;               // Heater voltage
    bool heater_ac;                      // Whether heater is AC or DC
    double heater_current;               // Heater current
    double base_current;                 // Base current for calculating gain
    double min_frequency;                // Minimum operating frequency
    double max_frequency;                // Maximum operating frequency
};

// Enum for standard logic ICs
enum class StandardLogicIC {
    // Basic gates (equivalent to 74xx series)
    IC7400,           // Quad 2-input NAND gate
    IC7402,           // Quad 2-input NOR gate
    IC7404,           // Hex inverter (NOT gate)
    IC7408,           // Quad 2-input AND gate
    IC7432,           // Quad 2-input OR gate
    IC7486,           // Quad 2-input XOR gate
    IC7410,           // Triple 3-input NAND gate
    IC7420,           // Dual 4-input NAND gate
    IC7427,           // Triple 3-input NOR gate
    
    // Flip-flops
    IC7474,           // Dual D-type flip-flop with preset and clear
    IC7473,           // Dual J-K flip-flop with clear
    IC7476,           // Dual J-K master-slave flip-flop
    
    // Registers
    IC7495,           // 4-bit universal shift register
    IC74173,          // Quadruple D-type flip-flop (register)
    
    // Counters
    IC7490,           // Divide-by-2 and divide-by-5 counter
    IC7493,           // 4-bit binary ripple counter
    IC74161,          // Synchronous 4-bit binary counter
    IC74163,          // Synchronous 4-bit binary counter with sync reset
    
    // Multiplexers/Demultiplexers
    IC74157,          // Quad 2-to-1 multiplexer
    IC74151,          // 8-to-1 multiplexer
    IC74138,          // 3-to-8 decoder/demultiplexer
    
    // Arithmetic units
    IC74181,          // 4-bit arithmetic/logic unit (ALU)
    IC74283,          // 4-bit binary full adder
    
    // Oscillators and timers
    IC555,            // Timer IC (could be implemented with tubes in astable mode)
    
    // Custom combinations
    TUBE_LOGIC_GATE_COMPOSITE,  // Composite logic gate
    TUBE_FLIP_FLOP_ARRAY,       // Array of flip-flops
    TUBE_REGISTER_BANK,         // Bank of registers
    TUBE_COUNTER_ARRAY,         // Array of counters
    TUBE_ARITHMETIC_LOGIC_UNIT  // Arithmetic Logic Unit (ALU)
};

// Enum for tube technology
enum class TubeTechnology {
    DIRECTLY_HEATED_TRIODE,
    INDIRECTLY_HEATED_TRIODE,
    PENTODE,
    TETRODE,
    BEAM_POWER_TUBE,
    SPECIALIZED_SWITCHING_TUBE
};

// Structure for tube component parameters
struct TubeComponentParams {
    std::string name;                    // Name of the component
    std::string description;             // Description of the component
    int pin_count;                       // Number of pins
    std::vector<std::string> pin_names;  // Names of pins
    TubeTechnology tech;                 // Tube technology used
    double plate_voltage;                // Operating plate voltage
    double heater_voltage;               // Heater voltage
    int tube_count;                      // Number of tubes used
    std::vector<TubeInfo> tube_specs;    // Specifications for each tube
    double current_draw;                 // Current draw in mA
    bool is_standard_logic;              // Whether this is a standard logic component
    double propagation_delay;            // Propagation delay in seconds
    double power_consumption;            // Power consumption in watts
    double supply_voltage;               // Required supply voltage
};

// Base class for tube-based standard logic components
class TubeStandardLogicComponent : public AnalogNodeBase {
public:
    typedef TubeStandardLogicComponent CLASSNAME;

    TubeStandardLogicComponent(StandardLogicIC ic_type);
    virtual ~TubeStandardLogicComponent();

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "TubeStandardLogicComponent"; }

    // Get/set component type
    StandardLogicIC GetComponentType() const { return ic_type; }
    
    // Get parameters
    const TubeComponentParams& GetParams() const { return params; }
    TubeComponentParams& GetParams() { return params; }
    
    // Get/set pin values
    virtual void SetPinValue(int pin_id, double value);
    virtual double GetPinValue(int pin_id) const;
    
    // Get/set pin by name
    virtual void SetPinValue(const std::string& pin_name, double value);
    virtual double GetPinValue(const std::string& pin_name) const;
    
    // Get number of pins
    int GetPinCount() const { return params.pin_count; }
    
    // Get pin names
    const std::vector<std::string>& GetPinNames() const { return params.pin_names; }
    
    // Enable/disable component
    void SetEnabled(bool enabled) { is_enabled = enabled; }
    bool IsEnabled() const { return is_enabled; }
    
    // Reset the component
    virtual void Reset();
    
    // Get power consumption
    double GetPowerConsumption() const { return params.power_consumption; }
    
    // Get propagation delay
    double GetPropagationDelay() const { return params.propagation_delay; }
    
    // Get/set supply voltage
    void SetSupplyVoltage(double voltage) { params.supply_voltage = voltage; }
    double GetSupplyVoltage() const { return params.supply_voltage; }
    
    // Get/set current draw
    void SetCurrentDraw(double current) { params.current_draw = current; }
    double GetCurrentDraw() const { return params.current_draw; }
    
    // Get/set plate voltage
    void SetPlateVoltage(double voltage) { params.plate_voltage = voltage; }
    double GetPlateVoltage() const { return params.plate_voltage; }
    
    // Get/set heater voltage
    void SetHeaterVoltage(double voltage) { params.heater_voltage = voltage; }
    double GetHeaterVoltage() const { return params.heater_voltage; }
    
    // Get description
    const std::string& GetDescription() const { return params.description; }

protected:
    StandardLogicIC ic_type;
    TubeComponentParams params;
    std::vector<double> pin_values;
    bool is_enabled;
    
    // Vector of all tubes used in this component
    std::vector<std::unique_ptr<Tube>> component_tubes;
    
    // Internal processing method
    virtual void ProcessComponent() = 0;
    
    // Initialize component based on type
    virtual void InitializeComponent() = 0;
    
    // Find pin index by name
    int GetPinIndex(const std::string& pin_name) const;
    
    static constexpr double MIN_SUPPLY_VOLTAGE = 50.0;
    static constexpr double MAX_SUPPLY_VOLTAGE = 500.0;
    static constexpr double MIN_PLATE_VOLTAGE = 25.0;
    static constexpr double MAX_PLATE_VOLTAGE = 450.0;
    static constexpr double MIN_HEATER_VOLTAGE = 1.0;
    static constexpr double MAX_HEATER_VOLTAGE = 25.0;
    static constexpr double MIN_PROPAGATION_DELAY = 0.0;
    static constexpr double MAX_PROPAGATION_DELAY = 0.001;  // 1ms
};

// Template class for creating composite logic components
template<int InputCount, int OutputCount>
class TubeCompositeLogic : public TubeStandardLogicComponent {
public:
    typedef TubeCompositeLogic<InputCount, OutputCount> CLASSNAME;

    TubeCompositeLogic(StandardLogicIC ic_type);
    virtual ~TubeCompositeLogic() {}

    virtual String GetClassName() const override { 
        return "TubeCompositeLogic_" + std::to_string(InputCount) + "_" + std::to_string(OutputCount); 
    }

    // Set/get input pin values
    void SetInput(int input_id, double value);
    double GetInput(int input_id) const;
    
    // Set/get output pin values
    void SetOutput(int output_id, double value);
    double GetOutput(int output_id) const;

protected:
    std::array<double, InputCount> input_pins;
    std::array<double, OutputCount> output_pins;
    
    virtual void ProcessComponent() override;
    virtual void InitializeComponent() override;
    
    // Define the logic function (to be implemented in derived classes)
    virtual std::array<bool, OutputCount> ComputeOutputs(const std::array<bool, InputCount>& inputs) = 0;
    
    // Convert analog to digital and vice versa
    virtual std::array<bool, InputCount> AnalogToDigital() const;
    virtual void DigitalToAnalog(const std::array<bool, OutputCount>& outputs);
};

// Class for tube-based register bank
class TubeRegisterBank : public TubeStandardLogicComponent {
public:
    typedef TubeRegisterBank CLASSNAME;

    TubeRegisterBank(int register_count = 4, int register_width = 8);
    virtual ~TubeRegisterBank() {}

    virtual String GetClassName() const override { return "TubeRegisterBank"; }

    // Set/get register values
    void SetRegisterValue(int reg_id, unsigned int value);
    unsigned int GetRegisterValue(int reg_id) const;
    
    // Set control signals
    void SetClockSignal(double signal);
    void SetEnableSignal(double signal);
    void SetWriteEnable(int reg_id, double signal);
    
    // Get output of a specific register
    unsigned int GetOutputRegisterValue(int reg_id) const;

protected:
    int register_count;
    int register_width;
    std::vector<unsigned int> register_values;
    std::vector<bool> write_enables;
    std::vector<std::unique_ptr<TubeRegister>> registers;
    double clock_signal;
    double enable_signal;
    
    virtual void ProcessComponent() override;
    virtual void InitializeComponent() override;
    
    void ProcessRegisterBank();
};

// Class for tube-based ALU
class TubeALULogicLibrary : public TubeStandardLogicComponent {
public:
    typedef TubeALULogicLibrary CLASSNAME;

    TubeALULogicLibrary(int data_width = 8);
    virtual ~TubeALULogicLibrary() {}

    virtual String GetClassName() const override { return "TubeALULogicLibrary"; }
    
    // Set/get operands
    void SetOperandA(unsigned int value);
    void SetOperandB(unsigned int value);
    unsigned int GetOperandA() const { return operand_a; }
    unsigned int GetOperandB() const { return operand_b; }
    
    // Set function/operation
    void SetFunction(int func) { function = func; }
    int GetFunction() const { return function; }
    
    // Get result
    unsigned int GetResult() const { return result; }
    
    // Get flags
    bool GetZeroFlag() const { return zero_flag; }
    bool GetCarryFlag() const { return carry_flag; }
    bool GetOverflowFlag() const { return overflow_flag; }
    bool GetSignFlag() const { return sign_flag; }

protected:
    int data_width;
    unsigned int operand_a;
    unsigned int operand_b;
    int function;  // 0=ADD, 1=SUB, 2=AND, 3=OR, 4=XOR, 5=NOT, etc.
    unsigned int result;
    bool zero_flag;
    bool carry_flag;
    bool overflow_flag;
    bool sign_flag;
    
    std::unique_ptr<TubeArithmeticUnit> arithmetic_unit;
    std::vector<std::unique_ptr<TubeLogicGate>> logic_units;
    
    virtual void ProcessComponent() override;
    virtual void InitializeComponent() override;
    
    void ProcessALU();
    void ComputeFlags();
};

// Class for tube-based counter array
class TubeCounterArray : public TubeStandardLogicComponent {
public:
    typedef TubeCounterArray CLASSNAME;

    TubeCounterArray(int counter_count = 4, int counter_width = 8);
    virtual ~TubeCounterArray() {}

    virtual String GetClassName() const override { return "TubeCounterArray"; }

    // Set/get counter values
    void SetCounterValue(int counter_id, unsigned int value);
    unsigned int GetCounterValue(int counter_id) const;
    
    // Set control signals for a specific counter
    void SetClockSignal(int counter_id, double signal);
    void SetEnableSignal(int counter_id, double signal);
    void SetResetSignal(int counter_id, double signal);
    
    // Cascade control
    void SetCascadeEnable(bool enable) { cascade_enabled = enable; }
    bool IsCascadeEnabled() const { return cascade_enabled; }
    
    // Get output of a specific counter
    unsigned int GetOutputCounterValue(int counter_id) const;

protected:
    int counter_count;
    int counter_width;
    std::vector<unsigned int> counter_values;
    std::vector<double> clock_signals;
    std::vector<double> enable_signals;
    std::vector<double> reset_signals;
    std::vector<std::unique_ptr<TubeCounter>> counters;
    bool cascade_enabled;
    
    virtual void ProcessComponent() override;
    virtual void InitializeComponent() override;
    
    void ProcessCounterArray();
};

// Library class that manages all standard tube logic components
class TubeLogicLibrary {
public:
    TubeLogicLibrary();
    virtual ~TubeLogicLibrary();
    
    // Create a standard logic component
    std::unique_ptr<TubeStandardLogicComponent> CreateComponent(StandardLogicIC ic_type);
    
    // Get component parameters
    const TubeComponentParams* GetComponentParams(StandardLogicIC ic_type) const;
    
    // Get list of all supported components
    std::vector<StandardLogicIC> GetSupportedComponents() const;
    
    // Register a custom component
    void RegisterCustomComponent(StandardLogicIC ic_type, const TubeComponentParams& params);
    
    // Get the library statistics
    int GetComponentCount() const { return supported_components.size(); }
    
    // Get tube count for a specific IC type
    int GetTubeCountForIC(StandardLogicIC ic_type) const;

private:
    std::unordered_map<StandardLogicIC, TubeComponentParams> component_params;
    std::vector<StandardLogicIC> supported_components;
    
    void InitializeLibrary();
    void AddStandardComponent(StandardLogicIC ic_type, const std::string& name, 
                              const std::string& description, int pin_count, 
                              const std::vector<std::string>& pin_names, 
                              TubeTechnology tech, int tube_count, 
                              double prop_delay, double power_cons);
};

#endif