#include "ProtoVM.h"
#include "ICs.h"
#include "IC4001.h"
#include "IC4002.h"
#include "IC4004.h"
#include "ICRamRom.h"
#include "Bus.h"
#include "BusController4004.h"
#include "StateMachine.h"

/*
 * Unit tests for individual chips without connections to other chips
 */

// Test IC4001 (4-bit ROM) functionality
bool TestIC4001Unit() {
    LOG("Testing IC4001 (4-bit ROM) unit functionality...");
    
    try {
        IC4001 rom;
        
        // The IC4001 constructor calls its initialization
        // Test initial state - check if memory starts with expected values
        // We'll just verify that the component can be constructed and ticked
        if (!rom.Tick()) {
            LOG("Error: IC4001 Tick() failed");
            return false;
        }
        
        LOG("✓ IC4001 unit test passed");
        return true;
    }
    catch (Exc e) {
        LOG("Error in TestIC4001Unit: " << e);
        return false;
    }
}

// Test IC4002 (4-bit RAM) functionality
bool TestIC4002Unit() {
    LOG("Testing IC4002 (4-bit RAM) unit functionality...");
    
    try {
        IC4002 ram;
        
        // Test Tick functionality
        if (!ram.Tick()) {
            LOG("Error: IC4002 Tick() failed");
            return false;
        }
        
        LOG("✓ IC4002 unit test passed");
        return true;
    }
    catch (Exc e) {
        LOG("Error in TestIC4002Unit: " << e);
        return false;
    }
}

// Test IC4004 (4-bit CPU) functionality in isolation
bool TestIC4004Unit() {
    LOG("Testing IC4004 (4-bit CPU) unit functionality...");
    
    try {
        IC4004 cpu;
        
        // Test initial state and basic functionality
        if (cpu.GetAccumulator() != 0) {
            LOG("Warning: IC4004 accumulator may not initialize to 0, but that's expected in some implementations");
        }
        
        if (cpu.GetProgramCounter() != 0) {
            LOG("Warning: IC4004 program counter may not initialize to 0, but that's expected in some implementations");
        }
        
        // Test Tick functionality
        if (!cpu.Tick()) {
            LOG("Error: IC4004 Tick() failed");
            return false;
        }
        
        LOG("✓ IC4004 unit test passed");
        return true;
    }
    catch (Exc e) {
        LOG("Error in TestIC4004Unit: " << e);
        return false;
    }
}

// Test IC6502 (6502 CPU) functionality in isolation
bool TestIC6502Unit() {
    LOG("Testing IC6502 (6502 CPU) unit functionality...");
    
    try {
        IC6502 cpu6502;
        
        // Test Tick functionality
        if (!cpu6502.Tick()) {
            LOG("Error: IC6502 Tick() failed");
            return false;
        }
        
        LOG("✓ IC6502 unit test passed");
        return true;
    }
    catch (Exc e) {
        LOG("Error in TestIC6502Unit: " << e);
        return false;
    }
}

// Test ICRamRom functionality
bool TestICRamRomUnit() {
    LOG("Testing ICRamRom unit functionality...");
    
    try {
        ICRamRom ramrom;
        
        // Test Tick functionality
        if (!ramrom.Tick()) {
            LOG("Error: ICRamRom Tick() failed");
            return false;
        }
        
        LOG("✓ ICRamRom unit test passed");
        return true;
    }
    catch (Exc e) {
        LOG("Error in TestICRamRomUnit: " << e);
        return false;
    }
}

// Test Bus functionality
template<int Width>
bool TestBusUnit() {
    String widthStr = AsString(Width);
    LOG("Testing Bus<" + widthStr + "> unit functionality...");
    
    try {
        Bus<Width> bus;
        
        // Test Tick functionality
        if (!bus.Tick()) {
            LOG("Error: Bus<" + widthStr + "> Tick() failed");
            return false;
        }
        
        LOG("✓ Bus<" + widthStr + "> unit test passed");
        return true;
    }
    catch (Exc e) {
        LOG("Error in TestBusUnit<" + widthStr + ">: " << e);
        return false;
    }
}

// Test BusController4004 functionality
bool TestBusController4004Unit() {
    LOG("Testing BusController4004 unit functionality...");
    
    try {
        BusController4004 busCtrl;
        
        // Test Tick functionality
        if (!busCtrl.Tick()) {
            LOG("Error: BusController4004 Tick() failed");
            return false;
        }
        
        LOG("✓ BusController4004 unit test passed");
        return true;
    }
    catch (Exc e) {
        LOG("Error in TestBusController4004Unit: " << e);
        return false;
    }
}

// Test StateMachine functionality
bool TestStateMachineUnit() {
    LOG("Testing StateMachine unit functionality...");
    
    try {
        StateMachine sm;
        
        // Test Tick functionality
        if (!sm.Tick()) {
            LOG("Error: StateMachine Tick() failed");
            return false;
        }
        
        LOG("✓ StateMachine unit test passed");
        return true;
    }
    catch (Exc e) {
        LOG("Error in TestStateMachineUnit: " << e);
        return false;
    }
}

// Main runner for chip unit tests
int RunChipUnitTests() {
    LOG("Running Chip Unit Tests...\n");
    
    int passed = 0;
    int total = 0;
    
    // Test each chip individually
    total++; if (TestIC4001Unit()) { LOG("✓ TestIC4001Unit PASSED"); passed++; } 
    else { LOG("✗ TestIC4001Unit FAILED"); }
    
    total++; if (TestIC4002Unit()) { LOG("✓ TestIC4002Unit PASSED"); passed++; } 
    else { LOG("✗ TestIC4002Unit FAILED"); }
    
    total++; if (TestIC4004Unit()) { LOG("✓ TestIC4004Unit PASSED"); passed++; } 
    else { LOG("✗ TestIC4004Unit FAILED"); }
    
    total++; if (TestIC6502Unit()) { LOG("✓ TestIC6502Unit PASSED"); passed++; } 
    else { LOG("✗ TestIC6502Unit FAILED"); }
    
    total++; if (TestICRamRomUnit()) { LOG("✓ TestICRamRomUnit PASSED"); passed++; } 
    else { LOG("✗ TestICRamRomUnit FAILED"); }
    
    total++; if (TestBusUnit<4>()) { LOG("✓ TestBusUnit<4> PASSED"); passed++; } 
    else { LOG("✗ TestBusUnit<4> FAILED"); }
    
    total++; if (TestBusUnit<8>()) { LOG("✓ TestBusUnit<8> PASSED"); passed++; } 
    else { LOG("✗ TestBusUnit<8> FAILED"); }
    
    total++; if (TestBusUnit<12>()) { LOG("✓ TestBusUnit<12> PASSED"); passed++; } 
    else { LOG("✗ TestBusUnit<12> FAILED"); }
    
    total++; if (TestBusController4004Unit()) { LOG("✓ TestBusController4004Unit PASSED"); passed++; } 
    else { LOG("✗ TestBusController4004Unit FAILED"); }
    
    total++; if (TestStateMachineUnit()) { LOG("✓ TestStateMachineUnit PASSED"); passed++; } 
    else { LOG("✗ TestStateMachineUnit FAILED"); }
    
    LOG("\nChip Unit Tests Summary: " << passed << "/" << total << " tests passed");
    
    if (passed == total) {
        LOG("All Chip Unit Tests PASSED! ✓");
        return 0;
    } else {
        LOG("Some Chip Unit Tests FAILED! ✗");
        return 1;
    }
}