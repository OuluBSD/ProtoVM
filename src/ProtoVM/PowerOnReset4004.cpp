#include "ProtoVM.h"
#include "PowerOnReset4004.h"

/*
 * Power-On Reset Circuit for Intel 4004 System
 *
 * Generates a reset pulse on power-on to ensure proper initialization
 * of all 4004 system components
 */

PowerOnReset4004::PowerOnReset4004() {
    reset_active = true;        // Start with reset active (power-on condition)
    reset_counter = 0;
    reset_duration = 10;        // Default reset pulse duration in ticks
    power_stable = false;
    power_counter = 0;

    // Add the pins for the power-on reset circuit
    AddSource("RESET_OUT");    // Reset output signal
    AddSink("PWR_GOOD");       // Power good input (not currently used in this simple implementation)

    in_pins = 0;

    LOG("PowerOnReset4004: Initialized with " << reset_duration << " tick reset duration");
}

bool PowerOnReset4004::Tick() {
    // Update power good status
    power_stable = (in_pins & (1 << PWR_GOOD)) != 0;
    
    // Generate reset sequence
    GenerateResetSequence();
    
    // Update outputs
    UpdateOutput();

    // Reset input values for next tick
    in_pins = 0;

    return true;
}

bool PowerOnReset4004::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    union {
        byte tmp[2];
        uint16 tmp16;
    };
    
    if (type == WRITE) {
        switch (conn_id) {
            // Handle reset output
            case RESET_OUT:
                tmp[0] = reset_active ? 1 : 0;
                return dest.PutRaw(dest_conn_id, tmp, 0, 1);

            default:
                LOG("PowerOnReset4004::Process: unimplemented connection-id " << conn_id);
                return false;
        }
    }

    return true;
}

bool PowerOnReset4004::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    bool value;
    
    switch (conn_id) {
        // Handle control pins
        case PWR_GOOD:
            ASSERT(data_bytes == 0 && data_bits == 1);
            value = (*data & 0x1) != 0;
            SetPin(conn_id, value);
            break;

        default:
            LOG("PowerOnReset4004::PutRaw: error: unsupported conn-id " << conn_id);
            return false;
    }

    return true;
}

void PowerOnReset4004::SetPin(int i, bool b) {
    uint32 mask = 1UL << i;
    if (b)
        in_pins |= mask;
    else
        in_pins &= ~mask;
}

void PowerOnReset4004::GenerateResetSequence() {
    // In the first few ticks after startup, keep reset active
    // After the reset duration, disable reset (release reset)
    if (reset_counter < reset_duration) {
        reset_active = true;
        reset_counter++;
    } else {
        reset_active = false;  // Release reset after initial delay
    }
}

void PowerOnReset4004::UpdateOutput() {
    // The reset signal is updated in GenerateResetSequence, but we update change status here
    bool output_changed = false;
    static bool last_reset = !reset_active;
    
    if (reset_active != last_reset) {
        output_changed = true;
        last_reset = reset_active;
    }
    
    SetChanged(output_changed);
}