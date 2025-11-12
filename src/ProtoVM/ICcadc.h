#ifndef ProtoVM_ICcadc_h
#define ProtoVM_ICcadc_h

#include "Component.h"
#include "ICs.h"
#include "Common.h"
#include "Bus.h"

/*
 * F-14 CADC (Central Air Data Computer) Implementation for ProtoVM
 *
 * The CADC was developed by Garrett AiResearch for the F-14 Tomcat.
 * It used a chipset approach with multiple specialized chips:
 * - Parallel Multiplier Unit (PMU)
 * - Parallel Divider Unit (PDU)
 * - Special Logic Function (SLF)
 * - Steering Logic Unit (SLU)
 * - Random Access Storage (RAS)
 * - Read-Only Memory (ROM)
 *
 * Architecture:
 * - 20-bit word length (19 data bits + 1 sign bit, two's complement)
 * - 375 kHz clock frequency
 * - 9375 instructions per second
 * - 3 pipeline modules with dedicated functionality
 * - Serial data processing
 * - Pipeline concurrency
 *
 * Word timing:
 * - Each bit: 2.66μs (at 375kHz)
 * - Word time: 20 bit times = 53.2μs for 20-bit serial processing
 * - Two word types: W0 (instruction fetch), W1 (data transfer)
 */

// Helper functions for working with 20-bit values
typedef int32_t int20;  // 20-bit signed integer (using 32-bit for internal calculations)

// Common constants for CADC system
static const int CADC_WORD_LENGTH = 20;      // 20-bit words
static const int CADC_CLOCK_FREQ = 375000;   // 375 kHz
static const double CADC_BIT_TIME_US = 2.66; // 2.66μs per bit
static const double CADC_WORD_TIME_US = 53.2; // 53.2μs per word

// Base class for all CADC components
class ICcadcBase : public Chip {
public:
    ICcadcBase() : Chip() {
        // Initialize timing variables
        bit_counter = 0;
        word_counter = 0;
        current_word_time = 0;
        last_clock_edge = false;
    }
    virtual ~ICcadcBase() {}

    // Common methods for all CADC components
    virtual bool Tick() override;
    virtual bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;

    // Timing methods
    void UpdateTiming();
    virtual void UpdateState() = 0;  // Each component must implement this
    
protected:
    // Timing variables
    int bit_counter;           // Current bit in the word (0-19)
    int word_counter;          // Current word number
    int current_word_time;     // Current word time (0 for W0, 1 for W1)
    bool last_clock_edge;      // Track last clock edge state
    byte current_word_data[CADC_WORD_LENGTH/8 + 1]; // Current word data (20 bits = 3 bytes)
    
    // Convert 20-bit values to/from bytes
    void Pack20BitValue(int20 value, byte* output);
    int20 Unpack20BitValue(const byte* input);
};

// Parallel Multiplier Unit (PMU)
class ICPmu : public ICcadcBase {
public:
    ICPmu();
    virtual ~ICPmu() {}

    void UpdateState() override;
    
private:
    // Internal state for PMU
    bool in_operation;                      // Currently performing multiplication
    byte multiplicand[CADC_WORD_LENGTH/8 + 1];  // Current multiplicand (20 bits)
    byte multiplier[CADC_WORD_LENGTH/8 + 1];    // Current multiplier (20 bits)
    byte product[CADC_WORD_LENGTH/8 + 1];       // Result of multiplication (20 bits)
    
    // Methods
    void Multiply();
    
    // Pin mappings (relative to AddPin calls)
    enum PinNames {
        // Data input pins (for multiplicand and multiplier)
        DATA_IN_0 = 0,  // LSB of input data
        DATA_IN_1 = 1,
        DATA_IN_2 = 2,
        DATA_IN_3 = 3,
        DATA_IN_4 = 4,
        DATA_IN_5 = 5,
        DATA_IN_6 = 6,
        DATA_IN_7 = 7,
        DATA_IN_8 = 8,
        DATA_IN_9 = 9,
        DATA_IN_10 = 10,
        DATA_IN_11 = 11,
        DATA_IN_12 = 12,
        DATA_IN_13 = 13,
        DATA_IN_14 = 14,
        DATA_IN_15 = 15,
        DATA_IN_16 = 16,
        DATA_IN_17 = 17,
        DATA_IN_18 = 18,
        DATA_IN_19 = 19,  // MSB of input data
        
