#include "TubeComputerSimulator.h"
#include <algorithm>
#include <sstream>

// TubeComputer implementation
TubeComputer::TubeComputer(ComputerArchitecture arch) 
    : architecture(arch)
    , is_enabled(true)
    , clock_speed(1000.0)  // 1kHz default
    , word_size(36)        // 36-bit word size for systems like ENIAC
    , memory_size(1024)    // 1KB default
    , program_counter(0)
    , instruction_register(0)
    , accumulator(0)
    , address_register(0)
    , carry_flag(false)
    , zero_flag(false)
    , overflow_flag(false)
    , simulation_time(0.0)
    , instruction_count(0)
    , operation_count(0)
    , tube_count(0)
    , total_power_consumption(0.0)
{
    InitializeComputer();
}

TubeComputer::~TubeComputer() {
    // Cleanup handled by destructors
}

bool TubeComputer::Tick() {
    if (!is_enabled) {
        return true;
    }
    
    // Process a single computer cycle
    ProcessComputerCycle();
    
    // Update statistics
    UpdateStatistics();
    
    return true;
}

void TubeComputer::InitializeComputer() {
    // Initialize memory segments based on architecture
    switch (architecture) {
        case ComputerArchitecture::ENIAC:
            AddMemorySegment(MemorySegment("Input Table", 0x0000, 1024, MemoryType::ROMT, true, false));
            AddMemorySegment(MemorySegment("Accumulator Storage", 0x0400, 2048, MemoryType::RAMT, true, true));
            AddMemorySegment(MemorySegment("Output Table", 0x0C00, 1024, MemoryType::ROMT, true, false));
            break;
            
        case ComputerArchitecture::EDSAC:
            AddMemorySegment(MemorySegment("Main Store", 0x0000, 1024, MemoryType::DRAMT, true, true));
            // Add more segments based on EDSAC architecture
            break;
            
        case ComputerArchitecture::COLUSSUS:
            AddMemorySegment(MemorySegment("Paper Tape Reader", 0x0000, 512, MemoryType::ROMT, true, false));
            AddMemorySegment(MemorySegment("Key Storage", 0x0200, 1024, MemoryType::RAMT, true, true));
            break;
            
        case ComputerArchitecture::CUSTOM:
        case ComputerArchitecture::IBM_701:
        case ComputerArchitecture::ILLIAC:
        case ComputerArchitecture::MANCHESTER:
        case ComputerArchitecture::WHIRLWIND:
        case ComputerArchitecture::SAGE:
        case ComputerArchitecture::EDVAC:
        default:
            AddMemorySegment(MemorySegment("Main Memory", 0x0000, memory_size, MemoryType::RAMT, true, true));
            break;
    }
    
    // Add registers based on architecture
    AddRegister(ProcessorRegister("PC", word_size, false));  // Program counter
    AddRegister(ProcessorRegister("IR", word_size, false));  // Instruction register
    AddRegister(ProcessorRegister("ACC", word_size, false)); // Accumulator
    
    // Initialize components
    clock_generator = std::make_unique<TubeHartleyClock>();  // Use Hartley clock for stability
    clock_generator->SetFrequency(clock_speed);
    
    alu = std::make_unique<TubeALU>(word_size);
    register_bank = std::make_unique<TubeRegisterBank>(8, word_size);  // 8 general purpose registers
    counter_array = std::make_unique<TubeCounterArray>(4, word_size);  // 4 counter registers
    control_mux_demux = std::make_unique<TubeMuxDemux>(8);  // Control signals
    
    // Initialize tube count estimation
    tube_count = (architecture == ComputerArchitecture::ENIAC) ? 17000 :  // ENIAC had ~17,000 tubes
                 (architecture == ComputerArchitecture::COLUSSUS) ? 2000 : // Colossus Mark 2 had ~2,000
                 500;  // Reasonable estimate for other architectures
    
    Reset();
}

