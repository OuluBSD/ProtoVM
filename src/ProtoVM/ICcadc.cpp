#include "ProtoVM.h"
#include "ICcadc.h"

/*
 * F-14 CADC (Central Air Data Computer) Implementation for ProtoVM
 *
 * Implements the CADC architecture with:
 * - Parallel Multiplier Unit (PMU)
 * - Parallel Divider Unit (PDU)
 * - Special Logic Function (SLF)
 * - Steering Logic Unit (SLU)
 * - Random Access Storage (RAS)
 * - Read-Only Memory (ROM)
 */

// Helper functions for working with 20-bit values
typedef signed int int20;  // 20-bit signed integer

void ICcadcBase::Pack20BitValue(int20 value, byte* output) {
    // Convert 20-bit value to 3 bytes (20 bits)
    // Sign-extend if necessary
    if (value < 0) {
        // Handle negative numbers in two's complement
        int32_t temp = value & 0xFFFFF;  // Mask to 20 bits
        output[0] = temp & 0xFF;         // LSB
        output[1] = (temp >> 8) & 0xFF;
        output[2] = (temp >> 16) & 0x0F; // Only 4 bits needed
    } else {
        output[0] = value & 0xFF;        // LSB
        output[1] = (value >> 8) & 0xFF;
        output[2] = (value >> 16) & 0x0F; // Only 4 bits needed
    }
}

int20 ICcadcBase::Unpack20BitValue(const byte* input) {
    // Convert 3 bytes back to 20-bit signed integer
    uint32_t temp = (input[2] & 0x0F) << 16;  // Only use 4 bits from MSB
    temp |= input[1] << 8;
    temp |= input[0];
    
    // Check if it's a negative number (bit 19 set)
    if (temp & 0x80000) {  // If sign bit is set
        temp |= 0xFFF00000;  // Sign-extend to fill upper 12 bits
    }
    
    return static_cast<int20>(temp);
}

bool ICcadcBase::Tick() {
    // Update timing and state
    UpdateTiming();
    UpdateState();
    
    // Store old values to detect changes
    bool old_changed = HasChanged();
    
    // Default: no change occurred
    SetChanged(false);
    
    return true;
}

bool ICcadcBase::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    byte temp_data[3];
    
    if (type == WRITE) {
        // Handle output based on connection ID
        if (conn_id >= 0 && conn_id < CADC_WORD_LENGTH) {
            // Data output pins - send current word data
            int byte_idx = conn_id / 8;
            int bit_idx = conn_id % 8;
            temp_data[0] = (current_word_data[byte_idx] >> bit_idx) & 1;
            return dest.PutRaw(dest_conn_id, temp_data, 0, 1);
        }
    } else if (type == TICK) {
        // Handle tick operations
        return Tick();
    }
    
    return true;
}

bool ICcadcBase::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (data_bytes == 0 && data_bits == 1) {
        // Single bit input - likely control signals or data bits
        // For now, just store the bit value based on conn_id
        if (conn_id < CADC_WORD_LENGTH) {
            // This is a data input bit
            int byte_idx = conn_id / 8;
            int bit_idx = conn_id % 8;
            byte mask = 1 << bit_idx;
            current_word_data[byte_idx] = (current_word_data[byte_idx] & ~mask) | ((*data & 1) << bit_idx);
        }
    } else if (data_bytes <= 3 && data_bits == 0) {
        // Multi-bit input (e.g., whole 20-bit values)
        for (int i = 0; i < data_bytes && i < 3; i++) {
            current_word_data[i] = data[i];
        }
    }
    
    return true;
}

void ICcadcBase::UpdateTiming() {
    // Update bit counter based on clock
    // In real CADC: 375 kHz clock = 2.66 μs per bit time
    bit_counter = (bit_counter + 1) % CADC_WORD_LENGTH;
    
    if (bit_counter == 0) {
        // Completed a word time
        word_counter++;
        current_word_time = (current_word_time + 1) % 2;  // Alternates between W0 and W1
    }
}

// PMU Implementation
ICPmu::ICPmu() {
    // Initialize PMU internal state
    in_operation = false;
    for (int i = 0; i < CADC_WORD_LENGTH/8 + 1; i++) {
        multiplicand[i] = 0;
        multiplier[i] = 0;
        product[i] = 0;
    }
    
    // Add pins for the PMU (28-pin DIP as per documentation)
    // Data input pins
    for (int i = 0; i < CADC_WORD_LENGTH; i++) {
        String name = "DI" + AsString(i);
        AddSink(name);
    }
    
    // Data output pins
    for (int i = 0; i < CADC_WORD_LENGTH; i++) {
        String name = "DO" + AsString(i);
        AddSource(name);
    }
    
    // Control pins
    AddSink("CLK");
    AddSink("RESET");
    AddSink("START");
    AddSource("VALID");
    AddSource("BUSY");
    
    LOG("ICPmu: Initialized with " << GetConnectorCount() << " pins");
}

