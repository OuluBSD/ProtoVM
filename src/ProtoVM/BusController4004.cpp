#include "ProtoVM.h"
#include "BusController4004.h"

/*
 * Bus Controller for Intel 4004 System Implementation
 */

BusController4004::BusController4004() {
    // Initialize internal state
    cpu_data = 0;
    rom_data = 0;
    ram_data_in = 0;
    ram_data_out = 0;
    cpu_rw = false;
    cpu_mr = false;
    cpu_mw = false;
    cpu_clock = false;
    mem_clock = false;
    
    in_cpu_data = 0;
    in_rom_data = 0;
    in_ram_data_in = 0;
    in_ram_data_out = 0;
    in_cpu_rw = false;
    in_cpu_mr = false;
    in_cpu_mw = false;
    in_cpu_clk = false;
    in_mem_clk = false;

    // Add the pins for the bus controller
    // CPU data pins (bidirectional)
    // Add the pins for the bus controller
    // CPU data pins (input from CPU when writing to memory, output to CPU when reading from memory)
    AddSink("CPU_D0_IN");     // CPU writes data to this pin when storing to memory
    AddSink("CPU_D1_IN"); 
    AddSink("CPU_D2_IN");
    AddSink("CPU_D3_IN");
    AddSource("CPU_D0_OUT");  // CPU reads data from this pin when loading from memory
    AddSource("CPU_D1_OUT");
    AddSource("CPU_D2_OUT");
    AddSource("CPU_D3_OUT");

    // ROM data pins (data output from ROM to controller)
    AddSink("ROM_D0_OUT");    // Data flows FROM ROM TO controller
    AddSink("ROM_D1_OUT");
    AddSink("ROM_D2_OUT");
    AddSink("ROM_D3_OUT");

    // RAM data pins (separate input and output)
    AddSink("RAM_DIN0");      // Data inputs to RAM
    AddSink("RAM_DIN1");
    AddSink("RAM_DIN2");
    AddSink("RAM_DIN3");
    AddSource("RAM_DOUT0");   // Data outputs from RAM
    AddSource("RAM_DOUT1");
    AddSource("RAM_DOUT2");
    AddSource("RAM_DOUT3");
    AddSink("RAM_DIN2");
    AddSink("RAM_DIN3");
    AddSource("RAM_DOUT0");   // Data outputs from RAM
    AddSource("RAM_DOUT1");
    AddSource("RAM_DOUT2");
    AddSource("RAM_DOUT3");
    
    // Control signals
    AddSink("CPU_CLK");
    AddSink("MEM_CLK");
    AddSink("CPU_RW");
    AddSink("CPU_MR");
    AddSink("CPU_MW");

    LOG("BusController4004: Initialized for 4004 CPU bus arbitration");
}

bool BusController4004::Tick() {
    // Store old values for change detection
    byte old_cpu_data = cpu_data;
    byte old_rom_data = rom_data;
    byte old_ram_data_in = ram_data_in;
    byte old_ram_data_out = ram_data_out;
    bool old_rw = cpu_rw;
    bool old_mr = cpu_mr;
    bool old_mw = cpu_mw;

    // Update internal state from input values
    cpu_rw = in_cpu_rw;
    cpu_mr = in_cpu_mr;
    cpu_mw = in_cpu_mw;
    cpu_clock = in_cpu_clk;
    mem_clock = in_mem_clk;

    // Handle bus arbitration based on control signals
    if (cpu_mr) {  // Memory Read operation
        // CPU wants to read from memory
        // If ROM is selected and enabled, route ROM data to CPU
        rom_data = in_rom_data;
        cpu_data = rom_data;  // ROM data to CPU
    } else if (cpu_mw) {  // Memory Write operation
        // CPU wants to write to memory
        cpu_data = in_cpu_data;
        ram_data_in = cpu_data;  // CPU data to RAM
    } else {
        // Default state - no memory operation
        cpu_data = in_cpu_data;
        rom_data = in_rom_data;
        ram_data_in = in_ram_data_in;
        ram_data_out = in_ram_data_out;
    }

    // Clear input values for next tick
    in_cpu_data = 0;
    in_rom_data = 0;
    in_ram_data_in = 0;
    in_ram_data_out = 0;
    in_cpu_rw = false;
    in_cpu_mr = false;
    in_cpu_mw = false;
    in_cpu_clk = false;
    in_mem_clk = false;

    // Detect changes
    bool state_changed = (cpu_data != old_cpu_data) ||
                        (rom_data != old_rom_data) ||
                        (ram_data_in != old_ram_data_in) ||
                        (ram_data_out != old_ram_data_out) ||
                        (cpu_rw != old_rw) ||
                        (cpu_mr != old_mr) ||
                        (cpu_mw != old_mw);

    SetChanged(state_changed);

    return true;
}

