#include "ProtoVM.h"
#include "MinimaxCADC.h"

/*
 * MinimaxCADC - Implementation of a minimal computer system using CADC architecture
 * 
 * This creates a complete computer system using the CADC chipset components:
 * - Three pipeline modules (Multiply, Divide, Special Logic)
 * - System Executive Control
 * - Timing and control logic
 * - Input/Output mechanisms
 * 
 * The system demonstrates air data computation capabilities of the F-14 CADC
 */

MinimaxCADC::MinimaxCADC() {
    // Initialize system components
    cadc_system = new CadcSystem();

    // Add system-level pins
    AddSink("PRESSURE_IN");
    AddSink("TEMP_IN");
    AddSink("ANGLE_OF_ATTACK");
    AddSink("START");
    AddSink("RESET");
    AddSource("BUSY");
    AddSource("VALID_OUTPUT");
    AddSource("ALTITUDE_OUT");
    AddSource("VERTICAL_SPEED_OUT");
    AddSource("AIR_SPEED_OUT");
    AddSource("MACH_NUMBER_OUT");
    AddSink("SYS_CLK");

    LOG("MinimaxCADC: Initialized with full CADC system");
}

MinimaxCADC::~MinimaxCADC() {
    delete cadc_system;
}

bool MinimaxCADC::Tick() {
    // Tick the CADC system
    cadc_system->Tick();

    return true;
}

bool MinimaxCADC::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    // Route process calls to the CADC system
    return cadc_system->Process(type, bytes, bits, conn_id, dest, dest_conn_id);
}

bool MinimaxCADC::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    // Route input to the CADC system
    return cadc_system->PutRaw(conn_id, data, data_bytes, data_bits);
}

// Function to set up the MiniMax CADC system
void SetupMiniMaxCADC(Machine& mach) {
    Pcb& pcb = mach.AddPcb();

    try {
        // Create the MiniMax CADC system
        MinimaxCADC& minimax_cadc = pcb.Add<MinimaxCADC>("MiniMaxCADC");

        LOG("MiniMaxCADC system configured with CADC chipset components");
    } catch (const std::exception& e) {
        LOG("Connection error in SetupMiniMaxCADC: " << String(e.what()));
    }
}

// Test function to demonstrate CADC polynomial evaluation
void TestCadcPolynomialEvaluation() {
    LOG("Testing CADC polynomial evaluation...");

    // Create a CADC system
    CadcSystem cadc;
    
    // Set up inputs (simplified)
    byte pressure_data[3] = {0x12, 0x34, 0x00};  // 20-bit pressure value
    byte temp_data[3] = {0x56, 0x78, 0x00};      // 20-bit temperature value
    byte angle_data[3] = {0x9A, 0xBC, 0x00};     // 20-bit angle value

    // Send sensor inputs to CADC
    cadc.PutRaw(CadcSystem::PRESSURE_IN, pressure_data, 3, 0);
    cadc.PutRaw(CadcSystem::TEMP_IN, temp_data, 3, 0);
    cadc.PutRaw(CadcSystem::ANGLE_OF_ATTACK, angle_data, 3, 0);

    // Start computation
    byte start_signal = 1;
    cadc.PutRaw(CadcSystem::START, &start_signal, 0, 1);

    // Perform several ticks to simulate the computation
    for (int i = 0; i < 100; i++) {
        cadc.Tick();
    }

    LOG("CADC polynomial evaluation test completed");
}

// Function to create a complete CADC system with polynomial evaluation
CadcSystem* CreateCadcWithPolynomialEvaluation() {
    CadcSystem* cadc = new CadcSystem();
    
    LOG("Created CADC system with polynomial evaluation capability");
    
    return cadc;
}