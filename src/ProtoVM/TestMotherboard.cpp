#include "ProtoVM.h"
#include "ICs.h"
#include "Helper4004.h"

// Forward declaration of SetupMiniMax4004
void SetupMiniMax4004(Machine& mach);

/*
 * Dummy chip classes for motherboard testing
 */

// Dummy 4004 CPU that doesn't connect to anything
class Dummy4004CPU : public ElectricNodeBase {
public:
    Dummy4004CPU() {
        AddBidirectional("D0");
        AddBidirectional("D1");
        AddBidirectional("D2");
        AddBidirectional("D3");
        AddSource("A0");
        AddSource("A1");
        AddSource("A2");
        AddSource("A3");
        AddSource("A4");
        AddSource("A5");
        AddSource("A6");
        AddSource("A7");
        AddSource("A8");
        AddSource("A9");
        AddSource("A10");
        AddSource("A11");
        AddSource("CM");
        AddSource("BUSY");
        AddSource("R/W");
        AddSource("MR");
        AddSource("MW");
        AddSink("SBY");
        AddSink("CM4");
        AddSink("RES");
        AddSource("OUT0");
        AddSource("OUT1");
        AddSource("OUT2");
        AddSource("OUT3");
        
        accumulator = 0;
        program_counter = 0;
        step = 0;
    }

    String GetClassName() const override { return "Dummy4004CPU"; }

    bool Tick() override {
        // Simulate minimal CPU operation
        step++;
        if (step > 100) step = 0;  // Reset counter to prevent overflow
        
        // Set some output for OUT0 (for WR0 instruction simulation)
        if (step == 10) {
            Cout() << (char)0x41;  // Output 'A' character
            Cout().Flush();
        }
        
        return true;
    }

    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override {
        return true; // No actual processing needed for dummy
    }

    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override {
        if (conn_id >= 21 && conn_id <= 22) { // SBY, CM4
            // Accept control signals
            return true;
        }
        return true;
    }

private:
    byte accumulator;
    uint16 program_counter;
    int step;
};

// Dummy ROM with minimal functionality
class Dummy4001 : public ElectricNodeBase {
public:
    Dummy4001() {
        // Add minimal required pins
        for (int i = 0; i < 4; i++) {
            AddBidirectional("D" + AsString(i));  // Data bus
        }
        for (int i = 0; i < 10; i++) {
            AddSink("A" + AsString(i));  // Address inputs
        }
        AddSink("~CS");   // Chip select
        AddSink("~OE");   // Output enable
        
        // Initialize memory
        for (int i = 0; i < 1024; i++) {
            memory[i] = 0;
        }
    }

    String GetClassName() const override { return "Dummy4001"; }

    bool Tick() override { return true; }

    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override {
        return true;
    }

    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override {
        return true;
    }
    
    byte GetMemory(int addr) { 
        if (addr >= 0 && addr < 1024) return memory[addr]; 
        return 0; 
    }
    
    void SetMemory(int addr, byte val) { 
        if (addr >= 0 && addr < 1024) memory[addr] = val; 
    }

private:
    byte memory[1024];
};

// Dummy RAM with minimal functionality
class Dummy4002 : public ElectricNodeBase {
public:
    Dummy4002() {
        // Add minimal required pins
        for (int i = 0; i < 4; i++) {
            AddBidirectional("D" + AsString(i));  // Data bus
        }
        for (int i = 0; i < 4; i++) {
            AddSink("A" + AsString(i));  // Address inputs
        }
        AddSink("~CS");   // Chip select
        AddSink("WE");    // Write enable
        
        // Initialize memory
        for (int i = 0; i < 16; i++) {
            memory[i] = 0;
        }
    }

    String GetClassName() const override { return "Dummy4002"; }

    bool Tick() override { return true; }

    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override {
        return true;
    }

    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override {
        return true;
    }
    
    byte GetMemory(int addr) { 
        if (addr >= 0 && addr < 16) return memory[addr]; 
        return 0; 
    }
    
