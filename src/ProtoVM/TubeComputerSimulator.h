#ifndef _ProtoVM_TubeComputerSimulator_h_
#define _ProtoVM_TubeComputerSimulator_h_

#include "AnalogCommon.h"
#include "TubeComputerSystems.h"
#include "TubeLogicLibrary.h"
#include "TubeClockOscillators.h"
#include "TubeArithmeticUnits.h"
#include "TubeCountersRegisters.h"
#include "TubeMultiplexers.h"
#include <vector>
#include <memory>
#include <map>
#include <string>
#include <functional>

// Enum for computer architectures
enum class ComputerArchitecture {
    ENIAC,           // ENIAC (Electronic Numerical Integrator and Computer)
    EDVAC,           // EDVAC (Electronic Discrete Variable Automatic Computer)
    EDSAC,           // EDSAC (Electronic Delay Storage Automatic Calculator)
    MANCHESTER,      // Manchester Mark 1
    COLUSSUS,        // Colossus (codebreaking computer)
    ILLIAC,          // ILLIAC (Illinois Automatic Computer)
    WHIRLWIND,       // MIT Whirlwind
    SAGE,            // SAGE (Semi-Automatic Ground Environment)
    IBM_701,         // IBM 701 (Defense Calculator)
    CUSTOM           // Custom architecture
};

// Enum for memory types
enum class MemoryType {
    RAMT,           // Random Access Memory Tube (Williams-Kilburn tube)
    DRAMT,          // Delay line memory (mercury acoustic delay lines)
    ROMT,           // Read-only memory (plug boards, wiring)
    MAGCORE         // Magnetic core memory (later addition)
};

// Structure to define a computer memory segment
struct MemorySegment {
    std::string name;
    int start_address;
    int size;
    MemoryType type;
    std::vector<int> data;
    bool is_readable;
    bool is_writable;
    
    MemorySegment(const std::string& n = "unnamed", int start = 0, int sz = 1024, 
                  MemoryType mt = MemoryType::RAMT, bool readable = true, bool writeable = true)
        : name(n), start_address(start), size(sz), type(mt), 
          is_readable(readable), is_writable(writeable) {
        data.resize(size, 0);
    }
};

// Structure to define a computer register
struct ProcessorRegister {
    std::string name;
    int width;  // in bits
    int value;
    bool is_general_purpose;
    
    ProcessorRegister(const std::string& n = "unnamed", int w = 32, bool gp = true)
        : name(n), width(w), value(0), is_general_purpose(gp) {}
};

// Structure to define an instruction
struct Instruction {
    std::string mnemonic;
    int opcode;
    int operand_count;
    std::vector<int> operands;
    int cycles;  // Execution cycles
    
    Instruction(const std::string& mn = "NOP", int op = 0, int ops = 0, int cyc = 1)
        : mnemonic(mn), opcode(op), operand_count(ops), cycles(cyc) {
        operands.resize(operand_count, 0);
    }
};

// Base class for tube-based computer simulation
class TubeComputer : public AnalogNodeBase {
public:
    typedef TubeComputer CLASSNAME;

