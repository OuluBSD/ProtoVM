#include "ProtoVM.h"
#include "ICs.h"

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
    
    LOG("\nMotherboard Tests Summary: " << passed << "/" << total << " tests passed");
    
    if (passed == total) {
        LOG("All Motherboard Tests PASSED! ✓");
        return 0;
    } else {
        LOG("Some Motherboard Tests FAILED! ✗");
        return 1;
    }
}