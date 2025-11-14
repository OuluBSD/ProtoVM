#ifndef ProtoVM_CadcSystem_h
#define ProtoVM_CadcSystem_h

#include "Component.h"
#include "ICs.h"
#include "Common.h"
#include "Bus.h"
#include "ICcadc.h"

/*
 * F-14 CADC System Implementation
 *
 * This implements the complete CADC system with:
 * - Three pipeline modules (Multiply, Divide, Special Logic)
 * - System Executive Control
 * - Interconnection between modules
 * - Timing coordination
 * - Polynomial evaluation algorithms
 * - Air data computation algorithms
 */

class CadcSystem : public ElectricNode {
public:
    CadcSystem();
    virtual ~CadcSystem();

    bool Tick() override;
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;

    String GetClassName() const override { return "CadcSystem"; }

    // Get references to system components for testing
    ICcadcModule* GetMultiplyModule() { return mul_module; }
    ICcadcModule* GetDivideModule() { return div_module; }
    ICcadcModule* GetSpecialLogicModule() { return slf_module; }
    ICcadcBase* GetSystemExecutiveControl() { return sys_exec_ctrl; }

    // Pin mappings
    enum PinNames {
        // Input sensors (simplified for this implementation)
        PRESSURE_IN = 0,
        TEMP_IN = 1,
        ANGLE_OF_ATTACK = 2,

        // Control inputs
        START = 3,
        RESET = 4,

        // Status outputs
        BUSY = 5,
        VALID_OUTPUT = 6,

        // Output data (simplified)
        ALTITUDE_OUT = 7,
        VERTICAL_SPEED_OUT = 8,
        AIR_SPEED_OUT = 9,
        MACH_NUMBER_OUT = 10,

        // System clock
        SYS_CLK = 11
    };

    // Timing variables - made public for testing and CLI access
    int system_cycle;              // Current system cycle
    int bit_time;                  // Current bit in word (0-19)
    int word_time;                 // Current word time (0-1) - W0 or W1
    int operation_time;            // Current operation time
    bool frame_mark;               // Frame marker for computation cycle
    bool word_mark;                // Word marker (T18 of every word)
    bool is_running;
    bool is_busy;

    // Air data computation state
    int20 prev_altitude;           // Previous altitude for rate computation
    int20 temperature_correction;  // Temperature correction factor

    // Polynomial evaluation helpers - made public for testing and CLI access
    int20 EvaluatePolynomial(int20 x, const int20* coefficients, int degree);
    int20 ComputeAltitude(int20 pressure_altitude, int20 temperature);
    int20 ComputeVerticalSpeed(int20 altitude_old, int20 altitude_new);
    int20 ComputeAirSpeed(int20 impact_pressure, int20 static_pressure);
    int20 ComputeMachNumber(int20 air_speed, int20 temperature);

private:
    // System components
    ICcadcModule* mul_module;      // Multiply module
    ICcadcModule* div_module;      // Divide module
    ICcadcModule* slf_module;      // Special Logic Function module
    ICcadcBase* sys_exec_ctrl;     // System Executive Control (placeholder - not implemented as concrete class)

    void UpdateSystemTiming();
    void HandleModuleCommunication();
    void ExecuteAirDataComputations();
    void UpdateControlSignals();
};

#endif