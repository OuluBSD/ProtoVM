#include "ProtoVM.h"
#include "ClockGenerator4004.h"

/*
 * Clock Generator for Intel 4004 System
 *
 * The Intel 4004 system requires specific clock timing:
 * - 4004 CPU operates with a two-phase non-overlapping clock
 * - Typical frequency around 740 kHz
 * - CM and CM4 signals need to be properly timed
 */

ClockGenerator4004::ClockGenerator4004() {
    clock_counter = 0;
    clock_frequency_hz = 740000;  // 740 kHz typical for 4004
    ticks_per_cycle = 10;         // Placeholder for simulation timing
    current_phase1 = false;
    current_phase2 = false;
    enable_signal = true;

    // Add the pins for the clock generator
    AddSink("CLK_EN");    // Clock enable input
    AddSource("CM4");     // Phase 1 clock output to CPU
    AddSource("CM");      // Phase 2 clock output to memory
    AddSource("T1");      // Test point 1
    AddSource("T2");      // Test point 2

    in_pins = 0;

    LOG("ClockGenerator4004: Initialized with " << clock_frequency_hz << "Hz clock");
}

bool ClockGenerator4004::Tick() {
    // Update enable status from input
    enable_signal = (in_pins & (1 << CLK_EN)) != 0;
    
    // Generate clock signals if enabled
    if (enable_signal) {
        GenerateClockSignal();
    } else {
        // If disabled, keep clocks low
        current_phase1 = false;
        current_phase2 = false;
    }
    
    // Update outputs
    UpdateOutput();

    // Reset input values for next tick
    in_pins = 0;

    return true;
}

bool ClockGenerator4004::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    union {
        byte tmp[2];
        uint16 tmp16;
    };
    
    if (type == WRITE) {
        switch (conn_id) {
            // Handle clock outputs
            case CM4:
                tmp[0] = current_phase1 ? 1 : 0;
                return dest.PutRaw(dest_conn_id, tmp, 0, 1);
            case CM:
                tmp[0] = current_phase2 ? 1 : 0;
                return dest.PutRaw(dest_conn_id, tmp, 0, 1);
            case T1:
                tmp[0] = (clock_counter & 1) ? 1 : 0;  // Low freq test signal
                return dest.PutRaw(dest_conn_id, tmp, 0, 1);
            case T2:
                tmp[0] = (clock_counter & 2) ? 1 : 0;  // Low freq test signal
                return dest.PutRaw(dest_conn_id, tmp, 0, 1);

            default:
                LOG("ClockGenerator4004::Process: unimplemented connection-id " << conn_id);
                return false;
        }
    }

    return true;
}

bool ClockGenerator4004::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    bool value;
    
    switch (conn_id) {
        // Handle control pins
        case CLK_EN:
            ASSERT(data_bytes == 0 && data_bits == 1);
            value = (*data & 0x1) != 0;
            SetPin(conn_id, value);
            break;

        default:
            LOG("ClockGenerator4004::PutRaw: error: unsupported conn-id " << conn_id);
            return false;
    }

    return true;
}

void ClockGenerator4004::SetPin(int i, bool b) {
    uint32 mask = 1UL << i;
    if (b)
        in_pins |= mask;
    else
        in_pins &= ~mask;
}

void ClockGenerator4004::GenerateClockSignal() {
    // The 4004 uses a two-phase non-overlapping clock
    // Phase 1 (CM4) and Phase 2 (CM) should not be high at the same time
    
    clock_counter++;
    
    // Create a two-phase clock pattern
    // For simulation purposes, we'll have a simple pattern
    // where the phases are non-overlapping
    int phase_duration = ticks_per_cycle / 4;  // Each phase is 1/4 cycle high
    
    if (phase_duration <= 0) phase_duration = 1;  // Ensure we don't divide by zero
    
    int phase_pos = clock_counter % (phase_duration * 4);
    
    // Phase 1 high during first quarter of cycle
    if (phase_pos < phase_duration) {
        current_phase1 = true;
        current_phase2 = false;
    }
    // Dead time (both low) during second quarter
    else if (phase_pos < 2 * phase_duration) {
        current_phase1 = false;
        current_phase2 = false;
    }
    // Phase 2 high during third quarter
    else if (phase_pos < 3 * phase_duration) {
        current_phase1 = false;
        current_phase2 = true;
    }
    // Dead time (both low) during fourth quarter
    else {
        current_phase1 = false;
        current_phase2 = false;
    }
}

void ClockGenerator4004::UpdateOutput() {
    // The clocks are updated in GenerateClockSignal, but we update change status here
    bool output_changed = false;
    static bool last_phase1 = !current_phase1;
    static bool last_phase2 = !current_phase2;
    
    if (current_phase1 != last_phase1 || current_phase2 != last_phase2) {
        output_changed = true;
        last_phase1 = current_phase1;
        last_phase2 = current_phase2;
    }
    
    SetChanged(output_changed);
}