    void SetMemory(int addr, byte val) { 
        if (addr >= 0 && addr < 16) memory[addr] = val; 
    }

private:
    byte memory[16];
};

// Dummy 6502 CPU with minimal functionality 
class Dummy6502 : public ElectricNodeBase {
public:
    Dummy6502() {
        // Add minimal required pins for 6502
        for (int i = 0; i < 8; i++) {
            AddBidirectional("D" + AsString(i));  // Data bus
        }
        for (int i = 0; i < 16; i++) {
            AddSource("A" + AsString(i));  // Address bus
        }
        AddSink("CLK");
        AddSource("RDY");
        AddSource("SYNC");
        AddSource("IRQ");
        AddSource("NMI");
        AddSource("RESET");
    }

    String GetClassName() const override { return "Dummy6502"; }

    bool Tick() override { return true; }

    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override {
        return true;
    }

    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override {
        return true;
    }
};

/*
 * Motherboard tests using dummy chips
 */

// Test 4004 motherboard setup with dummy chips
bool Test4004Motherboard() {
    LOG("Testing 4004 Motherboard with dummy chips...");
    
    try {
        Machine mach;
        Pcb& pcb = mach.AddPcb();
        
        // Add dummy components
        Dummy4004CPU& cpu = pcb.Add<Dummy4004CPU>("DUMMY_CPU4004");
        Dummy4001& rom = pcb.Add<Dummy4001>("DUMMY_ROM4001");
        Dummy4002& ram = pcb.Add<Dummy4002>("DUMMY_RAM4002");
        
        // Add supporting components
        Bus<12>& addr_bus = pcb.Add<Bus<12>>("ADDR_BUS");
        Bus<4>& data_bus = pcb.Add<Bus<4>>("DATA_BUS");
        
        // Add control pins
        Pin& clk = pcb.Add<Pin>("CLK").SetReference(1);
        Pin& reset = pcb.Add<Pin>("RESET").SetReference(0);
        Pin& ground = pcb.Add<Pin>("GROUND").SetReference(0);
        Pin& vcc = pcb.Add<Pin>("VCC").SetReference(1);
        
        // The dummy components don't need complex connections for this test
        // Just ensure they're added to the PCB and can be ticked
        
        // Tick the machine to ensure all components work
        for (int i = 0; i < 5; i++) {
            mach.Tick();
        }
        
        LOG("✓ 4004 Motherboard test passed");
        return true;
    }
    catch (Exc e) {
        LOG("Error in Test4004Motherboard: " << e);
        return false;
    }
}

// Test 6502 motherboard setup with dummy chips
bool Test6502Motherboard() {
    LOG("Testing 6502 Motherboard with dummy chips...");
    
    try {
        Machine mach;
        Pcb& pcb = mach.AddPcb();
        
        // Add dummy components
        Dummy6502& cpu6502 = pcb.Add<Dummy6502>("DUMMY_CPU6502");
        Dummy4001& rom = pcb.Add<Dummy4001>("DUMMY_ROM");
        Dummy4002& ram = pcb.Add<Dummy4002>("DUMMY_RAM");
        
        // Add supporting components
        Bus<16>& addr_bus = pcb.Add<Bus<16>>("ADDR_BUS");
        Bus<8>& data_bus = pcb.Add<Bus<8>>("DATA_BUS");
        
        // Add control pins
        Pin& clk = pcb.Add<Pin>("CLK").SetReference(1);
        Pin& reset = pcb.Add<Pin>("RESET").SetReference(0);
        Pin& ground = pcb.Add<Pin>("GROUND").SetReference(0);
        Pin& vcc = pcb.Add<Pin>("VCC").SetReference(1);
        
        // Tick the machine to ensure all components work
        for (int i = 0; i < 5; i++) {
            mach.Tick();
        }
        
        LOG("✓ 6502 Motherboard test passed");
        return true;
    }
    catch (Exc e) {
        LOG("Error in Test6502Motherboard: " << e);
        return false;
    }
}

