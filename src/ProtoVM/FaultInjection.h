#ifndef _ProtoVM_FaultInjection_h_
#define _ProtoVM_FaultInjection_h_

#include "ProtoVM.h"

// Types of faults that can be injected
enum FaultType {
    FAULT_STUCK_AT_0,      // Signal stuck at 0
    FAULT_STUCK_AT_1,      // Signal stuck at 1
    FAULT_OPEN_CIRCUIT,    // Signal never changes (freezes)
    FAULT_SHORT_CIRCUIT,   // Two signals shorted together
    FAULT_DELAY,           // Signal has extra delay
    FAULT_NOISE,           // Signal has random noise
    FAULT_POWER,           // Power-related faults
    FAULT_CLOCK,           // Clock-related faults
    FAULT_COUNT            // Total number of fault types
};

// Structure to represent a single fault
struct FaultDescriptor : Moveable<FaultDescriptor> {
    String component_name;    // Name of the component to inject the fault into
    String pin_name;          // Name of the pin/signal to inject the fault into
    FaultType fault_type;     // Type of fault to inject
    int start_tick;           // Tick number when fault should start
    int duration;             // Duration of the fault in ticks (-1 for permanent)
    double probability;       // For probabilistic faults
    byte fault_value;         // Specific value for stuck-at faults
    int additional_param;     // Additional parameter specific to fault type
    bool active;              // Whether the fault is currently active
    int fault_id;             // Unique identifier for this fault
    
    FaultDescriptor() 
        : fault_type(FAULT_STUCK_AT_0), start_tick(0), duration(0), 
          probability(1.0), fault_value(0), additional_param(0), active(false), fault_id(0) {}
};

// Structure to represent fault injection results
struct FaultInjectionResult : Moveable<FaultInjectionResult> {
    int fault_id;
    String fault_description;
    bool caused_failure;
    String failure_type;
    int tick_of_failure;
    String component_state_at_failure;
    
    FaultInjectionResult() : fault_id(-1), caused_failure(false), tick_of_failure(-1) {}
};

// Fault injection manager that can inject various types of faults into the system
class FaultInjectionManager {
private:
    Vector<FaultDescriptor> scheduled_faults;
    Vector<FaultInjectionResult> results;
    Machine* machine;
    int current_tick;
    bool injection_active;
    
public:
    FaultInjectionManager(Machine* mach = nullptr);
    void SetMachine(Machine* mach) { machine = mach; }
    
    // Methods to schedule faults
    int ScheduleStuckAtFault(const String& comp_name, const String& pin_name, byte value, int start_tick, int duration = -1);
    int ScheduleOpenCircuitFault(const String& comp_name, const String& pin_name, int start_tick, int duration = -1);
    int ScheduleShortCircuitFault(const String& comp1, const String& pin1, const String& comp2, const String& pin2, int start_tick, int duration = -1);
    int ScheduleNoiseFault(const String& comp_name, const String& pin_name, double noise_prob, int start_tick, int duration = -1);
    int ScheduleDelayFault(const String& comp_name, const String& pin_name, int extra_delay, int start_tick, int duration = -1);
    
    // Generic fault scheduling
    int ScheduleFault(const FaultDescriptor& fault);
    
    // Fault injection during simulation
    void InjectFaults();
    void ProcessActiveFaults();
    void ActivateFault(int fault_id);
    void DeactivateFault(int fault_id);
    void RemoveFault(int fault_id);
    
    // Simulation hooks
    void OnPreTick();
    void OnPostTick();
    
    // Results management
    void ReportFaultInjectionResults() const;
    const Vector<FaultInjectionResult>& GetResults() const { return results; }
    void ClearResults();
    
    // Utility methods
    void EnableInjection() { injection_active = true; }
    void DisableInjection() { injection_active = false; }
    bool IsInjectionEnabled() const { return injection_active; }
    const Vector<FaultDescriptor>& GetScheduledFaults() const { return scheduled_faults; }
    
    // Verification methods
    bool VerifyFaultTolerance(const String& test_name, int max_ticks = 1000);
    void RunFaultToleranceTests();
};

// Specialized fault injectors for specific fault types
class StuckAtFaultInjector {
public:
    static bool ApplyFault(ElectricNodeBase* component, const String& pin_name, byte fault_value);
    static bool RemoveFault(ElectricNodeBase* component, const String& pin_name);
};

class NoiseFaultInjector {
public:
    static bool ApplyFault(ElectricNodeBase* component, const String& pin_name, double noise_probability);
    static bool RemoveFault(ElectricNodeBase* component, const String& pin_name);
    static byte AddNoise(byte original_value, double noise_prob);
};

class DelayFaultInjector {
public:
    static bool ApplyFault(ElectricNodeBase* component, const String& pin_name, int extra_delay_ticks);
    static bool RemoveFault(ElectricNodeBase* component, const String& pin_name);
};

// Component wrapper that can have faults injected into it
class FaultInjectableComponent : public ElcBase {
private:
    Vector<FaultDescriptor> active_faults;
    bool fault_mode;
    
public:
    FaultInjectableComponent();
    
    // Fault management methods
    void AddFault(const FaultDescriptor& fault);
    void RemoveFault(int fault_id);
    bool HasActiveFaults() const { return !active_faults.IsEmpty(); }
    void ClearFaults();
    
    // Override base class methods to handle faults
    bool Tick() override;
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    
    // Fault handling methods
    byte ApplyFaultToValue(byte original_value, uint16 conn_id);
    bool IsFaultActiveForPin(uint16 conn_id);
};

#endif