        // Data output pins (for product)
        DATA_OUT_0 = 20,
        DATA_OUT_1 = 21,
        DATA_OUT_2 = 22,
        DATA_OUT_3 = 23,
        DATA_OUT_4 = 24,
        DATA_OUT_5 = 25,
        DATA_OUT_6 = 26,
        DATA_OUT_7 = 27,
        DATA_OUT_8 = 28,
        DATA_OUT_9 = 29,
        DATA_OUT_10 = 30,
        DATA_OUT_11 = 31,
        DATA_OUT_12 = 32,
        DATA_OUT_13 = 33,
        DATA_OUT_14 = 34,
        DATA_OUT_15 = 35,
        DATA_OUT_16 = 36,
        DATA_OUT_17 = 37,
        DATA_OUT_18 = 38,
        DATA_OUT_19 = 39,  // MSB of output data
        
        // Control pins
        CLK = 40,      // Clock input (375 kHz)
        RESET = 41,    // Reset input
        START = 42,    // Start multiplication
        VALID = 43,    // Output valid flag
        BUSY = 44      // Busy flag
    };
};

// Parallel Divider Unit (PDU)
class ICPdu : public ICcadcBase {
public:
    ICPdu();
    virtual ~ICPdu() {}

    void UpdateState() override;
    
private:
    // Internal state for PDU
    bool in_operation;                    // Currently performing division
    byte dividend[CADC_WORD_LENGTH/8 + 1];  // Current dividend (20 bits)
    byte divisor[CADC_WORD_LENGTH/8 + 1];   // Current divisor (20 bits)
    byte quotient[CADC_WORD_LENGTH/8 + 1];  // Result of division (20 bits)
    
    // Methods
    void Divide();
    
    // Pin mappings
    enum PinNames {
        // Data input pins (for dividend and divisor)
        DATA_IN_0 = 0,
        DATA_IN_1 = 1,
        DATA_IN_2 = 2,
        DATA_IN_3 = 3,
        DATA_IN_4 = 4,
        DATA_IN_5 = 5,
        DATA_IN_6 = 6,
        DATA_IN_7 = 7,
        DATA_IN_8 = 8,
        DATA_IN_9 = 9,
        DATA_IN_10 = 10,
        DATA_IN_11 = 11,
        DATA_IN_12 = 12,
        DATA_IN_13 = 13,
        DATA_IN_14 = 14,
        DATA_IN_15 = 15,
        DATA_IN_16 = 16,
        DATA_IN_17 = 17,
        DATA_IN_18 = 18,
        DATA_IN_19 = 19,  // MSB of input data
        
        // Data output pins (for quotient)
        DATA_OUT_0 = 20,
        DATA_OUT_1 = 21,
        DATA_OUT_2 = 22,
        DATA_OUT_3 = 23,
        DATA_OUT_4 = 24,
        DATA_OUT_5 = 25,
        DATA_OUT_6 = 26,
        DATA_OUT_7 = 27,
        DATA_OUT_8 = 28,
        DATA_OUT_9 = 29,
        DATA_OUT_10 = 30,
        DATA_OUT_11 = 31,
        DATA_OUT_12 = 32,
        DATA_OUT_13 = 33,
        DATA_OUT_14 = 34,
        DATA_OUT_15 = 35,
        DATA_OUT_16 = 36,
        DATA_OUT_17 = 37,
        DATA_OUT_18 = 38,
        DATA_OUT_19 = 39,  // MSB of output data
        
        // Control pins
        CLK = 40,
        RESET = 41,
        START = 42,
        VALID = 43,
        BUSY = 44
    };
};

// Special Logic Function (SLF)
class ICSlf : public ICcadcBase {
public:
    ICSlf();
    virtual ~ICSlf() {}

