#ifndef ProtoVM_IC4003_h
#define ProtoVM_IC4003_h

#include "Component.h"
#include "ICs.h"
#include "Common.h"
#include "Bus.h"

/*
 * Intel 4003 Shift Register Implementation for ProtoVM
 *
 * The Intel 4003 is a 4-bit shift register used for I/O expansion in the 4004 system.
 * It's typically used to expand the limited I/O pins of the 4004 CPU.
 *
 * Pinout:
 * - O0-O3: Output pins (shift register output)
 * - L0-L3: Latch pins for O0-O3 outputs
 * - CM4: Clock input
 * - SO0: Shift Out (for cascading)
 * - SR0: Serial Input
 */

class IC4003 : public Chip {
public:
    IC4003();
    virtual ~IC4003() {}

    bool Tick() override;
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;

    String GetClassName() const override { return "IC4003"; }

    // I/O character output functionality
    void SetCharacterOutputCallback(void (*callback)(char c));
    void ProcessOutputData();  // Process data when latched for character output

private:
    // 4-bit shift register
    byte shift_reg;     // 4-bit shift register: [bit3][bit2][bit1][bit0]
    byte output_latch;  // 4-bit output latch
    
    // Pin mappings (relative to AddPin calls)
    enum PinNames {
        O0 = 0,    // Output pins
        O1 = 1,
        O2 = 2,
        O3 = 3,
        L0 = 4,    // Latch pins
        L1 = 5,
        L2 = 6,
        L3 = 7,
        SR0 = 8,   // Serial input
        SO0 = 9,   // Serial output
        CM4 = 10   // Clock input
    };

    // Internal state for current tick
    uint32 in_pins;
    byte in_data;
    
    void SetPin(int i, bool b);
    void ShiftRegister();
    void LatchOutput();
    void UpdateOutput();

private:
    void (*char_output_callback)(char c);  // Callback for character output
};

#endif
