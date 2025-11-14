#include "ProtoVM.h"
#include "CadcSystem.h"

/*
 * F-14 CADC System Implementation
 * Implements the complete CADC system with interconnection and timing
 */

// Implementation of polynomial evaluation functions

int20 CadcSystem::EvaluatePolynomial(int20 x, const int20* coefficients, int degree) {
    // Horner's method for evaluating polynomials: a_n*x^n + a_(n-1)*x^(n-1) + ... + a_1*x + a_0
    // The computation is: ((a_n*x + a_(n-1))*x + a_(n-2))*x + ... + a_1)*x + a_0
    if (degree < 0) return 0;

    int20 result = coefficients[degree]; // Start with the highest degree coefficient

    for (int i = degree - 1; i >= 0; i--) {
        // In CADC, we need to handle fixed-point arithmetic and scaling
        // For simplicity, we'll use integer arithmetic with appropriate scaling
        result = (result * x) / (1 << 10) + coefficients[i];  // Scale by 2^10 to prevent overflow
    }

    return result;
}

int20 CadcSystem::ComputeAltitude(int20 pressure_altitude, int20 temperature) {
    // Simplified altitude computation using standard atmosphere model
    // In real CADC, this would involve complex polynomial evaluations
    // This is a simplified approximation

    // Standard sea level pressure (PSI) in scaled integer form
    const int20 P0 = 0x40000;  // Approximately 14.696 PSI scaled

    // Pressure ratio
    if (pressure_altitude <= 0) return 0;  // Safety check

    // Simplified calculation: altitude = -29.92 * ln(PS / P0) where PS is static pressure
    // For our simulation, use a polynomial approximation
    int20 altitude = pressure_altitude; // Simplified for now

    // Apply temperature correction if available
    if (temperature > 0) {
        altitude = (altitude * temperature) / 0x20000;  // Temperature correction factor
    }

    return altitude;
}

int20 CadcSystem::ComputeVerticalSpeed(int20 altitude_old, int20 altitude_new) {
    // Compute rate of altitude change (feet per second)
    // In real CADC, this uses historical altitude values stored in RAS
    int20 delta_alt = altitude_new - altitude_old;

    // Assuming a time interval based on system clock (375kHz)
    // For 10ms computation cycle: divide by appropriate scaling factor
    // Simplified implementation for simulation
    return delta_alt / 4;  // Simplified scaling
}

int20 CadcSystem::ComputeAirSpeed(int20 impact_pressure, int20 static_pressure) {
    // Compute indicated airspeed using impact and static pressure
    // IAS = sqrt(295.4 * (q_c + P_s) * (pow((q_c/P_s + 1), 2/7) - 1))
    // Where q_c = impact pressure - static pressure, P_s = static pressure
    // Simplified implementation using polynomial approximation

    if (static_pressure <= 0) return 0;  // Safety check

    int20 dynamic_pressure = impact_pressure - static_pressure;

    if (dynamic_pressure <= 0) return 0;  // No dynamic pressure, no airspeed

    // Simplified polynomial approximation for airspeed
    // In real CADC, this involves complex polynomial evaluation using PMU and PDU
    int20 airspeed = dynamic_pressure / 2;  // Simplified

    return airspeed;
}

int20 CadcSystem::ComputeMachNumber(int20 air_speed, int20 temperature) {
    // Compute Mach number: M = TAS / speed of sound
    // Where speed of sound = sqrt(gamma * R * T), gamma = 1.4 for air
    // Simplified implementation using polynomial approximation

    if (temperature <= 0) return 0;  // Safety check

    // For our simulation, use a simplified polynomial
    // Mach = a * V + b * T + c (where V is airspeed, T is temperature)
    int20 mach = (air_speed * 0x1000) / (temperature + 0x8000);  // Simplified scaling

    return mach;
}

CadcSystem::CadcSystem() {
    // Initialize system components
    mul_module = new ICcadcModule();  // Multiply module with PMU
    div_module = new ICcadcModule();  // Divide module with PDU
    slf_module = new ICcadcModule();  // Special Logic module with SLF
    // For now, we'll make sys_exec_ctrl a separate component or remove it
    // since ICcadcBase is abstract. For a real implementation we'd need a concrete class

    // Initialize timing variables
    system_cycle = 0;
    bit_time = 0;
    word_time = 0;
    operation_time = 0;
    frame_mark = false;
    word_mark = false;
    is_running = false;
    is_busy = false;
    prev_altitude = 0;
    temperature_correction = 0;

    // Add system pins
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

    // Initialize additional state variables
    prev_altitude = 0;
    temperature_correction = 0;

    LOG("CadcSystem: Initialized with 3 pipeline modules");
}

CadcSystem::~CadcSystem() {
    // Clean up components
    delete mul_module;
    delete div_module;
    delete slf_module;
    // sys_exec_ctrl is not used since it would require a concrete implementation of ICcadcBase
}

bool CadcSystem::Tick() {
    // Update system timing
    UpdateSystemTiming();

    // Update all modules
    mul_module->Tick();
    div_module->Tick();
    slf_module->Tick();
    // sys_exec_ctrl tick is not called since it's not implemented as a concrete class

    // Handle module communication and data exchange
    HandleModuleCommunication();

    // Execute air data computations
    ExecuteAirDataComputations();

    // Update control signals
    UpdateControlSignals();

    return true;
}