void TubeComputer::Reset() {
    program_counter = 0;
    instruction_register = 0;
    accumulator = 0;
    address_register = 0;
    carry_flag = false;
    zero_flag = false;
    overflow_flag = false;
    
    instruction_count = 0;
    operation_count = 0;
    simulation_time = 0.0;
    
    // Reset all registers
    for (auto& reg : registers) {
        reg.value = 0;
    }
    
    // Reset register map
    for (auto& pair : register_map) {
        pair.second = 0;
    }
    
    // Reset ALU
    if (alu) {
        alu->SetOperandA(0);
        alu->SetOperandB(0);
        alu->SetFunction(0);  // Default to ADD
    }
    
    // Reset components
    if (clock_generator) {
        clock_generator->Reset();
    }
    if (register_bank) {
        for (int i = 0; i < 8; i++) {
            register_bank->SetRegisterValue(i, 0);
        }
    }
    if (counter_array) {
        for (int i = 0; i < 4; i++) {
            counter_array->SetCounterValue(i, 0);
        }
    }
}

void TubeComputer::AddMemorySegment(const MemorySegment& segment) {
    memory_segments.push_back(segment);
    
    // Initialize main memory if it's a RAM segment
    if (segment.type == MemoryType::RAMT || segment.type == MemoryType::DRAMT) {
        for (int i = segment.start_address; i < segment.start_address + segment.size; i++) {
            main_memory[i] = 0;
        }
    }
}

void TubeComputer::AddRegister(const ProcessorRegister& reg) {
    registers.push_back(reg);
    register_map[reg.name] = 0;  // Initialize to 0
}

void TubeComputer::SetMemory(int address, int value) {
    MemorySegment* seg = FindMemorySegment(address);
    if (seg && seg->is_writable) {
        if (seg->type == MemoryType::RAMT || seg->type == MemoryType::DRAMT) {
            main_memory[address] = value;
        } else if (seg->type == MemoryType::ROMT) {
            // For ROM, we might store the value in the segment data if it's writable
            int offset = address - seg->start_address;
            if (offset >= 0 && offset < seg->size) {
                seg->data[offset] = value;
            }
        }
    }
}

int TubeComputer::GetMemory(int address) const {
    MemorySegment* seg = const_cast<TubeComputer*>(this)->FindMemorySegment(address);
    if (seg && seg->is_readable) {
        if (seg->type == MemoryType::RAMT || seg->type == MemoryType::DRAMT) {
            auto it = main_memory.find(address);
            return (it != main_memory.end()) ? it->second : 0;
        } else if (seg->type == MemoryType::ROMT) {
            int offset = address - seg->start_address;
            if (offset >= 0 && offset < seg->size) {
                return seg->data[offset];
            }
        }
    }
    return 0;
}

void TubeComputer::SetRegister(const std::string& name, int value) {
    auto it = register_map.find(name);
    if (it != register_map.end()) {
        it->second = value;
    }
    
    // Also check in the registers vector
    for (auto& reg : registers) {
        if (reg.name == name) {
            reg.value = value;
            break;
        }
    }
}

int TubeComputer::GetRegister(const std::string& name) const {
    auto it = register_map.find(name);
    if (it != register_map.end()) {
        return it->second;
    }
    
    // Also check in the registers vector
    for (const auto& reg : registers) {
        if (reg.name == name) {
            return reg.value;
        }
    }
    
    return 0;
}

