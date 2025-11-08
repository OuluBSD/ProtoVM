#ifndef ProtoVM_PowerOnReset4004_h
#define ProtoVM_PowerOnReset4004_h

#include "Component.h"
#include "ICs.h"
#include "Common.h"

/*
 * Power-On Reset Circuit for Intel 4004 System
 *
 * Generates a reset pulse on power-on to ensure proper initialization
 * of all 4004 system components
 */

class PowerOnReset4004 : public Chip {
public:
    PowerOnReset4004();
    virtual ~PowerOnReset4004() {}

    virtual bool Tick();
    virtual bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id);
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits);

    virtual const char* GetClassName() const { return "PowerOnReset4004"; }

private:
    bool reset_active;         // Current reset state
    int reset_counter;         // Counter for reset duration
    int reset_duration;        // How many ticks the reset should last
    bool power_stable;         // Whether power has stabilized
    int power_counter;         // Counter for power stabilization

    // Pin mappings (relative to AddPin calls)
    enum PinNames {
        RESET_OUT = 0,    // Reset output signal
        PWR_GOOD = 1      // Power good input
    };

    // Internal state for current tick
    uint32 in_pins;
    
    void SetPin(int i, bool b);
    void GenerateResetSequence();
    void UpdateOutput();
    
public:
    void SetResetDuration(int ticks) { reset_duration = ticks; }
    int GetResetDuration() const { return reset_duration; }
};

#endif