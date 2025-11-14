#ifndef ProtoVM_MinimaxCADC_h
#define ProtoVM_MinimaxCADC_h

#include "Component.h"
#include "CadcSystem.h"

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

class MinimaxCADC : public ElectricNode {
public:
    MinimaxCADC();
    virtual ~MinimaxCADC();

    bool Tick() override;
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;

    String GetClassName() const override { return "MinimaxCADC"; }

private:
    CadcSystem* cadc_system;

public:
    // Pin mappings
    enum PinNames {
        // Input sensors
        PRESSURE_IN = 0,
        TEMP_IN = 1,
        ANGLE_OF_ATTACK = 2,

        // Control inputs
        START = 3,
        RESET = 4,

        // Status outputs
        BUSY = 5,
        VALID_OUTPUT = 6,

        // Output data
        ALTITUDE_OUT = 7,
        VERTICAL_SPEED_OUT = 8,
        AIR_SPEED_OUT = 9,
        MACH_NUMBER_OUT = 10,

        // System clock
        SYS_CLK = 11
    };
};

// Function to set up the MiniMax CADC system
void SetupMiniMaxCADC(Machine& mach);

// Test function to demonstrate CADC polynomial evaluation
void TestCadcPolynomialEvaluation();

// Function to create a complete CADC system with polynomial evaluation
CadcSystem* CreateCadcWithPolynomialEvaluation();

#endif