void TubeComputer::ExecuteNextInstruction() {
    // Read the instruction from memory at the program counter
    int instruction = GetMemory(program_counter);
    instruction_register = instruction;
    
    // Increment program counter
    program_counter++;
    
    // Decode the instruction
    int opcode = (instruction >> (word_size - 8)) & 0xFF;  // Assume top 8 bits are opcode
    int operand = instruction & ((1 << (word_size - 8)) - 1);  // Remaining bits are operand
    
    // Execute the instruction
    switch (opcode) {
        case 0x00:  // NOP - No operation
            break;
            
        case 0x01:  // LOAD - Load accumulator with value from memory
            accumulator = GetMemory(operand);
            break;
            
        case 0x02:  // STORE - Store accumulator to memory
            SetMemory(operand, accumulator);
            break;
            
        case 0x03:  // ADD - Add memory value to accumulator
            {
                int mem_val = GetMemory(operand);
                long long result = static_cast<long long>(accumulator) + mem_val;
                
                // Check for overflow
                if (result > (1 << (word_size - 1)) - 1) {
                    overflow_flag = true;
                    result = result & ((1 << word_size) - 1);  // Wrap around
                } else {
                    overflow_flag = false;
                }
                
                // Check for carry
                if (result > (1 << (word_size - 1)) - 1) {
                    carry_flag = true;
                } else {
                    carry_flag = false;
                }
                
                accumulator = static_cast<int>(result);
                zero_flag = (accumulator == 0);
            }
            break;
            
        case 0x04:  // SUB - Subtract memory value from accumulator
            {
                int mem_val = GetMemory(operand);
                long long result = static_cast<long long>(accumulator) - mem_val;
                
                // Check for underflow
                if (result < -(1 << (word_size - 1))) {
                    overflow_flag = true;
                    result = result & ((1 << word_size) - 1);  // Wrap around
                } else {
                    overflow_flag = false;
                }
                
                accumulator = static_cast<int>(result);
                zero_flag = (accumulator == 0);
            }
            break;
            
        case 0x05:  // JUMP - Jump to address
            program_counter = operand;
            break;
            
        case 0x06:  // JUMP IF ZERO - Jump to address if accumulator is zero
            if (zero_flag) {
                program_counter = operand;
            }
            break;
            
        case 0x07:  // JUMP IF POSITIVE - Jump to address if accumulator is positive
            if (accumulator >= 0) {
                program_counter = operand;
            }
            break;
            
        default:
            // Unknown instruction
            break;
    }
    
    instruction_count++;
}

void TubeComputer::LoadProgram(const std::vector<Instruction>& program, int start_address) {
    for (size_t i = 0; i < program.size(); i++) {
        // Convert instruction to memory format
        // This is a simplified approach - real encoding would be more complex
        int encoded_instruction = (program[i].opcode << (word_size - 8)) | program[i].operands[0];
        SetMemory(start_address + i, encoded_instruction);
    }
}

void TubeComputer::LoadData(const std::vector<int>& data, int start_address) {
    for (size_t i = 0; i < data.size(); i++) {
        SetMemory(start_address + i, data[i]);
    }
}

std::string TubeComputer::GetComputerName() const {
    switch (architecture) {
        case ComputerArchitecture::ENIAC: return "ENIAC Computer";
        case ComputerArchitecture::EDSAC: return "EDSAC Computer";
        case ComputerArchitecture::COLUSSUS: return "Colossus Computer";
        case ComputerArchitecture::EDVAC: return "EDVAC Computer";
        case ComputerArchitecture::MANCHESTER: return "Manchester Mark 1 Computer";
        case ComputerArchitecture::ILLIAC: return "ILLIAC Computer";
        case ComputerArchitecture::WHIRLWIND: return "MIT Whirlwind Computer";
        case ComputerArchitecture::SAGE: return "SAGE Computer";
        case ComputerArchitecture::IBM_701: return "IBM 701";
        case ComputerArchitecture::CUSTOM: return "Custom Tube Computer";
        default: return "Unknown Computer";
    }
}

double TubeComputer::GetPowerConsumption() const {
    // Estimate based on architecture and tube count
    // Real early computers consumed lots of power (ENIAC used 150kW!)
    double power_per_tube = 0.25;  // 0.25W per tube estimate
    return tube_count * power_per_tube;
}

void TubeComputer::ProcessComputerCycle() {
    // Process the clock
    if (clock_generator) {
        clock_generator->Tick();
        
        // Execute instruction if clock edge triggers
        static double prev_clock = 0.0;
        double current_clock = clock_generator->GetClockSignal();
        
        // Execute on rising edge
        if (current_clock > TUBE_THRESHOLD && prev_clock < TUBE_THRESHOLD && is_enabled) {
            ExecuteNextInstruction();
        }
        
        prev_clock = current_clock;
    }
    
    // Process ALU if needed
    if (alu) {
        alu->Tick();
    }
    
    // Process register bank
    if (register_bank) {
        register_bank->Tick();
    }
    
    // Process counter array
    if (counter_array) {
        counter_array->Tick();
    }
    
    // Process control mux/demux
    if (control_mux_demux) {
        control_mux_demux->Tick();
    }
    
    operation_count++;
}