void ICPmu::UpdateState() {
    // Check for control signals
    // This is a simplified implementation - real PMU would do Booth's algorithm
    
    // Perform multiplication if START is high and not currently operating
    // For simulation, assume inputs are ready and multiply immediately
    
    int20 a = Unpack20BitValue(multiplicand);
    int20 b = Unpack20BitValue(multiplier);
    int20 result = a * b;
    
    // In CADC, products would be properly rounded and scaled
    // For now, just store the result
    Pack20BitValue(result, product);
    
    // Update control signals
    // The busy and valid signals would be handled through the PutRaw/Process methods
    // based on the current state and operations
    
    // Update output data pins
    // This would happen through the Process method when other components request data
}

void ICPmu::Multiply() {
    // Perform the multiplication using Booth's algorithm
    // This is a simplified implementation
    int20 a = Unpack20BitValue(multiplicand);
    int20 b = Unpack20BitValue(multiplier);
    int20 result = a * b;
    
    Pack20BitValue(result, product);
    in_operation = false;
}

// PDU Implementation
ICPdu::ICPdu() {
    // Initialize PDU internal state
    in_operation = false;
    for (int i = 0; i < CADC_WORD_LENGTH/8 + 1; i++) {
        dividend[i] = 0;
        divisor[i] = 0;
        quotient[i] = 0;
    }
    
    // Add pins for the PDU (28-pin DIP as per documentation)
    // Data input pins
    for (int i = 0; i < CADC_WORD_LENGTH; i++) {
        String name = "DI" + AsString(i);
        AddSink(name);
    }
    
    // Data output pins
    for (int i = 0; i < CADC_WORD_LENGTH; i++) {
        String name = "DO" + AsString(i);
        AddSource(name);
    }
    
    // Control pins
    AddSink("CLK");
    AddSink("RESET");
    AddSink("START");
    AddSource("VALID");
    AddSource("BUSY");
    
    LOG("ICPdu: Initialized with " << GetConnectorCount() << " pins");
}

void ICPdu::UpdateState() {
    // Check for control signals
    // This is a simplified implementation - real PDU would do non-restoring division
    
    // Perform division if START is high and not currently operating
    int20 divd = Unpack20BitValue(dividend);
    int20 divs = Unpack20BitValue(divisor);
    int20 result = 0;
    
    if (divs != 0) {  // Avoid division by zero
        result = divd / divs;
    }
    
    Pack20BitValue(result, quotient);
    
    // Update control signals
    // The busy and valid signals would be handled through the PutRaw/Process methods
    // based on the current state and operations
    
    // Update output data pins
    // This would happen through the Process method when other components request data
}

void ICPdu::Divide() {
    // Perform the division using non-restoring algorithm
    // This is a simplified implementation
    int20 divd = Unpack20BitValue(dividend);
    int20 divs = Unpack20BitValue(divisor);
    int20 result = 0;
    
    if (divs != 0) {
        result = divd / divs;
    }
    
    Pack20BitValue(result, quotient);
    in_operation = false;
}

// SLF Implementation
ICSlf::ICSlf() {
    // Initialize SLF internal state
    for (int i = 0; i < CADC_WORD_LENGTH/8 + 1; i++) {
        upper_limit[i] = 0;
        lower_limit[i] = 0;
        parameter[i] = 0;
        output[i] = 0;
    }
    
    and_operation = false;
    or_operation = false;
    conditional_transfer = false;
    unconditional_transfer = false;
    
    // Add pins for the SLF (28-pin DIP as per documentation)
    // Data input pins
    for (int i = 0; i < CADC_WORD_LENGTH; i++) {
        String name = "DI" + AsString(i);
        AddSink(name);
    }
    
    // Data output pins
    for (int i = 0; i < CADC_WORD_LENGTH; i++) {
        String name = "DO" + AsString(i);
        AddSource(name);
    }
    
    // Control pins
    AddSink("CLK");
    AddSink("RESET");
    AddSink("INSTR0");
    AddSink("INSTR1");
    AddSink("INSTR2");
    AddSink("INSTR3");
    AddSource("VALID");
    AddSource("BUSY");
    
    LOG("ICSlf: Initialized with " << GetConnectorCount() << " pins");
}

