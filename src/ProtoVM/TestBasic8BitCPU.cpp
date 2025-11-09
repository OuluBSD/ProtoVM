#include "ProtoVM.h"
#include "Basic8BitCPU.h"

#include <Core/Core.h>
using namespace UPP;

// Test for the Basic8BitCPU component
void TestBasic8BitCPU() {
    LOG("Starting Basic8BitCPU Test...");
    
    // Create the CPU
    Basic8BitCPU cpu;
    cpu.SetName("TestBasic8BitCPU");

    // Initialize CPU state
    LOG("Initial state:");
    LOG("  A = 0x" << HexStr(cpu.GetAccumulator()));
    LOG("  X = 0x" << HexStr(cpu.GetXRegister()));
    LOG("  Y = 0x" << HexStr(cpu.GetYRegister()));
    LOG("  PC = 0x" << HexStr(cpu.GetProgramCounter()));
    LOG("  S = 0x" << HexStr(cpu.GetStackPointer()));

    // Simulate a simple program execution
    // For this test, we'll manually set up a simple sequence:
    // LDA_IMM 0x42 (load 0x42 into accumulator)
    // LDX_IMM 0x10 (load 0x10 into X register)
    // INX (increment X register)
    
    // The actual implementation would fetch these from memory
    // For our test, we'll simulate the effect by modifying the internal state
    // This is a simplified test to demonstrate the CPU's functionality
    
    LOG("Running test program simulation...");
    
    // First instruction: LDA_IMM 0x42
    cpu.PutRaw(24, (byte*)0, 0, 1);  // Simulate clock tick
    cpu.Tick();
    
    LOG("After LDA_IMM simulation:");
    LOG("  A = 0x" << HexStr(cpu.GetAccumulator()));
    LOG("  X = 0x" << HexStr(cpu.GetXRegister()));
    LOG("  Y = 0x" << HexStr(cpu.GetYRegister()));

    // Second instruction: LDX_IMM 0x10
    cpu.Tick();
    LOG("After LDX_IMM simulation:");
    LOG("  A = 0x" << HexStr(cpu.GetAccumulator()));
    LOG("  X = 0x" << HexStr(cpu.GetXRegister()));
    LOG("  Y = 0x" << HexStr(cpu.GetYRegister()));

    // Third instruction: INX
    cpu.Tick();
    LOG("After INX simulation:");
    LOG("  A = 0x" << HexStr(cpu.GetAccumulator()));
    LOG("  X = 0x" << HexStr(cpu.GetXRegister()));
    LOG("  Y = 0x" << HexStr(cpu.GetYRegister()));

    LOG("Basic8BitCPU test completed.");
}

// Entry point for the test
void Test70_Basic8BitCPU() {
    TestBasic8BitCPU();
    LOG("Basic 8-bit CPU test completed.");
}