void TubeComputer::ProcessMemoryAccess() {
    // Handle memory accesses - this would be called during instruction execution
    // For now, we'll just update stats
}

void TubeComputer::ProcessInstructionExecution() {
    // Process the execution of the current instruction
    // This is handled in ExecuteNextInstruction, so this is a placeholder
}

void TubeComputer::ProcessControlFlow() {
    // Handle control flow changes like jumps, branches, etc.
    // This is handled in instruction execution, so this is a placeholder
}

void TubeComputer::UpdateStatistics() {
    // Update simulation time based on clock speed
    static double last_time = 0.0;
    simulation_time += 1.0 / clock_speed;
    
    // Update power consumption
    total_power_consumption = GetPowerConsumption();
}

MemorySegment* TubeComputer::FindMemorySegment(int address) {
    for (auto& seg : memory_segments) {
        if (address >= seg.start_address && address < seg.start_address + seg.size) {
            return &seg;
        }
    }
    return nullptr;
}

void TubeComputer::RunDiagnostics() {
    std::stringstream report;
    report << "Diagnostic Report for " << GetComputerName() << "\n";
    report << "=========================================\n";
    report << "Architecture: " << (int)architecture << "\n";
    report << "Word Size: " << word_size << " bits\n";
    report << "Memory Size: " << memory_size << " words\n";
    report << "Clock Speed: " << clock_speed << " Hz\n";
    report << "Power Consumption: " << GetPowerConsumption() << " watts\n";
    report << "Tube Count: " << tube_count << " tubes\n";
    report << "Total Instructions: " << instruction_count << "\n";
    report << "Total Operations: " << operation_count << "\n";
    report << "Simulation Time: " << simulation_time << " seconds\n";
    report << "Current PC: 0x" << std::hex << program_counter << std::dec << "\n";
    report << "Accumulator: " << accumulator << "\n";
    report << "Status Flags - Carry: " << carry_flag << ", Zero: " << zero_flag << ", Overflow: " << overflow_flag << "\n";
    
    diagnostic_report = report.str();
}

void TubeComputer::SimulateTubeFailures(double failure_rate) {
    // Simulate random tube failures based on the rate
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<> dis(0.0, 1.0);
    
    if (dis(gen) < failure_rate) {
        // For this simulation, reduce performance rather than completely fail
        // This could manifest as incorrect computations, slower processing, etc.
        instruction_count = static_cast<int>(instruction_count * 0.99);  // Slight slowdown
    }
}


// ENIACComputer implementation
ENIACComputer::ENIACComputer() 
    : TubeComputer(ComputerArchitecture::ENIAC)
    , fixed_program_mode(true)
{
    InitializeComputer();
}

void ENIACComputer::InitializeComputer() {
    // ENIAC-specific initialization
    clock_speed = 100000.0;  // 100kHz clock for ENIAC's adder units
    word_size = 10;          // 10-decimal digit words
    memory_size = 20;        // 20 accumulators with 10 decimal digits each
    
    // Initialize ENIAC-specific components
    InitializeENIACHardware();
    
    TubeComputer::InitializeComputer();
}