    void UpdateState() override;
    
private:
    // Internal state for SLF
    byte upper_limit[CADC_WORD_LENGTH/8 + 1];   // Upper limit (20 bits)
    byte lower_limit[CADC_WORD_LENGTH/8 + 1];   // Lower limit (20 bits)
    byte parameter[CADC_WORD_LENGTH/8 + 1];     // Parameter to limit (20 bits)
    byte output[CADC_WORD_LENGTH/8 + 1];        // Limited parameter (20 bits)
    
    // Control/Logic state
    bool and_operation;
    bool or_operation;
    bool conditional_transfer;
    bool unconditional_transfer;
    
    // Methods
    void LimitFunction();
    void LogicOperation();
    
    // Pin mappings
    enum PinNames {
        // Data input pins (for U, P, L registers)
        DATA_IN_0 = 0,
        DATA_IN_1 = 1,
        DATA_IN_2 = 2,
        DATA_IN_3 = 3,
        DATA_IN_4 = 4,
        DATA_IN_5 = 5,
        DATA_IN_6 = 6,
        DATA_IN_7 = 7,
        DATA_IN_8 = 8,
        DATA_IN_9 = 9,
        DATA_IN_10 = 10,
        DATA_IN_11 = 11,
        DATA_IN_12 = 12,
        DATA_IN_13 = 13,
        DATA_IN_14 = 14,
        DATA_IN_15 = 15,
        DATA_IN_16 = 16,
        DATA_IN_17 = 17,
        DATA_IN_18 = 18,
        DATA_IN_19 = 19,  // MSB of input data
        
        // Data output pins
        DATA_OUT_0 = 20,
        DATA_OUT_1 = 21,
        DATA_OUT_2 = 22,
        DATA_OUT_3 = 23,
        DATA_OUT_4 = 24,
        DATA_OUT_5 = 25,
        DATA_OUT_6 = 26,
        DATA_OUT_7 = 27,
        DATA_OUT_8 = 28,
        DATA_OUT_9 = 29,
        DATA_OUT_10 = 30,
        DATA_OUT_11 = 31,
        DATA_OUT_12 = 32,
        DATA_OUT_13 = 33,
        DATA_OUT_14 = 34,
        DATA_OUT_15 = 35,
        DATA_OUT_16 = 36,
        DATA_OUT_17 = 37,
        DATA_OUT_18 = 38,
        DATA_OUT_19 = 39,  // MSB of output data
        
        // Control pins
        CLK = 40,
        RESET = 41,
        INSTR0 = 42,  // Instruction bit 0
        INSTR1 = 43,  // Instruction bit 1
        INSTR2 = 44,  // Instruction bit 2
        INSTR3 = 45,  // Instruction bit 3
        VALID = 46,
        BUSY = 47
    };
};

// Steering Logic Unit (SLU/Data Steering)
class ICSlu : public ICcadcBase {
public:
    ICSlu();
    virtual ~ICSlu() {}

    void UpdateState() override;
    
private:
    // Internal state for SLU
    byte input_data[3][CADC_WORD_LENGTH/8 + 1];  // Three input data sources
    byte output_data[3][CADC_WORD_LENGTH/8 + 1]; // Three output data destinations
    
    // Control state - instruction word determines data routing
    byte instruction_word[2];  // 15-bit instruction word (for 3 outputs)
    
    // Methods
    void RouteData();
    
    // Pin mappings (simplified for this implementation)
    enum PinNames {
        // Input data pins (3 sources)
        IN0_0 = 0,   // Input 0 data bits
        IN0_1 = 1,
        IN0_2 = 2,
        IN0_3 = 3,
        IN0_4 = 4,
        IN0_5 = 5,
        IN0_6 = 6,
        IN0_7 = 7,
        IN0_8 = 8,
        IN0_9 = 9,
        IN0_10 = 10,
        IN0_11 = 11,
        IN0_12 = 12,
        IN0_13 = 13,
        IN0_14 = 14,
        IN0_15 = 15,
        IN0_16 = 16,
        IN0_17 = 17,
        IN0_18 = 18,
        IN0_19 = 19,  // MSB of input 0
        