void ICSlf::UpdateState() {
    // Apply the limit function or logic operation based on instruction
    LimitFunction();
    
    // Update control signals
    // The valid signal would be handled through the PutRaw/Process methods
    // based on the current state and operations
    
    // Update output data pins
    // This would happen through the Process method when other components request data
}

void ICSlf::LimitFunction() {
    // Implements the limit function:
    // P if U >= P >= L
    // L if P < L
    // U if P > U
    int20 p = Unpack20BitValue(parameter);
    int20 u = Unpack20BitValue(upper_limit);
    int20 l = Unpack20BitValue(lower_limit);
    int20 result = p;  // Default to parameter value
    
    if (p < l) {
        result = l;
    } else if (p > u) {
        result = u;
    }
    
    Pack20BitValue(result, output);
}

void ICSlf::LogicOperation() {
    // Perform AND/OR operations
    // This would depend on the specific instruction bits
    if (and_operation) {
        for (int i = 0; i < CADC_WORD_LENGTH/8 + 1; i++) {
            output[i] = upper_limit[i] & parameter[i];
        }
    } else if (or_operation) {
        for (int i = 0; i < CADC_WORD_LENGTH/8 + 1; i++) {
            output[i] = upper_limit[i] | parameter[i];
        }
    }
}

// SLU Implementation
ICSlu::ICSlu() {
    // Initialize SLU internal state
    for (int src = 0; src < 3; src++) {
        for (int i = 0; i < CADC_WORD_LENGTH/8 + 1; i++) {
            input_data[src][i] = 0;
            output_data[src][i] = 0;
        }
    }
    
    // Initialize instruction word
    instruction_word[0] = 0;
    instruction_word[1] = 0;
    
    // Add pins for the SLU (28-pin DIP as per documentation)
    // Input data pins (3 sources)
    for (int src = 0; src < 3; src++) {
        for (int i = 0; i < CADC_WORD_LENGTH; i++) {
            String name = "IN" + AsString(src) + "_" + AsString(i);
            AddSink(name);
        }
    }
    
    // Output data pins (3 destinations)
    for (int dst = 0; dst < 3; dst++) {
        for (int i = 0; i < CADC_WORD_LENGTH; i++) {
            String name = "OUT" + AsString(dst) + "_" + AsString(i);
            AddSource(name);
        }
    }
    
    // Instruction control pins
    AddSink("CLK");
    AddSink("RESET");
    for (int i = 0; i < 15; i++) {  // 15-bit instruction word
        String name = "INSTR" + AsString(i);
        AddSink(name);
    }
    AddSource("VALID");
    AddSource("BUSY");
    
    LOG("ICSlu: Initialized with " << GetConnectorCount() << " pins");
}

void ICSlu::UpdateState() {
    // Route the data based on the instruction word
    RouteData();
    
    // Update control signals
    // The valid signal would be handled through the PutRaw/Process methods
    // based on the current state and operations
    
    // Update output data pins
    // This would happen through the Process method when other components request data
}

void ICSlu::RouteData() {
    // Decode the 15-bit instruction to determine data routing
    // Based on documentation, the instruction format determines which inputs go to which outputs
    // This is a simplified implementation
    
    // For each output, determine what input to route based on instruction bits
    for (int i = 0; i < 3; i++) {
        // In a real implementation, this would use the instruction bits to determine routing
        // For this simulation, just pass through input 0 to output 0, etc.
        for (int j = 0; j < CADC_WORD_LENGTH/8 + 1; j++) {
            if (i < 3) {  // We have 3 outputs
                output_data[i][j] = input_data[i][j];
            }
        }
    }
}

// RAS Implementation
ICRas::ICRas() {
    // Initialize RAS internal state
    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < CADC_WORD_LENGTH/8 + 1; j++) {
            memory[i][j] = 0;
        }
    }
    selected_register = 0;
    write_mode = false;
    
    // Add pins for the RAS (14-pin DIP as per documentation)
    // Data I/O pins (20 bits)
    for (int i = 0; i < CADC_WORD_LENGTH; i++) {
        String name = "D" + AsString(i);
        AddBidirectional(name);
    }
    
    // Address selection pins (4 bits for 16 registers)
    for (int i = 0; i < 4; i++) {
        String name = "A" + AsString(i);
        AddSink(name);
    }
    
    // Control pins
    AddSink("CLK");
    AddSink("RESET");
    AddSink("WE");  // Write Enable
    AddSink("OE");  // Output Enable
    AddSink("CS");  // Chip Select
    
    LOG("ICRas: Initialized with " << GetConnectorCount() << " pins");
}