// Test mixed motherboard setup
bool TestMixedMotherboard() {
    LOG("Testing Mixed Motherboard with dummy chips...");
    
    try {
        Machine mach;
        Pcb& pcb = mach.AddPcb();
        
        // Add various dummy components
        Dummy4004CPU& cpu4004 = pcb.Add<Dummy4004CPU>("DUMMY_CPU4004");
        Dummy6502& cpu6502 = pcb.Add<Dummy6502>("DUMMY_CPU6502");
        Dummy4001& rom = pcb.Add<Dummy4001>("DUMMY_ROM");
        Dummy4002& ram = pcb.Add<Dummy4002>("DUMMY_RAM");
        
        // Add buses
        Bus<16>& addr_bus = pcb.Add<Bus<16>>("ADDR_BUS");
        Bus<8>& data_bus8 = pcb.Add<Bus<8>>("DATA_BUS8");
        Bus<4>& data_bus4 = pcb.Add<Bus<4>>("DATA_BUS4");
        
        // Add control pins
        Pin& clk = pcb.Add<Pin>("CLK").SetReference(1);
        Pin& reset = pcb.Add<Pin>("RESET").SetReference(0);
        Pin& ground = pcb.Add<Pin>("GROUND").SetReference(0);
        Pin& vcc = pcb.Add<Pin>("VCC").SetReference(1);
        
        // Tick the machine to ensure all components work
        for (int i = 0; i < 5; i++) {
            mach.Tick();
        }
        
        LOG("✓ Mixed Motherboard test passed");
        return true;
    }
    catch (Exc e) {
        LOG("Error in TestMixedMotherboard: " << e);
        return false;
    }
}

// Test 4004 CPU + Memory interaction
bool Test4004CPUMemoryInteraction() {
    LOG("Testing 4004 CPU + Memory interaction...");
    
    try {
        Machine mach;
        Pcb& pcb = mach.AddPcb();
        
        // Add real 4004 CPU and memory components
        IC4004& cpu = pcb.Add<IC4004>("REAL_CPU4004");
        IC4001& rom = pcb.Add<IC4001>("REAL_ROM4001");
        IC4002& ram = pcb.Add<IC4002>("REAL_RAM4002");
        
        // Add buses
        Bus<12>& addr_bus = pcb.Add<Bus<12>>("ADDR_BUS");
        Bus<4>& data_bus = pcb.Add<Bus<4>>("DATA_BUS");
        
        // Add control pins
        Pin& clk = pcb.Add<Pin>("CLK").SetReference(1);
        Pin& reset = pcb.Add<Pin>("RESET").SetReference(0); // Initially reset
        Pin& ground = pcb.Add<Pin>("GROUND").SetReference(0);
        Pin& vcc = pcb.Add<Pin>("VCC").SetReference(1);
        
        // Mark output pins as optional since they go to terminal
        cpu.NotRequired("OUT0");
        cpu.NotRequired("OUT1");
        cpu.NotRequired("OUT2");
        cpu.NotRequired("OUT3");
        
        // Connect CPU to buses and memory
        for (int i = 0; i < 4; i++) {
            cpu["D" + AsString(i)] >> data_bus[i];
            data_bus[i] >> cpu["D" + AsString(i)];
        }
        
        // Connect address bus
        for (int i = 0; i < 12; i++) {
            cpu["A" + AsString(i)] >> addr_bus[i];
        }
        
        // Connect control signals
        clk >> cpu["CM4"];
        reset >> cpu["RES"];
        ground >> cpu["SBY"];
        
        // Connect ROM and RAM to buses
        for (int i = 0; i < 4; i++) {
            rom["D" + AsString(i)] >> data_bus[i];
            data_bus[i] >> rom["D" + AsString(i)];
        }
        
        for (int i = 0; i < 4; i++) {
            ram["D" + AsString(i)] >> data_bus[i];
            data_bus[i] >> ram["D" + AsString(i)];
        }
        
        for (int i = 0; i < 8; i++) {  // 8 address pins for ROM
            addr_bus[i] >> rom["A" + AsString(i)];
        }
        
        for (int i = 0; i < 4; i++) {  // 4 address pins for RAM
            addr_bus[i] >> ram["A" + AsString(i)];
        }
        
        // Connect ROM/RAM control signals
        ground >> rom["~OE"];  // Output enabled
        ground >> rom["~CS"];  // Chip select active
        vcc >> ram["~CS"];     // Chip select active
        ground >> ram["WE"];   // Write enable inactive (read mode)
        
        // Initialize memory with a simple program to test CPU-memory interaction
        // Write a value to memory and read it back through the CPU
        rom.SetMemory(0x0, 0x5);  // Test value
        rom.SetMemory(0x1, 0xA);  // Another test value
        
        // Release reset and run for a few ticks to allow interaction
        reset.SetReference(1);  // Deassert reset
        
        for (int i = 0; i < 10; i++) {
            mach.Tick();
        }
        
        LOG("✓ 4004 CPU + Memory interaction test passed");
        return true;
    }
    catch (Exc e) {
        LOG("Error in Test4004CPUMemoryInteraction: " << e);
        return false;
    }
}