        IN1_0 = 20,  // Input 1 data bits
        IN1_1 = 21,
        IN1_2 = 22,
        IN1_3 = 23,
        IN1_4 = 24,
        IN1_5 = 25,
        IN1_6 = 26,
        IN1_7 = 27,
        IN1_8 = 28,
        IN1_9 = 29,
        IN1_10 = 30,
        IN1_11 = 31,
        IN1_12 = 32,
        IN1_13 = 33,
        IN1_14 = 34,
        IN1_15 = 35,
        IN1_16 = 36,
        IN1_17 = 37,
        IN1_18 = 38,
        IN1_19 = 39,  // MSB of input 1
        
        IN2_0 = 40,  // Input 2 data bits
        IN2_1 = 41,
        IN2_2 = 42,
        IN2_3 = 43,
        IN2_4 = 44,
        IN2_5 = 45,
        IN2_6 = 46,
        IN2_7 = 47,
        IN2_8 = 48,
        IN2_9 = 49,
        IN2_10 = 50,
        IN2_11 = 51,
        IN2_12 = 52,
        IN2_13 = 53,
        IN2_14 = 54,
        IN2_15 = 55,
        IN2_16 = 56,
        IN2_17 = 57,
        IN2_18 = 58,
        IN2_19 = 59,  // MSB of input 2
        
        // Output data pins (3 destinations)
        OUT0_0 = 60,
        OUT0_1 = 61,
        OUT0_2 = 62,
        OUT0_3 = 63,
        OUT0_4 = 64,
        OUT0_5 = 65,
        OUT0_6 = 66,
        OUT0_7 = 67,
        OUT0_8 = 68,
        OUT0_9 = 69,
        OUT0_10 = 70,
        OUT0_11 = 71,
        OUT0_12 = 72,
        OUT0_13 = 73,
        OUT0_14 = 74,
        OUT0_15 = 75,
        OUT0_16 = 76,
        OUT0_17 = 77,
        OUT0_18 = 78,
        OUT0_19 = 79,  // MSB of output 0
        
        OUT1_0 = 80,
        OUT1_1 = 81,
        OUT1_2 = 82,
        OUT1_3 = 83,
        OUT1_4 = 84,
        OUT1_5 = 85,
        OUT1_6 = 86,
        OUT1_7 = 87,
        OUT1_8 = 88,
        OUT1_9 = 89,
        OUT1_10 = 90,
        OUT1_11 = 91,
        OUT1_12 = 92,
        OUT1_13 = 93,
        OUT1_14 = 94,
        OUT1_15 = 95,
        OUT1_16 = 96,
        OUT1_17 = 97,
        OUT1_18 = 98,
        OUT1_19 = 99,  // MSB of output 1
        
        OUT2_0 = 100,
        OUT2_1 = 101,
        OUT2_2 = 102,
        OUT2_3 = 103,
        OUT2_4 = 104,
        OUT2_5 = 105,
        OUT2_6 = 106,
        OUT2_7 = 107,
        OUT2_8 = 108,
        OUT2_9 = 109,
        OUT2_10 = 110,
        OUT2_11 = 111,
        OUT2_12 = 112,
        OUT2_13 = 113,
        OUT2_14 = 114,
        OUT2_15 = 115,
        OUT2_16 = 116,
        OUT2_17 = 117,
        OUT2_18 = 118,
        OUT2_19 = 119,  // MSB of output 2
        
        // Control pins
        CLK = 120,
        RESET = 121,
        INSTR_0 = 122,   // Instruction word bits
        INSTR_1 = 123,
        INSTR_2 = 124,
        INSTR_3 = 125,
        INSTR_4 = 126,
        INSTR_5 = 127,
        INSTR_6 = 128,
        INSTR_7 = 129,
        INSTR_8 = 130,
        INSTR_9 = 131,
        INSTR_10 = 132,
        INSTR_11 = 133,
        INSTR_12 = 134,
        INSTR_13 = 135,
        INSTR_14 = 136,
        VALID = 137,
        BUSY = 138
    };
};

// Random Access Storage (RAS) - 16-word storage
class ICRas : public ICcadcBase {
public:
    ICRas();
    virtual ~ICRas() {}

    void UpdateState() override;
    
private:
    // Internal state for RAS
    byte memory[16][CADC_WORD_LENGTH/8 + 1];  // 16 registers of 20 bits each
    int selected_register;  // Currently selected register (0-15)
    bool write_mode;        // Whether in write mode
    
