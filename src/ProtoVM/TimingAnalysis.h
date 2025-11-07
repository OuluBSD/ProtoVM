#ifndef _ProtoVM_TimingAnalysis_h_
#define _ProtoVM_TimingAnalysis_h_

#include "ProtoVM.h"

// Enhanced timing analysis tools
// Includes propagation delay measurement, timing path analysis, and setup/hold time checking

// Structure to store timing path information
struct TimingPath : Moveable<TimingPath> {
    Vector<ElectricNodeBase*> components;  // Components in the timing path
    Vector<String> connections;            // Connections between components
    int total_delay;                       // Total delay in simulation ticks
    String path_name;                      // Name of the path for identification
    
    TimingPath() : total_delay(0) {}
};

// Structure to store timing violation information
struct TimingViolation : Moveable<TimingViolation> {
    String component_name;
    String violation_type;   // "SETUP", "HOLD", "MAX_DELAY", etc.
    String details;
    int tick_number;
    double delay_value;      // The delay that caused the violation
    
    TimingViolation() : tick_number(0), delay_value(0.0) {}
};

// Class for performing detailed timing analysis
class TimingAnalyzer {
private:
    Machine* machine;
    Vector<TimingPath> timing_paths;
    Vector<TimingViolation> violations;
    int current_tick;
    
public:
    TimingAnalyzer(Machine* mach);
    
    // Path discovery and analysis
    void DiscoverAllTimingPaths();
    void DiscoverTimingPathFrom(ElectricNodeBase* start_component);
    void DiscoverTimingPathTo(ElectricNodeBase* end_component);
    void AnalyzeTimingPath(TimingPath& path);
    
    // Propagation delay analysis
    void AnalyzePropagationDelays();
    int CalculatePathDelay(const Vector<ElectricNodeBase*>& path) const;
    void ReportPropagationDelays() const;
    
    // Timing constraint checking
    void CheckSetupHoldTimes();
    void CheckMaxDelayConstraints();
    void CheckClockDomainCrossings();
    
    // Results reporting
    void ReportTimingAnalysis() const;
    void ReportTimingViolations() const;
    const Vector<TimingPath>& GetTimingPaths() const { return timing_paths; }
    const Vector<TimingViolation>& GetViolations() const { return violations; }
    
    // Helper methods
    void AddViolation(const String& comp_name, const String& type, const String& details);
    void ClearResults();
    
    // Performance analysis
    void IdentifyCriticalPaths();
    void ReportCriticalPaths(int limit = 10) const;
};

// Enhanced component with detailed timing info
class TimedComponent : public ElcBase {
private:
    // Timing information for this component
    int propagation_delay;           // Delay in simulation ticks
    int setup_time;                  // Setup time requirement in ticks
    int hold_time;                   // Hold time requirement in ticks
    int clock_to_q_delay;            // Clock-to-Q delay for sequential elements
    Vector<int> input_delays;        // Individual input delays
    
    // Timing path tracking
    Vector<ElectricNodeBase*> fan_in;   // Components that drive inputs to this component
    Vector<ElectricNodeBase*> fan_out;  // Components that are driven by outputs of this component
    
public:
    TimedComponent();
    
    // Timing configuration
    void SetPropagationDelay(int delay) { propagation_delay = delay; }
    void SetSetupTime(int time) { setup_time = time; }
    void SetHoldTime(int time) { hold_time = time; }
    void SetClockToQDelay(int delay) { clock_to_q_delay = delay; }
    void SetInputDelay(int input_idx, int delay);
    
    // Getters
    int GetPropagationDelay() const { return propagation_delay; }
    int GetSetupTime() const { return setup_time; }
    int GetHoldTime() const { return hold_time; }
    int GetClockToQDelay() const { return clock_to_q_delay; }
    int GetInputDelay(int input_idx) const;
    
    // Fan-in/fan-out tracking
    void AddFanInComponent(ElectricNodeBase* comp);
    void AddFanOutComponent(ElectricNodeBase* comp);
    const Vector<ElectricNodeBase*>& GetFanIn() const { return fan_in; }
    const Vector<ElectricNodeBase*>& GetFanOut() const { return fan_out; }
    
    // Override base class methods
    bool Tick() override;
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
};

// Specialized measurement component for timing analysis
class TimingProbe : public ElcBase {
private:
    ElectricNodeBase* target_component;
    String target_pin;
    Vector<int> timestamps;        // Timestamps of signal changes
    Vector<byte> values;           // Signal values at each timestamp
    int last_change_tick;          // Tick when last change occurred
    
public:
    TimingProbe(ElectricNodeBase* comp = nullptr, const String& pin = "");
    
    void SetTarget(ElectricNodeBase* comp, const String& pin);
    
    // Measurement methods
    void RecordChange(byte value, int tick);
    double CalculateFrequency() const;  // Calculate frequency of signal changes
    double CalculatePeriod() const;     // Calculate period of signal changes
    int GetRiseTime(int threshold = 50) const;   // Rise time in simulation ticks
    int GetFallTime(int threshold = 50) const;   // Fall time in simulation ticks
    int GetPropagationDelay(const TimingProbe& other) const;  // Propagation delay to another probe
    
    // Analysis results
    void ReportTimingMeasurements() const;
    
    // Override base class methods
    bool Tick() override;
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
};

#endif