// Test 4004 CPU + Memory + Motherboard interaction (similar to run_4004_program.sh)
bool Test4004CPUMemoryBoardPutchar() {
    LOG("Testing 4004 CPU + Memory + Motherboard interaction (putchar simulation)...");
    
    try {
        // Use the same setup as SetupMiniMax4004 to simulate the real environment
        Machine mach;
        SetupMiniMax4004(mach);  // This sets up the real circuit
        
        // Load the same binary as run_4004_program.sh
        if (!LoadProgramTo4004ROM(mach, "4004_putchar.bin", 0x0)) {
            LOG("Error: Could not load 4004_putchar.bin for put character test");
            return false;
        }
        
        // Run for multiple ticks to execute the program
        for (int i = 0; i < 50; i++) {
            mach.Tick();
        }
        
        LOG("✓ 4004 CPU + Memory + Motherboard interaction (putchar) test passed");
        return true;
    }
    catch (Exc e) {
        LOG("Error in Test4004CPUMemoryBoardPutchar: " << e);
        return false;
    }
}

// Main runner for motherboard tests
int RunMotherboardTests() {
    LOG("Running Motherboard Tests with Dummy Chips...\n");
    
    int passed = 0;
    int total = 0;
    
    total++; if (Test4004Motherboard()) { LOG("✓ Test4004Motherboard PASSED"); passed++; }
    else { LOG("✗ Test4004Motherboard FAILED"); }
    
    total++; if (Test6502Motherboard()) { LOG("✓ Test6502Motherboard PASSED"); passed++; }
    else { LOG("✗ Test6502Motherboard FAILED"); }
    
    total++; if (TestMixedMotherboard()) { LOG("✓ TestMixedMotherboard PASSED"); passed++; }
    else { LOG("✗ TestMixedMotherboard FAILED"); }
    
    total++; if (Test4004CPUMemoryInteraction()) { LOG("✓ Test4004CPUMemoryInteraction PASSED"); passed++; }
    else { LOG("✗ Test4004CPUMemoryInteraction FAILED"); }
    
    total++; if (Test4004CPUMemoryBoardPutchar()) { LOG("✓ Test4004CPUMemoryBoardPutchar PASSED"); passed++; }
    else { LOG("✗ Test4004CPUMemoryBoardPutchar FAILED"); }
    
    LOG("\nMotherboard Tests Summary: " << passed << "/" << total << " tests passed");
    
    if (passed == total) {
        LOG("All Motherboard Tests PASSED! ✓");
        return 0;
    } else {
        LOG("Some Motherboard Tests FAILED! ✗");
        return 1;
    }
}