#ifndef ProtoVM_ClockGenerator4004_h
#define ProtoVM_ClockGenerator4004_h

#include "Component.h"
#include "ICs.h"
#include "Common.h"
#include "ClockDivider.h"

/*
 * Clock Generator for Intel 4004 System
 *
 * The Intel 4004 system requires specific clock timing:
 * - 4004 CPU operates with a two-phase non-overlapping clock
 * - Typical frequency around 740 kHz
 * - CM and CM4 signals need to be properly timed
 */

class ClockGenerator4004 : public Chip {
public:
    ClockGenerator4004();
    virtual ~ClockGenerator4004() {}

    bool Tick() override;
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;

    String GetClassName() const override { return "ClockGenerator4004"; }

private:
    int clock_counter;           // Internal counter for clock generation
    int clock_frequency_hz;      // Clock frequency in Hz (e.g., 740000 for 740kHz)
    int ticks_per_cycle;         // Ticks needed for one clock cycle at current simulation rate
    bool current_phase1;         // Current state of phase 1 clock
    bool current_phase2;         // Current state of phase 2 clock
    bool enable_signal;          // Clock enable input
    
    // Pin mappings (relative to AddPin calls)
    enum PinNames {
        CLK_EN = 0,    // Clock enable input
        CM4 = 1,       // Phase 1 clock output to CPU
        CM = 2,        // Phase 2 clock output to memory
        T1 = 3,        // Test point 1
        T2 = 4         // Test point 2
    };

    // Internal state for current tick
    uint32 in_pins;
    
    void SetPin(int i, bool b);
    void GenerateClockSignal();
    void UpdateOutput();
    
public:
    // Set clock frequency in Hz
    void SetClockFrequency(int freq_hz) { 
        clock_frequency_hz = freq_hz; 
        // Recalculate ticks_per_cycle based on simulation timing
        // For now, this is a placeholder value
        ticks_per_cycle = 1000000 / freq_hz;  // Simplified calculation
    }
    
    int GetClockFrequency() const { return clock_frequency_hz; }
};

#endif