void ENIACComputer::InitializeENIACHardware() {
    // Initialize the 20 accumulators
    accumulators.resize(20, 0);
    
    // Initialize the 6000+ program switches
    program_switches.resize(6000, false);
    
    // Initialize multiplier registers (B units)
    multiplier_registers.resize(1, 0);
    
    // Initialize function table units
    function_tables.resize(1, 0);
    
    // Set up memory segments to reflect ENIAC architecture
    memory_segments.clear();
    for (int i = 0; i < 20; i++) {
        std::string name = "Accumulator " + std::to_string(i);
        memory_segments.emplace_back(name, i * 10, 10, MemoryType::RAMT, true, true);
    }
    
    // Add function table memory
    memory_segments.emplace_back("Function Tables", 200, 100, MemoryType::ROMT, true, false);
    
    // Add input/output tables
    memory_segments.emplace_back("Input Tables", 300, 100, MemoryType::ROMT, true, false);
    memory_segments.emplace_back("Output Tables", 400, 100, MemoryType::ROMT, false, true);
    
    // Initialize registers
    registers.clear();
    AddRegister(ProcessorRegister("A0", word_size, false));  // Accumulator 0
    AddRegister(ProcessorRegister("A1", word_size, false));  // Accumulator 1
    // Add more registers as needed...
    
    tube_count = 17000;  // ENIAC had approximately 17,000 tubes
}

void ENIACComputer::ProcessComputerCycle() {
    ProcessENIACOperations();
    
    // Call the parent class processing
    TubeComputer::ProcessComputerCycle();
}

void ENIACComputer::ProcessENIACOperations() {
    // Process ENIAC-specific operations
    // In fixed program mode, operations are hardwired
    if (fixed_program_mode) {
        // Simulate the execution of a simple program based on switch settings
        // This is a simplified version - in real ENIAC, operators manually
        // wired the units together with patch cables
        int result = 0;
        
        // Example: Add accumulator 1 and accumulator 2, store in accumulator 3
        if (program_switches[0] && program_switches[1]) {  // If switches 0 and 1 are on
            result = accumulators[0] + accumulators[1];
            if (result > 9999999999) result = result % 10000000000; // 10 digit wraparound
            accumulators[2] = result;
        }
        
        // Example: Multiply accumulator 3 by multiplier register 1, store in accumulator 4
        if (program_switches[2] && program_switches[3]) {  // If switches 2 and 3 are on
            result = accumulators[2] * multiplier_registers[0];
            if (result > 9999999999) result = result % 10000000000; // 10 digit wraparound
            accumulators[3] = result;
        }
    }
}

void ENIACComputer::SetProgramSwitches(const std::vector<bool>& switches) {
    program_switches = switches;
    if (program_switches.size() < 6000) {
        program_switches.resize(6000, false);  // Pad with false values
    }
}

void ENIACComputer::SetAccumulator(int unit, int value) {
    if (unit >= 0 && unit < 20) {
        accumulators[unit] = value;
        
        // Also update in main memory
        SetMemory(unit * 10, value);
    }
}

int ENIACComputer::GetAccumulator(int unit) const {
    if (unit >= 0 && unit < 20) {
        return accumulators[unit];
    }
    return 0;
}


// EDSACComputer implementation
EDSACComputer::EDSACComputer() 
    : TubeComputer(ComputerArchitecture::EDSAC)
    , current_instruction(0)
    , multiplier(0)
    , current_address(0)
{
    InitializeComputer();
}

void EDSACComputer::InitializeComputer() {
    // EDSAC-specific initialization
    clock_speed = 500.0;  // 500Hz clock
    word_size = 18;       // 18-bit words
    memory_size = 1024;   // 1K words
    
    // Initialize EDSAC-specific components
    InitializeEDSACHardware();
    
    TubeComputer::InitializeComputer();
}

void EDSACComputer::InitializeEDSACHardware() {
    // Initialize the main store (memory)
    main_store.resize(1024, 0);
    
    // Set up memory segments to reflect EDSAC architecture
    memory_segments.clear();
    memory_segments.emplace_back("Main Store", 0, 1024, MemoryType::DRAMT, true, true);
    
    // Initialize registers
    registers.clear();
    AddRegister(ProcessorRegister("PC", word_size, false));   // Program counter
    AddRegister(ProcessorRegister("MQ", word_size, false));   // Multiplier quotient register
    AddRegister(ProcessorRegister("MAR", word_size, false));  // Memory address register
    
    tube_count = 3000;  // EDSAC had approximately 3,000 tubes
}