bool CadcSystem::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    byte temp_data[3];

    if (type == WRITE) {
        // Handle output based on connection ID
        switch (conn_id) {
            case BUSY:
                temp_data[0] = is_busy ? 1 : 0;
                return dest.PutRaw(dest_conn_id, temp_data, 0, 1);

            case VALID_OUTPUT:
                temp_data[0] = frame_mark ? 1 : 0;  // Valid when frame mark is set
                return dest.PutRaw(dest_conn_id, temp_data, 0, 1);

            case ALTITUDE_OUT:
            case VERTICAL_SPEED_OUT:
            case AIR_SPEED_OUT:
            case MACH_NUMBER_OUT:
                // For this demo, output simple values
                temp_data[0] = 0x42;  // Example value
                temp_data[1] = 0x01;
                temp_data[2] = 0x00;
                return dest.PutRaw(dest_conn_id, temp_data, bytes, bits);

            default:
                return true;
        }
    }

    return true;
}

bool CadcSystem::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    switch (conn_id) {
        case START:
            if (data_bits == 1 && *data == 1) {
                is_running = true;
            }
            break;

        case RESET:
            if (data_bits == 1 && *data == 1) {
                system_cycle = 0;
                bit_time = 0;
                word_time = 0;
                operation_time = 0;
                is_running = false;
                is_busy = false;
            }
            break;

        case PRESSURE_IN:
        case TEMP_IN:
        case ANGLE_OF_ATTACK:
            // Input sensor data - would be processed by the system
            // For now, just store in a buffer for the computations
            break;

        case SYS_CLK:
            // System clock input - drives the timing
            break;

        default:
            break;
    }

    return true;
}

void CadcSystem::UpdateSystemTiming() {
    // Advance timing based on system clock
    // In real CADC: 375 kHz clock = 2.66 μs per bit time
    bit_time = (bit_time + 1) % CADC_WORD_LENGTH;  // 0-19

    if (bit_time == 0) {
        // Completed a word time
        word_time = (word_time + 1) % 2;  // Alternates W0 and W1
        system_cycle++;

        // Word mark is generated at T18 of every word
        if (bit_time == 18) {
            word_mark = true;
        } else {
            word_mark = false;
        }
    }

    // Operation time: Two consecutive word times make one operation time
    if (word_time == 0) {
        operation_time = system_cycle / 2;
    }

    // Frame mark: Generated by system executive control at end of computation cycle
    // For this simplified model, set frame mark periodically
    if (system_cycle % 16 == 15) {  // Example: every 16 system cycles
        frame_mark = true;
    } else {
        frame_mark = false;
    }
}

void CadcSystem::HandleModuleCommunication() {
    // Handle data exchange between modules
    // In real CADC, modules communicate through steering units and shared buses

    // Example: Route data from RAS in one module to input of another
    // This would involve more complex interconnect logic in a real implementation

    // For this simulation, we'll just simulate data exchange between modules
    // based on the operation being performed

    // In W0 (instruction fetch), modules receive microcode from their ROMs
    // In W1 (data transfer), modules exchange data and perform operations
    if (word_time == 0) {
        // W0: Instruction fetch phase
        // Modules get their next instruction from ROM
    } else {
        // W1: Data transfer phase
        // Modules process data and possibly exchange with other modules
    }
}

void CadcSystem::ExecuteAirDataComputations() {
    // Execute the core air data computations
    // This simulates the polynomial evaluations, data limiting, etc. that the CADC does

    // The CADC computes:
    // - Altitude
    // - Vertical Speed
    // - Air Speed
    // - Mach Number
    // from inputs like:
    // - Static and dynamic pressure
    // - Temperature
    // - Angle of attack

    if (is_running) {
        // Simulate computation progress
        is_busy = true;

        // When a computation cycle completes (frame mark), set outputs
        if (frame_mark) {
            is_busy = false;

            // In real CADC, polynomial evaluations would be performed using:
            // - PMU for multiplication operations
            // - PDU for division operations
            // - SLF for data limiting and logical operations
            // - RAS for temporary storage
            // - ROM for microcode storage
            
            // For this simulation, we'll use our helper functions that simulate
            // the polynomial evaluation and air data computation
            
            // Get inputs from internal state (in a real implementation these would come from sensor inputs)
            // For simulation, we'll use representative values from the PutRaw method
            static int20 static_pressure = 0x20000;  // Default pressure value
            static int20 temperature = 0x18000;      // Default temperature value
            static int20 impact_pressure = 0x21000;  // Default impact pressure
            
            // Compute air data parameters using CADC algorithm equivalents
            int20 altitude = ComputeAltitude(static_pressure, temperature);
            int20 vertical_speed = ComputeVerticalSpeed(prev_altitude, altitude);
            int20 air_speed = ComputeAirSpeed(impact_pressure, static_pressure);
            int20 mach_number = ComputeMachNumber(air_speed, temperature);
            
            // Update previous altitude for next vertical speed calculation
            prev_altitude = altitude;
            
            // In real CADC, these computations would be implemented using
            // complex polynomial evaluations stored in ROM and executed by the modules
            // For example: F(Vc) = a_n*Vc^n + a_(n-1)*Vc^(n-1) + ... + a_1*Vc + a_0
            // where Vc is calibrated airspeed
            
            // Example polynomial evaluation (simplified)
            // Polynomial: 0.001*x^3 - 0.1*x^2 + 2.5*x + 100
            int20 coefficients[] = {100, 2500, -100, 1};  // Coefficients a0, a1, a2, a3 (scaled)
            int20 poly_result = EvaluatePolynomial(air_speed >> 10, coefficients, 3);  // Scale down input
            
            // These values would be stored in RAS and accessed by other modules
            // in the pipeline for further computation
        }
    }
}

void CadcSystem::UpdateControlSignals() {
    // Update system control signals based on current state
    // The busy status and other signals would be sent through the Process method
    // when other components request them

    // Set other control pins as needed
}