void ICRas::UpdateState() {
    // Update based on control signals
    // Read address from address pins
    selected_register = 0;
    for (int i = 0; i < 4; i++) {
        // This is a simplified approach - we'd need to read the actual pin values
        // For now, assume address is set by some mechanism
    }
    
    // If chip select is active and write enable is high, perform write
    // If chip select is active and output enable is high, perform read
    bool chip_select = true;  // Simplified
    bool write_enable = true; // Would come from control pin
    bool output_enable = true; // Would come from control pin
    
    if (chip_select && write_enable) {
        // Write to the selected register
        WriteRegister(selected_register, current_word_data);
    } else if (chip_select && output_enable) {
        // Read from the selected register
        ReadRegister(selected_register, current_word_data);
    }
}

void ICRas::ReadRegister(int reg_num, byte* output) {
    if (reg_num >= 0 && reg_num < 16) {
        for (int i = 0; i < CADC_WORD_LENGTH/8 + 1; i++) {
            output[i] = memory[reg_num][i];
        }
    }
}

void ICRas::WriteRegister(int reg_num, const byte* input) {
    if (reg_num >= 0 && reg_num < 16) {
        for (int i = 0; i < CADC_WORD_LENGTH/8 + 1; i++) {
            memory[reg_num][i] = input[i];
        }
    }
}

// ROM Implementation
ICRom::ICRom() {
    // Initialize ROM internal state
    for (int i = 0; i < 128; i++) {
        for (int j = 0; j < CADC_WORD_LENGTH/8 + 1; j++) {
            memory[i][j] = 0;
        }
    }
    current_address = 0;
    address_register = 0;
    retain_mode = false;
    sequential_mode = false;
    
    // Load default microcode
    LoadMicrocode();
    
    // Add pins for the ROM (14-pin DIP as per documentation)
    // Data output pins (20 bits)
    for (int i = 0; i < CADC_WORD_LENGTH; i++) {
        String name = "D" + AsString(i);
        AddSource(name);
    }
    
    // Address selection pins (7 bits for 128 words)
    for (int i = 0; i < 7; i++) {
        String name = "A" + AsString(i);
        AddSink(name);
    }
    
    // Control pins
    AddSink("CLK");
    AddSink("RESET");
    AddSink("CE");  // Chip Enable
    AddSink("OE");  // Output Enable
    
    // Address control pins
    AddSink("ADDR_LOAD");
    AddSink("ADDR_INC");
    AddSink("ADDR_RETAIN");
    AddSink("ADDR_RESET");
    
    LOG("ICRom: Initialized with " << GetConnectorCount() << " pins");
}

void ICRom::UpdateState() {
    // Handle address management and memory read
    bool chip_enable = true;  // Would come from control pin
    bool output_enable = true; // Would come from control pin
    
    // Handle address control signals - these would be implemented based on actual pin states
    // For now, we'll just implement basic address management
    // In a real implementation, we'd check the actual pin values
    
    // Update current address
    current_address = address_register;
    
    // If enabled, output the data from the current address
    if (chip_enable && output_enable) {
        ReadMemory(current_address, current_word_data);
    }
}

void ICRom::ReadMemory(int addr, byte* output) {
    if (addr >= 0 && addr < 128) {
        for (int i = 0; i < CADC_WORD_LENGTH/8 + 1; i++) {
            output[i] = memory[addr][i];
        }
    }
}

void ICRom::LoadMicrocode() {
    // Load default microcode for testing
    // This would contain actual CADC algorithms for air data computation
    
    // Example: Initialize with simple test pattern
    for (int addr = 0; addr < 128; addr++) {
        // Each address contains a 20-bit word
        int20 value = (addr << 10) | (addr & 0x3FF);  // Example pattern
        Pack20BitValue(value, memory[addr]);
    }
}

// CADC Module Implementation
ICcadcModule::ICcadcModule() {
    arithmetic_unit = nullptr;
    steering_unit = nullptr;
    ras_unit = nullptr;
    rom_unit = nullptr;
    
    // Initialize instruction word
    for (int i = 0; i < CADC_WORD_LENGTH/8 + 1; i++) {
        instruction_word[i] = 0;
    }
    
    LOG("ICcadcModule: Initialized");
}

void ICcadcModule::SetArithmeticUnit(ICcadcBase* unit) {
    arithmetic_unit = unit;
}

void ICcadcModule::UpdateState() {
    // Update all components in the module
    if (arithmetic_unit) {
        arithmetic_unit->UpdateState();
    }
    if (steering_unit) {
        steering_unit->UpdateState();
    }
    if (ras_unit) {
        ras_unit->UpdateState();
    }
    if (rom_unit) {
        rom_unit->UpdateState();
    }
}