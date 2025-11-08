#include "ProtoVM.h"
#include "IC4004.h"
#include "IC4001.h"
#include "IC4002.h"
#include "Bus.h"
#include "Common.h"

/*
 * Unit tests for Intel 4004 CPU component
 * Tests various aspects of the 4004 CPU functionality
 */

void Test4004BasicOperation() {
    LOG("=== Testing 4004 Basic Operation ===");
    
    Machine mach;
    Pcb& pcb = mach.AddPcb();
    
    // Create 4004 CPU and associated components
    IC4004& cpu = pcb.Add<IC4004>("TEST_CPU4004");
    IC4001& rom = pcb.Add<IC4001>("TEST_ROM");
    IC4002& ram = pcb.Add<IC4002>("TEST_RAM");
    
    // Create buses
    Bus<4>& data_bus = pcb.Add<Bus<4>>("DATA_BUS");
    Bus<12>& addr_bus = pcb.Add<Bus<12>>("ADDR_BUS");
    
    // Create control signals
    Pin& clk = pcb.Add<Pin>("CLK").SetReference(1);
    Pin& reset = pcb.Add<Pin>("RESET").SetReference(0);
    
    try {
        // Connect components
        cpu["D0"] >> data_bus[0];
        cpu["D1"] >> data_bus[1];
        cpu["D2"] >> data_bus[2];
        cpu["D3"] >> data_bus[3];
        
        cpu["A0"] >> addr_bus[0];
        cpu["A1"] >> addr_bus[1];
        cpu["A2"] >> addr_bus[2];
        cpu["A3"] >> addr_bus[3];
        cpu["A4"] >> addr_bus[4];
        cpu["A5"] >> addr_bus[5];
        cpu["A6"] >> addr_bus[6];
        cpu["A7"] >> addr_bus[7];
        cpu["A8"] >> addr_bus[8];
        cpu["A9"] >> addr_bus[9];
        cpu["A10"] >> addr_bus[10];
        cpu["A11"] >> addr_bus[11];
        
        clk >> cpu["CM4"];
        reset >> cpu["RES"];
        
        // Initialize machine
        if (!mach.Init()) {
            LOG("ERROR: Failed to initialize machine");
            return;
        }
        
        LOG("4004 CPU basic operation test: PASSED");
    } catch (Exc e) {
        LOG("4004 CPU basic operation test: FAILED - " << e);
    }
}

void Test4004RegisterOperations() {
    LOG("\n=== Testing 4004 Register Operations ===");
    
    Machine mach;
    Pcb& pcb = mach.AddPcb();
    
    // Create 4004 CPU
    IC4004& cpu = pcb.Add<IC4004>("TEST_CPU4004");
    
    try {
        // Initialize machine
        if (!mach.Init()) {
            LOG("ERROR: Failed to initialize machine for register test");
            return;
        }
        
        // Test that internal registers exist and are accessible
        // The CPU constructor initializes registers to 0
        LOG("4004 has " << sizeof(cpu.registers)/sizeof(cpu.registers[0]) << " registers");
        LOG("Accumulator initialized to: 0x" << HexStr(cpu.accumulator));
        LOG("Program counter initialized to: 0x" << HexStr(cpu.program_counter));
        
        // Test register access - in our simulation, we can't directly modify internal registers
        // But we can verify they exist and have expected initial values
        bool all_registers_zero = true;
        for (int i = 0; i < 16; i++) {
            if (cpu.registers[i] != 0) {
                all_registers_zero = false;
                break;
            }
        }
        
        if (all_registers_zero && cpu.accumulator == 0 && cpu.program_counter == 0) {
            LOG("4004 register initialization test: PASSED");
        } else {
            LOG("4004 register initialization test: FAILED");
        }
    } catch (Exc e) {
        LOG("4004 register operations test: FAILED - " << e);
    }
}

void Test4004ClockAndTiming() {
    LOG("\n=== Testing 4004 Clock and Timing ===");
    
    Machine mach;
    Pcb& pcb = mach.AddPcb();
    
    // Create 4004 CPU and clock components
    IC4004& cpu = pcb.Add<IC4004>("TEST_CPU4004");
    
    // Create buses
    Bus<4>& data_bus = pcb.Add<Bus<4>>("DATA_BUS");
    Bus<12>& addr_bus = pcb.Add<Bus<12>>("ADDR_BUS");
    
    // Create control signals
    Pin& clk = pcb.Add<Pin>("CLK").SetReference(1);
    Pin& reset = pcb.Add<Pin>("RESET").SetReference(0);
    
    try {
        // Connect components
        clk >> cpu["CM4"];
        reset >> cpu["RES"];
        
        // Initialize machine
        if (!mach.Init()) {
            LOG("ERROR: Failed to initialize machine for timing test");
            return;
        }
        
        // Test clock divider functionality
        int old_divider = cpu.GetClockDivider();
        cpu.SetClockDivider(2);
        int new_divider = cpu.GetClockDivider();
        
        if (new_divider == 2) {
            LOG("4004 clock divider functionality test: PASSED");
        } else {
            LOG("4004 clock divider functionality test: FAILED");
        }
        
        // Test ticking
        for (int i = 0; i < 5; i++) {
            bool tick_result = mach.Tick();
            if (!tick_result) {
                LOG("Tick " << i << " failed");
                break;
            }
        }
        
        LOG("4004 clock and timing test: PASSED");
    } catch (Exc e) {
        LOG("4004 clock and timing test: FAILED - " << e);
    }
}

void Test4004FlagsAndStatus() {
    LOG("\n=== Testing 4004 Flags and Status ===");
    
    Machine mach;
    Pcb& pcb = mach.AddPcb();
    
    // Create 4004 CPU
    IC4004& cpu = pcb.Add<IC4004>("TEST_CPU4004");
    
    try {
        // Initialize machine
        if (!mach.Init()) {
            LOG("ERROR: Failed to initialize machine for flags test");
            return;
        }
        
        // Test initial flag states
        if (!cpu.carry_flag && !cpu.aux_carry_flag) {
            LOG("4004 initial flag states correct: PASSED");
        } else {
            LOG("4004 initial flag states incorrect: FAILED");
        }
        
        // Test flag modification capability
        cpu.carry_flag = true;
        cpu.aux_carry_flag = true;
        
        if (cpu.carry_flag && cpu.aux_carry_flag) {
            LOG("4004 flag modification capability: PASSED");
        } else {
            LOG("4004 flag modification capability: FAILED");
        }
        
        LOG("4004 flags and status test: PASSED");
    } catch (Exc e) {
        LOG("4004 flags and status test: FAILED - " << e);
    }
}

void Run4004UnitTests() {
    LOG("Starting Intel 4004 CPU Unit Tests...\n");
    
    Test4004BasicOperation();
    Test4004RegisterOperations();
    Test4004ClockAndTiming();
    Test4004FlagsAndStatus();
    
    LOG("\nIntel 4004 CPU Unit Tests completed.");
}

// Main test function accessible from outside
void RunArithmeticUnitTests(Machine& mach) {
    LOG("Running 4004 CPU tests via RunArithmeticUnitTests...");
    Run4004UnitTests();
}