bool BusController4004::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    union {
        byte tmp[2];
        uint16 tmp16;
    };

    if (type == WRITE) {
        switch (conn_id) {
            // Handle RAM output pins
            case BusController4004::RAM_D0_OUT:
            case BusController4004::RAM_D1_OUT:
            case BusController4004::RAM_D2_OUT:
            case BusController4004::RAM_D3_OUT:
            {
                // Output RAM data to connected components
                int bit_pos = conn_id - BusController4004::RAM_D0_OUT;
                byte bit_val = (ram_data_out >> bit_pos) & 0x1;
                return dest.PutRaw(dest_conn_id, &bit_val, 0, 1);
            }

            // CPU data pins (when sending data to CPU)
            case BusController4004::CPU_D0_OUT:
            case BusController4004::CPU_D1_OUT:
            case BusController4004::CPU_D2_OUT:
            case BusController4004::CPU_D3_OUT:
            {
                // Send appropriate data to CPU based on which memory is selected
                int bit_pos = conn_id - BusController4004::CPU_D0_OUT;
                byte output_data = 0;
                
                // Determine what data to send based on control signals
                if (cpu_mr) { // Memory Read - send ROM data
                    output_data = (rom_data >> bit_pos) & 0x1;
                } else if (cpu_mw) { // Memory Write - send CPU data back
                    output_data = (cpu_data >> bit_pos) & 0x1;
                } else {
                    // Default state - possibly floating or last value
                    output_data = (cpu_data >> bit_pos) & 0x1;
                }
                
                return dest.PutRaw(dest_conn_id, &output_data, 0, 1);
            }

            default:
                LOG("BusController4004::Process: unimplemented connection-id " << conn_id);
                return false;
        }
    }

    return true;
}

bool BusController4004::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    bool value;

    switch (conn_id) {
        // Handle CPU data inputs (when CPU sends data to bus controller)
        case BusController4004::CPU_D0_IN:
        case BusController4004::CPU_D1_IN:
        case BusController4004::CPU_D2_IN:
        case BusController4004::CPU_D3_IN:
            if (data_bytes == 0 && data_bits == 1) {  // Single bit input
                int bit_pos = conn_id - BusController4004::CPU_D0_IN;
                byte mask = 1 << bit_pos;
                in_cpu_data = (in_cpu_data & ~mask) | ((*data & 1) << bit_pos);
            }
            break;

        // Handle ROM data inputs (data from ROM to bus controller)
        case BusController4004::ROM_D0_OUT:
        case BusController4004::ROM_D1_OUT:
        case BusController4004::ROM_D2_OUT:
        case BusController4004::ROM_D3_OUT:
            if (data_bytes == 0 && data_bits == 1) {  // Single bit input
                int bit_pos = conn_id - BusController4004::ROM_D0_OUT;
                byte mask = 1 << bit_pos;
                in_rom_data = (in_rom_data & ~mask) | ((*data & 1) << bit_pos);
            }
            break;

        // Handle RAM data inputs
        case BusController4004::RAM_D0_IN:
        case BusController4004::RAM_D1_IN:
        case BusController4004::RAM_D2_IN:
        case BusController4004::RAM_D3_IN:
            if (data_bytes == 0 && data_bits == 1) {  // Single bit input
                int bit_pos = conn_id - BusController4004::RAM_D0_IN;
                byte mask = 1 << bit_pos;
                in_ram_data_in = (in_ram_data_in & ~mask) | ((*data & 1) << bit_pos);
            }
            break;

        // Handle control inputs
        case BusController4004::CPU_RW:  // CPU R/W
            ASSERT(data_bytes == 0 && data_bits == 1);
            in_cpu_rw = (*data & 0x1) != 0;
            break;
        case BusController4004::CPU_MR:  // CPU Memory Read
            ASSERT(data_bytes == 0 && data_bits == 1);
            in_cpu_mr = (*data & 0x1) != 0;
            break;
        case BusController4004::CPU_MW:  // CPU Memory Write
            ASSERT(data_bytes == 0 && data_bits == 1);
            in_cpu_mw = (*data & 0x1) != 0;
            break;
        case BusController4004::CPU_CLK:
            ASSERT(data_bytes == 0 && data_bits == 1);
            in_cpu_clk = (*data & 0x1) != 0;
            break;
        case BusController4004::MEM_CLK:
            ASSERT(data_bytes == 0 && data_bits == 1);
            in_mem_clk = (*data & 0x1) != 0;
            break;

        default:
            LOG("BusController4004::PutRaw: error: unsupported conn-id " << conn_id);
            return false;
    }

    return true;
}

void BusController4004::SetPinState(int pin, bool state, byte value) {
    // Helper function to set internal pin states
    switch (pin) {
        case BusController4004::CPU_RW:
            in_cpu_rw = state;
            break;
        case BusController4004::CPU_MR:
            in_cpu_mr = state;
            break;
        case BusController4004::CPU_MW:
            in_cpu_mw = state;
            break;
        case BusController4004::CPU_CLK:
            in_cpu_clk = state;
            break;
        case BusController4004::MEM_CLK:
            in_mem_clk = state;
            break;
    }
}