    // Methods
    void ReadRegister(int reg_num, byte* output);
    void WriteRegister(int reg_num, const byte* input);
    
    // Pin mappings
    enum PinNames {
        // Data input/output pins (20-bit bidirectional)
        DATA_0 = 0,
        DATA_1 = 1,
        DATA_2 = 2,
        DATA_3 = 3,
        DATA_4 = 4,
        DATA_5 = 5,
        DATA_6 = 6,
        DATA_7 = 7,
        DATA_8 = 8,
        DATA_9 = 9,
        DATA_10 = 10,
        DATA_11 = 11,
        DATA_12 = 12,
        DATA_13 = 13,
        DATA_14 = 14,
        DATA_15 = 15,
        DATA_16 = 16,
        DATA_17 = 17,
        DATA_18 = 18,
        DATA_19 = 19,  // MSB of data
        
        // Address selection pins (4-bit address: 0-15)
        ADDR_0 = 20,
        ADDR_1 = 21,
        ADDR_2 = 22,
        ADDR_3 = 23,
        
        // Control pins
        CLK = 24,
        RESET = 25,
        WE = 26,       // Write Enable
        OE = 27,       // Output Enable
        CS = 28        // Chip Select
    };
};

// Read-Only Memory (ROM) - 128 words of 20 bits each
class ICRom : public ICcadcBase {
public:
    ICRom();
    virtual ~ICRom() {}

    void UpdateState() override;
    
private:
    // Internal state for ROM
    byte memory[128][CADC_WORD_LENGTH/8 + 1];  // 128 words of 20 bits each
    int current_address;  // Current address (7-bit: 0-127)
    
    // Address management
    int address_register;  // Internal address register counter
    bool retain_mode;      // Whether to retain address
    bool sequential_mode;  // Whether to step through addresses
    
    // Methods
    void ReadMemory(int addr, byte* output);
    void LoadMicrocode();  // Load default microcode
    
    // Pin mappings
    enum PinNames {
        // Data output pins (20-bit)
        DATA_0 = 0,
        DATA_1 = 1,
        DATA_2 = 2,
        DATA_3 = 3,
        DATA_4 = 4,
        DATA_5 = 5,
        DATA_6 = 6,
        DATA_7 = 7,
        DATA_8 = 8,
        DATA_9 = 9,
        DATA_10 = 10,
        DATA_11 = 11,
        DATA_12 = 12,
        DATA_13 = 13,
        DATA_14 = 14,
        DATA_15 = 15,
        DATA_16 = 16,
        DATA_17 = 17,
        DATA_18 = 18,
        DATA_19 = 19,  // MSB of data
        
        // Address input pins (7-bit address: 0-127)
        ADDR_0 = 20,
        ADDR_1 = 21,
        ADDR_2 = 22,
        ADDR_3 = 23,
        ADDR_4 = 24,
        ADDR_5 = 25,
        ADDR_6 = 26,
        
        // Control pins
        CLK = 27,
        RESET = 28,
        CE = 29,       // Chip Enable
        OE = 30,       // Output Enable
        CS = 31,       // Chip Select
        
        // Address control pins
        ADDR_LOAD = 32,    // Load address
        ADDR_INC = 33,     // Increment address
        ADDR_RETAIN = 34,  // Retain address
        ADDR_RESET = 35    // Reset address to 0
    };
};

// CADC System Module - Combines arithmetic unit, SLU, RAS, and ROMs
class ICcadcModule : public ICcadcBase {
public:
    ICcadcModule();
    virtual ~ICcadcModule() {}

    void SetArithmeticUnit(ICcadcBase* unit);  // PMU, PDU, or SLF
    void UpdateState() override;
    
private:
    // Components of the module
    ICcadcBase* arithmetic_unit;  // PMU, PDU, or SLF
    ICSlu* steering_unit;         // Data steering unit
    ICRas* ras_unit;              // Random access storage
    ICRom* rom_unit;              // Instruction ROM
    
    // Module-specific state
    byte instruction_word[CADC_WORD_LENGTH/8 + 1];  // Current instruction word
    
    // Pin mappings would be specific to the module type
};

#endif