void EDSACComputer::ProcessComputerCycle() {
    ProcessEDSACOperations();
    
    // Call the parent class processing
    TubeComputer::ProcessComputerCycle();
}

void EDSACComputer::ProcessEDSACOperations() {
    // Execute the next instruction in the sequence
    if (current_address >= 0 && current_address < main_store.size()) {
        int instruction = main_store[current_address];
        current_instruction = instruction;
        
        // EDSAC instruction format: [OPCODE][ADDRESS]
        // Here we'll simulate a few basic operations
        int opcode = (instruction >> 13) & 0x1F;  // Top 5 bits for opcode
        int address = instruction & 0x1FFF;       // Bottom 13 bits for address
        
        switch (opcode) {
            case 0:  // NOP (No operation)
                break;
            case 1:  // LDA (Load accumulator)
                accumulator = main_store[address % main_store.size()];
                break;
            case 2:  // STO (Store accumulator)
                main_store[address % main_store.size()] = accumulator;
                break;
            case 3:  // ADD (Add)
                accumulator += main_store[address % main_store.size()];
                break;
            case 4:  // SUB (Subtract)
                accumulator -= main_store[address % main_store.size()];
                break;
            case 5:  // Input
                // Simulate input operation
                break;
            case 6:  // Output
                // Simulate output operation
                break;
            case 7:  // Conditional skip (E - Effective address)
                if (accumulator >= 0) {
                    current_address++;  // Skip next instruction
                }
                break;
            default:
                // Unknown opcode, do nothing
                break;
        }
        
        // Increment program counter (unless we're skipping)
        if (opcode != 7 || accumulator < 0) {
            current_address = (current_address + 1) % main_store.size();
        }
    }
}

void EDSACComputer::ExecuteNextInstruction() {
    ProcessEDSACOperations();
    
    instruction_count++;
}

void EDSACComputer::LoadProgram(const std::vector<Instruction>& program, int start_address) {
    int addr = start_address;
    for (const auto& instr : program) {
        if (addr < main_store.size()) {
            // Encode the instruction in EDSAC format
            int encoded = (instr.opcode << 13) | (instr.operands.empty() ? 0 : instr.operands[0]);
            main_store[addr] = encoded;
            addr = (addr + 1) % main_store.size();
        }
    }
}


// TubeComputerSimulator implementation
TubeComputerSimulator::TubeComputerSimulator() {
    // Initialize the simulator
}

TubeComputerSimulator::~TubeComputerSimulator() {
    // Cleanup handled by destructors
}

std::unique_ptr<TubeComputer> TubeComputerSimulator::CreateComputer(ComputerArchitecture arch) {
    switch (arch) {
        case ComputerArchitecture::ENIAC:
            return std::make_unique<ENIACComputer>();
            
        case ComputerArchitecture::EDSAC:
            return std::make_unique<EDSACComputer>();
            
        case ComputerArchitecture::COLUSSUS:
        case ComputerArchitecture::EDVAC:
        case ComputerArchitecture::MANCHESTER:
        case ComputerArchitecture::ILLIAC:
        case ComputerArchitecture::WHIRLWIND:
        case ComputerArchitecture::SAGE:
        case ComputerArchitecture::IBM_701:
        case ComputerArchitecture::CUSTOM:
        default:
            return std::make_unique<TubeComputer>(arch);
    }
}

std::unique_ptr<TubeComputer> TubeComputerSimulator::CreateCustomComputer(int memory_size, int word_size, 
                                                                        double clock_speed, const std::string& name) {
    auto computer = std::make_unique<TubeComputer>(ComputerArchitecture::CUSTOM);
    computer->SetMemorySize(memory_size);
    computer->SetWordSize(word_size);
    computer->SetClockSpeed(clock_speed);
    
    // Add a generic memory segment
    computer->AddMemorySegment(MemorySegment("Main Memory", 0x0000, memory_size, MemoryType::RAMT, true, true));
    
    return computer;
}