    TubeComputer(ComputerArchitecture arch = ComputerArchitecture::CUSTOM);
    virtual ~TubeComputer();

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "TubeComputer"; }

    // Get/set architecture
    ComputerArchitecture GetArchitecture() const { return architecture; }
    void SetArchitecture(ComputerArchitecture arch) { architecture = arch; }
    
    // Get computer name
    std::string GetComputerName() const;
    
    // Memory operations
    void SetMemory(int address, int value);
    int GetMemory(int address) const;
    
    // Register operations
    void SetRegister(const std::string& name, int value);
    int GetRegister(const std::string& name) const;
    
    // Add memory segment
    void AddMemorySegment(const MemorySegment& segment);
    
    // Add processor register
    void AddRegister(const ProcessorRegister& reg);
    
    // Get/set clock speed
    void SetClockSpeed(double hz) { clock_speed = std::max(1.0, std::min(100000.0, hz)); }
    double GetClockSpeed() const { return clock_speed; }
    
    // Get/set word size
    void SetWordSize(int bits) { word_size = std::max(8, std::min(64, bits)); }
    int GetWordSize() const { return word_size; }
    
    // Get/set memory size
    void SetMemorySize(int words) { memory_size = std::max(64, std::min(65536, words)); }
    int GetMemorySize() const override { return memory_size; }
    
    // Enable/disable the computer
    void SetEnabled(bool enabled) { is_enabled = enabled; }
    bool IsEnabled() const { return is_enabled; }
    
    // Reset the computer
    virtual void Reset();
    
    // Execute next instruction
    virtual void ExecuteNextInstruction();
    
    // Load program into memory
    virtual void LoadProgram(const std::vector<Instruction>& program, int start_address = 0);
    
    // Load data into memory
    virtual void LoadData(const std::vector<int>& data, int start_address = 0);
    
    // Get/set program counter
    void SetProgramCounter(int addr) { program_counter = addr; }
    int GetProgramCounter() const { return program_counter; }
    
    // Get/set instruction register
    void SetInstructionRegister(int ir) { instruction_register = ir; }
    int GetInstructionRegister() const { return instruction_register; }
    
    // Get/set accumulator
    void SetAccumulator(int acc) { accumulator = acc; }
    int GetAccumulator() const { return accumulator; }
    
    // Get/set address register
    void SetAddressRegister(int addr) { address_register = addr; }
    int GetAddressRegister() const { return address_register; }
    
    // Get/set status flags
    void SetCarryFlag(bool carry) { carry_flag = carry; }
    bool GetCarryFlag() const { return carry_flag; }
    
    void SetZeroFlag(bool zero) { zero_flag = zero; }
    bool GetZeroFlag() const { return zero_flag; }
    
    void SetOverflowFlag(bool overflow) { overflow_flag = overflow; }
    bool GetOverflowFlag() const { return overflow_flag; }
    
    // Get total instruction count
    int GetInstructionCount() const { return instruction_count; }
    
    // Get total operation count
    int GetOperationCount() const { return operation_count; }
    
    // Get power consumption
    double GetPowerConsumption() const;
    
    // Get simulation statistics
    double GetSimulationTime() const { return simulation_time; }
    int GetTubecount() const { return tube_count; }
    
    // Perform system diagnostics
    virtual void RunDiagnostics();
    
    // Get diagnostic report
    std::string GetDiagnosticReport() const { return diagnostic_report; }
    
    // Simulate tube failures
    void SimulateTubeFailures(double failure_rate = 0.001);  // Default to 0.1% per tick

protected:
    ComputerArchitecture architecture;
    bool is_enabled;
    double clock_speed;              // Clock speed in Hz
    int word_size;                   // Word size in bits
    int memory_size;                 // Memory size in words
    int program_counter;             // Program counter
    int instruction_register;        // Instruction register
    int accumulator;                 // Accumulator register
    int address_register;            // Address register
    bool carry_flag;                 // Carry flag
    bool zero_flag;                  // Zero flag
    bool overflow_flag;              // Overflow flag
    double simulation_time;          // Total simulation time
    int instruction_count;           // Total instructions executed
    int operation_count;             // Total operations performed
    int tube_count;                  // Total number of tubes in the system
    double total_power_consumption;  // Total power consumption
    std::string diagnostic_report;   // Diagnostic report
    
    // Computer memory
    std::vector<MemorySegment> memory_segments;
    std::map<int, int> main_memory;  // Main memory (sparse for large memories)
    
    // Processor registers
    std::vector<ProcessorRegister> registers;
    std::map<std::string, int> register_map;
    
    // Components of the computer
    std::unique_ptr<TubeClockOscillator> clock_generator;
    std::unique_ptr<TubeALU> alu;
    std::unique_ptr<TubeRegisterBank> register_bank;
    std::unique_ptr<TubeCounterArray> counter_array;
    std::unique_ptr<TubeMuxDemux> control_mux_demux;
    
    // Library of standard logic components
    TubeLogicLibrary logic_library;
    
    // Internal processing
    virtual void ProcessComputerCycle();
    virtual void ProcessMemoryAccess();
    virtual void ProcessInstructionExecution();
    virtual void ProcessControlFlow();
    virtual void UpdateStatistics();
    
    // Initialize the computer based on architecture
    virtual void InitializeComputer();
    
    // Map physical address to memory segment
    MemorySegment* FindMemorySegment(int address);
    
    static constexpr double MIN_CLOCK_SPEED = 1.0;    // 1 Hz
    static constexpr double MAX_CLOCK_SPEED = 100000.0; // 100 kHz
    static constexpr int MIN_WORD_SIZE = 8;           // 8 bits
    static constexpr int MAX_WORD_SIZE = 64;          // 64 bits
    static constexpr int MIN_MEMORY_SIZE = 64;        // 64 words
    static constexpr int MAX_MEMORY_SIZE = 65536;     // 64K words
};

