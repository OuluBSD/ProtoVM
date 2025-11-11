#include "ProtoVM.h"
#include "BusInterface4004.h"

/*
 * Bus Interface for Intel 4004 System
 *
 * Handles data bus conversion between 4-bit CPU bus and 8-bit memory buses:
 * - Converts 4-bit data from CPU to 8-bit memory interface
 * - Combines two 4-bit memory values into an 8-bit value when reading
 * - Splits 8-bit memory values into two 4-bit values when writing
 */

BusInterface4004::BusInterface4004() {
    // Initialize internal state
    cpu_data = 0;
    mem_data_low = 0;
    mem_data_high = 0;
    is_reading = false;
    is_writing = false;
    cpu_clock = false;
    mem_clock = false;
    
    in_cpu_data = 0;
    in_mem_data = 0;
    in_r_w = false;
    in_cpu_clk = false;
    in_mem_clk = false;

    // Add the pins for the bus interface
    // CPU data bus (bidirectional, 4 bits)
    AddBidirectional("CPU_D0");
    AddBidirectional("CPU_D1");
    AddBidirectional("CPU_D2");
    AddBidirectional("CPU_D3");
    
    // Memory data bus (bidirectional, 8 bits)
    AddBidirectional("MEM_D0");
    AddBidirectional("MEM_D1");
    AddBidirectional("MEM_D2");
    AddBidirectional("MEM_D3");
    AddBidirectional("MEM_D4");
    AddBidirectional("MEM_D5");
    AddBidirectional("MEM_D6");
    AddBidirectional("MEM_D7");

    // Control signals
    AddSink("R/W");      // Read/Write control from CPU
    AddSink("CPU_CLK");  // CPU clock
    AddSink("MEM_CLK");  // Memory clock
    
    LOG("BusInterface4004: Initialized for 4-bit to 8-bit bus conversion");
}

bool BusInterface4004::Tick() {
    // Store old values for change detection
    byte old_cpu_data = cpu_data;
    byte old_mem_data_low = mem_data_low;
    byte old_mem_data_high = mem_data_high;
    bool old_reading = is_reading;
    bool old_writing = is_writing;

    // Update internal state from input values
    is_reading = in_r_w;
    is_writing = !in_r_w;  // Write when R/W is high
    cpu_clock = in_cpu_clk;
    mem_clock = in_mem_clk;

    if (is_reading) {
        // Reading from memory to CPU
        // For 4004, we need to handle the memory differently
        // The 4004 has separate 4-bit memory chips (4002), so we'll simulate
        // reading the low 4 bits from memory
        cpu_data = mem_data_low & 0x0F;
    } else if (is_writing) {
        // Writing from CPU to memory
        // Combine CPU data with high nibble if available
        mem_data_low = (mem_data_low & 0xF0) | (cpu_data & 0x0F);
    }

    // Clear input values for next tick
    in_cpu_data = 0;
    in_mem_data = 0;
    in_r_w = false;
    in_cpu_clk = false;
    in_mem_clk = false;

    // Detect changes
    bool state_changed = (cpu_data != old_cpu_data) ||
                        (mem_data_low != old_mem_data_low) ||
                        (mem_data_high != old_mem_data_high) ||
                        (is_reading != old_reading) ||
                        (is_writing != old_writing);

    SetChanged(state_changed);

    return true;
}

bool BusInterface4004::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    union {
        byte tmp[2];
        uint16 tmp16;
    };

    if (type == WRITE) {
        switch (conn_id) {
            // Handle CPU data bus outputs
            case CPU_D0:
            case CPU_D1:
            case CPU_D2:
            case CPU_D3:
                if (is_writing) {  // When writing to memory
                    // Extract the correct bit of CPU data
                    byte bit_val = (cpu_data >> (conn_id - CPU_D0)) & 0x1;
                    return dest.PutRaw(dest_conn_id, &bit_val, 0, 1);
                }
                break;

            // Handle memory data bus outputs
            case MEM_D0:
            case MEM_D1:
            case MEM_D2:
            case MEM_D3:
            case MEM_D4:
            case MEM_D5:
            case MEM_D6:
            case MEM_D7:
                if (is_reading) {  // When reading from memory to CPU
                    // Extract the correct bit of memory data
                    byte bit_val = (mem_data_low >> (conn_id - MEM_D0)) & 0x1;
                    return dest.PutRaw(dest_conn_id, &bit_val, 0, 1);
                }
                break;

            default:
                LOG("BusInterface4004::Process: unimplemented connection-id " << conn_id);
                return false;
        }
    }

    return true;
}

bool BusInterface4004::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    bool value;

    switch (conn_id) {
        // Handle CPU data bus inputs (reading from CPU)
        case CPU_D0:
        case CPU_D1:
        case CPU_D2:
        case CPU_D3:
            if (data_bytes == 0 && data_bits == 1) {  // Single bit input
                byte bit_pos = conn_id - CPU_D0;
                byte mask = 1 << bit_pos;
                cpu_data = (cpu_data & ~mask) | ((*data & 1) << bit_pos);
            }
            break;

        // Handle memory data bus inputs (reading from memory)
        case MEM_D0:
        case MEM_D1:
        case MEM_D2:
        case MEM_D3:
        case MEM_D4:
        case MEM_D5:
        case MEM_D6:
        case MEM_D7:
            if (data_bytes == 0 && data_bits == 1) {  // Single bit input
                byte bit_pos = conn_id - MEM_D0;
                byte mask = 1 << bit_pos;
                mem_data_low = (mem_data_low & ~mask) | ((*data & 1) << bit_pos);
            }
            break;

        // Handle control inputs
        case R_W:
            ASSERT(data_bytes == 0 && data_bits == 1);
            in_r_w = (*data & 0x1) != 0;
            break;
        case CPU_CLK:
            ASSERT(data_bytes == 0 && data_bits == 1);
            in_cpu_clk = (*data & 0x1) != 0;
            break;
        case MEM_CLK:
            ASSERT(data_bytes == 0 && data_bits == 1);
            in_mem_clk = (*data & 0x1) != 0;
            break;

        default:
            LOG("BusInterface4004::PutRaw: error: unsupported conn-id " << conn_id);
            return false;
    }

    return true;
}

void BusInterface4004::SetPinState(int pin, bool state, byte value) {
    // Helper function to set internal pin states
    switch (pin) {
        case R_W:
            in_r_w = state;
            break;
        case CPU_CLK:
            in_cpu_clk = state;
            break;
        case MEM_CLK:
            in_mem_clk = state;
            break;
        // Additional cases for data pins if needed
    }
}