void TubeComputerSimulator::RunSimulation(TubeComputer* computer, double duration_seconds) {
    if (!computer) return;
    
    double start_time = computer->GetSimulationTime();
    double target_time = start_time + duration_seconds;
    
    while (computer->GetSimulationTime() < target_time && computer->IsEnabled()) {
        computer->Tick();
    }
}

void TubeComputerSimulator::RunSimulationForInstructions(TubeComputer* computer, int instruction_count) {
    if (!computer) return;
    
    int start_count = computer->GetInstructionCount();
    
    while ((computer->GetInstructionCount() - start_count) < instruction_count && computer->IsEnabled()) {
        computer->Tick();
    }
}

TubeComputerSimulator::SimulationStats TubeComputerSimulator::GetSimulationStats(const TubeComputer* computer) const {
    SimulationStats stats{};
    if (computer) {
        stats.total_instructions_executed = computer->GetInstructionCount();
        stats.total_operations_performed = computer->GetOperationCount();
        stats.total_simulated_time = computer->GetSimulationTime();
        stats.power_consumption = computer->GetPowerConsumption();
        stats.tube_count = computer->GetTubecount();
        
        // Calculate approximate ticks per second if time > 0
        if (stats.total_simulated_time > 0.0) {
            stats.average_tps = stats.total_operations_performed / stats.total_simulated_time;
        }
    }
    return stats;
}

bool TubeComputerSimulator::LoadProgramFromFile(TubeComputer* computer, const std::string& filename) {
    // Since this is a simulation, we'll implement a simple loader
    // In a real implementation, this would read a binary or assembly file
    if (!computer) return false;
    
    // For now, we'll just generate a simple test program
    std::vector<Instruction> test_program;
    
    // Create a simple addition program
    Instruction instr;
    instr.mnemonic = "LOAD";
    instr.opcode = 0x01;
    instr.operands = {0x0010};  // Load from address 0x0010
    test_program.push_back(instr);
    
    instr.mnemonic = "ADD";
    instr.opcode = 0x03;
    instr.operands = {0x0011};  // Add from address 0x0011
    test_program.push_back(instr);
    
    instr.mnemonic = "STORE";
    instr.opcode = 0x02;
    instr.operands = {0x0012};  // Store to address 0x0012
    test_program.push_back(instr);
    
    // Load the test program into the computer
    computer->LoadProgram(test_program, 0x0000);
    
    return true;
}

bool TubeComputerSimulator::SaveMemoryToFile(const TubeComputer* computer, const std::string& filename) const {
    // This is a simulation function - in a real implementation,
    // this would save the memory state to a file
    return true;
}

std::vector<ComputerArchitecture> TubeComputerSimulator::GetSupportedArchitectures() const {
    return {
        ComputerArchitecture::ENIAC,
        ComputerArchitecture::EDSAC,
        ComputerArchitecture::COLUSSUS,
        ComputerArchitecture::EDVAC,
        ComputerArchitecture::MANCHESTER,
        ComputerArchitecture::ILLIAC,
        ComputerArchitecture::WHIRLWIND,
        ComputerArchitecture::SAGE,
        ComputerArchitecture::IBM_701,
        ComputerArchitecture::CUSTOM
    };
}

void TubeComputerSimulator::RunSystemDiagnostics(TubeComputer* computer) {
    if (computer) {
        computer->RunDiagnostics();
    }
}

void TubeComputerSimulator::SimulateTubeAging(TubeComputer* computer, double years_to_simulate) {
    if (!computer) return;
    
    // Simulate aging effects on tubes over time
    // In a real system, this would affect tube performance, increase failure rates, etc.
    // For our simulation, we'll just increase the failure probability
    
    // Calculate how many times the simulation should run based on the years
    double total_seconds = years_to_simulate * 365.0 * 24.0 * 3600.0;  // Seconds in years
    double simulation_rate = 1000.0;  // Simulate at 1000x speed
    
    // Increase the component's failure rate based on the simulated aging
    double aging_factor = 1.0 + (years_to_simulate * 0.01);  // 1% increase per year
    // This is just a placeholder - in a real implementation we would apply the aging factor
}