// ENIAC computer simulation
class ENIACComputer : public TubeComputer {
public:
    typedef ENIACComputer CLASSNAME;

    ENIACComputer();
    virtual ~ENIACComputer() {}

    virtual String GetClassName() const override { return "ENIACComputer"; }
    
    // ENIAC-specific functionality
    void SetProgrammingMode(bool fixed) { fixed_program_mode = fixed; }
    bool IsFixedProgramMode() const { return fixed_program_mode; }
    
    // Set program switches
    void SetProgramSwitches(const std::vector<bool>& switches);
    std::vector<bool> GetProgramSwitches() const { return program_switches; }
    
    // Set accumulator values
    void SetAccumulator(int unit, int value);
    int GetAccumulator(int unit) const;

protected:
    bool fixed_program_mode;              // Whether using fixed or rewired programming
    std::vector<int> accumulators;        // 20 accumulators (A units)
    std::vector<bool> program_switches;   // Program switches (6000+ switches)
    std::vector<int> multiplier_registers; // B units (multipliers)
    std::vector<int> function_tables;     // Function table units
    
    virtual void InitializeComputer() override;
    virtual void ProcessComputerCycle() override;
    
    void InitializeENIACHardware();
    void ProcessENIACOperations();
};

// EDSAC computer simulation
class EDSACComputer : public TubeComputer {
public:
    typedef EDSACComputer CLASSNAME;

    EDSACComputer();
    virtual ~EDSACComputer() {}

    virtual String GetClassName() const override { return "EDSACComputer"; }
    
    // EDSAC-specific functions
    void SetStore(const std::vector<int>& store) { main_store = store; }
    std::vector<int> GetStore() const { return main_store; }
    
    void SetCurrentInstruction(int instruction) { current_instruction = instruction; }
    int GetCurrentInstruction() const { return current_instruction; }
    
    void SetAccumulator(int acc) { accumulator = acc; }
    int GetAccumulator() const { return accumulator; }
    
    void SetMultiplier(int mult) { multiplier = mult; }
    int GetMultiplier() const { return multiplier; }
    
    // Execute next instruction
    void ExecuteNextInstruction() override;
    
    // Load program into memory
    void LoadProgram(const std::vector<Instruction>& program, int start_address = 0) override;

protected:
    std::vector<int> main_store;        // Main store (memory)
    int current_instruction;            // Current instruction being executed
    int multiplier;                     // Multiplier register
    int current_address;                // Current memory address
    
    virtual void InitializeComputer() override;
    virtual void ProcessComputerCycle() override;
    
    void InitializeEDSACHardware();
    void ProcessEDSACOperations();
};

// Universal Tube Computer Simulator class
class TubeComputerSimulator {
public:
    TubeComputerSimulator();
    virtual ~TubeComputerSimulator();
    
    // Create a computer of a specified architecture
    std::unique_ptr<TubeComputer> CreateComputer(ComputerArchitecture arch);
    
    // Create a custom computer with specified parameters
    std::unique_ptr<TubeComputer> CreateCustomComputer(int memory_size, int word_size, 
                                                      double clock_speed, const std::string& name = "Custom");
    
    // Run simulation for a specified duration
    void RunSimulation(TubeComputer* computer, double duration_seconds);
    
    // Run simulation for a specified number of instructions
    void RunSimulationForInstructions(TubeComputer* computer, int instruction_count);
    
    // Get simulation statistics
    struct SimulationStats {
        int total_instructions_executed;
        int total_operations_performed;
        double total_simulated_time;
        double power_consumption;
        double average_tps;  // Ticks per second (simulated performance)
        int tube_count;
        int memory_accesses;
        int io_operations;
    };
    
    SimulationStats GetSimulationStats(const TubeComputer* computer) const;
    
    // Load a program from file
    bool LoadProgramFromFile(TubeComputer* computer, const std::string& filename);
    
    // Save memory state to file
    bool SaveMemoryToFile(const TubeComputer* computer, const std::string& filename) const;
    
    // Get list of supported architectures
    std::vector<ComputerArchitecture> GetSupportedArchitectures() const;
    
    // Perform system-wide diagnostics
    void RunSystemDiagnostics(TubeComputer* computer);
    
    // Simulate tube aging effects
    void SimulateTubeAging(TubeComputer* computer, double years_to_simulate);
};

#endif