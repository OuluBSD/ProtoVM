#pragma once

#include <queue>
#include <functional>



// Structure to represent a delayed event
struct DelayedEvent {
	int delay;  // Number of ticks to delay
	int original_tick;  // Simulation tick when event was scheduled
	std::function<bool()> action;  // Function to execute when delay expires
	bool operator>(const DelayedEvent& other) const {
		// Priority queue is max-heap by default, but we want min-heap behavior based on remaining delay
		// So we compare based on when the event should execute (original_tick + delay)
		return (original_tick + delay) > (other.original_tick + other.delay);
	}
};

class Machine {
public:
	Array<Pcb> pcbs;
	//Port power;
	LinkBaseMap l;
	
	// Delay queue for handling propagation delays
	std::priority_queue<DelayedEvent, std::vector<DelayedEvent>, std::greater<DelayedEvent>> delay_queue;
	int current_tick = 0;  // Current simulation tick
	
	// Timing violation tracking
	int timing_violations = 0;  // Count of timing violations detected
	
	// Topological ordering flag
	bool use_topological_ordering = false;  // Whether to use topological ordering for component evaluation
	
	bool Init();
	bool Tick();
	bool RunInitOps();
	bool RunRtOps();
	bool RunRtOpsWithChangeDetection(bool& changed);
	uint64 GetStateHash();
	bool IsStateInHistory(uint64 current_state, const Vector<uint64>& history);
	
	Pcb& AddPcb();
	
	// Methods for handling delayed events
	void ScheduleEvent(int delay, std::function<bool()> action);
	void ProcessDelayedEvents();
	
	// Methods for timing violation reporting
	void ReportTimingViolation(const String& component_name, const String& violation_details);
	int GetTimingViolationCount() const { return timing_violations; }
	void ResetTimingViolationCount() { timing_violations = 0; }
	
	// Method to check component timing constraints
	void CheckComponentTiming(ElectricNodeBase& component);
	
	// Methods for timing analysis
	void PerformTimingAnalysis();  // Overall timing analysis of the circuit
	void ReportTimingAnalysis();   // Report timing analysis results
	
	// Methods for topological sorting
	Vector<ElectricNodeBase*> PerformTopologicalSort();
	void BuildDependencyGraph();
	
	// Methods for clock domain management
	int CreateClockDomain(int frequency_hz = 0);  // Create a new clock domain with given frequency
	void AssignComponentToClockDomain(ElectricNodeBase* component, int domain_id);  // Assign component to a domain
	Vector<ElectricNodeBase*> GetComponentsInClockDomain(int domain_id);           // Get all components in a domain
	void CheckClockDomainCrossings();  // Check for signals crossing between clock domains

	// Enhanced clock domain management with simulation
	void SimulateClockDomains();        // Advance simulated clocks based on real time
	void ReportClockDomainInfo();       // Report information about all clock domains
	void SetGlobalClockMultiplier(double multiplier);  // Adjust all clock frequencies by a multiplier
	
	//Port& GetPower() {return power;}
	
	// Breakpoint functionality
private:
	Vector<int> breakpoints;  // List of tick numbers where simulation should pause
	bool simulation_paused = false;  // Whether simulation is currently paused
public:
	void AddBreakpoint(int tick_number);  // Add a breakpoint at specified tick
	void RemoveBreakpoint(int tick_number);  // Remove a breakpoint
	void ClearBreakpoints();  // Clear all breakpoints
	bool HasBreakpointAt(int tick_number) const;  // Check if there's a breakpoint at this tick
	bool IsPaused() const { return simulation_paused; }  // Check if simulation is paused
	void Resume() { simulation_paused = false; }  // Resume simulation

	// Performance profiling
private:
	bool profiling_enabled = false;
	int64 profiling_start_time = 0;
	int64 total_simulation_time = 0;  // Total time spent in simulation
	
	// Structure to track component performance
	struct ComponentProfile : Moveable<ComponentProfile> {
		String component_name;
		int64 total_time_spent;    // Total time spent in this component (in microseconds)
		int64 call_count;          // Number of times this component was processed
		int64 min_time;            // Minimum time for a single call
		int64 max_time;            // Maximum time for a single call
	};
	
	Vector<ComponentProfile> component_profiles;  // Profile data for each component
	int max_components_to_profile = 50;  // Maximum number of components to profile individually

	// Clock domain management
	struct ClockDomain : Moveable<ClockDomain> {
		int id;
		int frequency_hz;           // Frequency in Hertz
		double period_ticks;        // Period in simulation ticks (computed from frequency)
		int64 last_edge_tick;       // Simulation tick when the last clock edge occurred
		int64 next_edge_tick;       // Simulation tick when the next edge should occur
		bool clock_state;           // Current state of the clock
		// Keep track of component IDs instead of pointers to avoid copy issues
		Vector<int> component_ids;  // IDs of components in this domain
		
		ClockDomain() : id(0), frequency_hz(0), period_ticks(1.0), last_edge_tick(-1), next_edge_tick(0), clock_state(false) {}
		ClockDomain(const ClockDomain& other) : id(other.id), frequency_hz(other.frequency_hz), period_ticks(other.period_ticks),
			last_edge_tick(other.last_edge_tick), next_edge_tick(other.next_edge_tick), clock_state(other.clock_state),
			component_ids() {
			component_ids <<= other.component_ids;  // Use U++ deep copy operator
		}
		ClockDomain(ClockDomain&& other) = default;
		ClockDomain& operator=(const ClockDomain& other) {
			id = other.id; frequency_hz = other.frequency_hz; period_ticks = other.period_ticks;
			last_edge_tick = other.last_edge_tick; next_edge_tick = other.next_edge_tick; clock_state = other.clock_state;
			component_ids <<= other.component_ids;  // Use U++ deep copy operator
			return *this;
		}
		ClockDomain& operator=(ClockDomain&& other) = default;
	};
	
	Vector<ClockDomain> clock_domains;  // List of all clock domains
	double global_clock_multiplier = 1.0;  // Multiplier to adjust all clock frequencies

public:

	// Signal tracing functionality
private:
	// Structure to track signal changes
	struct SignalTrace : Moveable<SignalTrace> {
		ElectricNodeBase* component;
		String pin_name;
		byte last_value;
		Vector<byte> value_history;
		Vector<int> tick_history;
		bool trace_enabled;
		
		SignalTrace() : component(nullptr), last_value(0), trace_enabled(true) {}
		SignalTrace(const SignalTrace& other) : component(other.component), pin_name(other.pin_name), last_value(other.last_value),
			value_history(), tick_history(), trace_enabled(other.trace_enabled) {
			value_history <<= other.value_history;
			tick_history <<= other.tick_history;
		}
		SignalTrace(SignalTrace&& other) = default;
		SignalTrace& operator=(const SignalTrace& other) {
			component = other.component; pin_name = other.pin_name; last_value = other.last_value;
			value_history <<= other.value_history; tick_history <<= other.tick_history; trace_enabled = other.trace_enabled;
			return *this;
		}
		SignalTrace& operator=(SignalTrace&& other) = default;
	};
	Vector<SignalTrace> signal_traces;  // List of signals to trace

	// Signal transition logging
	struct SignalTransition : Moveable<SignalTransition> {
		String component_name;
		String pin_name;
		byte old_value;
		byte new_value;
		int tick_number;
		String timestamp;  // Optional timestamp info
		
		SignalTransition() : old_value(0), new_value(0), tick_number(0) {}
		SignalTransition(const SignalTransition& other) : component_name(other.component_name), pin_name(other.pin_name),
			old_value(other.old_value), new_value(other.new_value), tick_number(other.tick_number), timestamp(other.timestamp) {}
		SignalTransition(SignalTransition&& other) = default;
		SignalTransition& operator=(const SignalTransition& other) {
			component_name = other.component_name; pin_name = other.pin_name; old_value = other.old_value;
			new_value = other.new_value; tick_number = other.tick_number; timestamp = other.timestamp;
			return *this;
		}
		SignalTransition& operator=(SignalTransition&& other) = default;
	};
	Vector<SignalTransition> signal_transitions;  // Log of all signal transitions
	int max_transitions_to_store = 1000;  // Maximum transitions to keep in memory

public:
	// Methods for signal tracing
	void AddSignalToTrace(ElectricNodeBase* component, const String& pin_name);
	void RemoveSignalFromTrace(ElectricNodeBase* component, const String& pin_name);
	void ClearSignalTraces();
	void EnableSignalTrace(int trace_id, bool enable = true);
	void DisableSignalTrace(int trace_id);
	void LogSignalTraces();  // Log all traced signals
	const Vector<SignalTrace>& GetSignalTraces() const { return signal_traces; }

	// Methods for signal transition logging
	void LogSignalTransition(ElectricNodeBase* component, const String& pin_name, byte old_val, byte new_val);
	void LogAllSignalTransitions();  // Log all transitions in the current tick
	void ClearSignalTransitionLog(); // Clear the transition log
	int GetSignalTransitionCount() const { return signal_transitions.GetCount(); }
	const Vector<SignalTransition>& GetSignalTransitions() const { return signal_transitions; }
	void SetMaxTransitionLogSize(int max_size) { max_transitions_to_store = max_size; }

	// Methods for waveform generation
	void GenerateWaveformData();      // Generate waveform data for traced signals
	void ExportWaveformData(const String& filename); // Export waveform data to file
	String GenerateVCDFormat();       // Generate data in VCD (Value Change Dump) format
	void CreateWaveformForSignal(const String& component_name, const String& pin_name); // Create waveform for specific signal

	// Methods for performance profiling
	void StartProfiling();           // Start performance profiling
	void StopProfiling();            // Stop performance profiling
	void ReportProfilingResults();   // Report profiling results
	void ResetProfilingData();       // Reset all profiling data
	void AddProfilingSample(const String& component_name, int64 duration); // Add a profiling sample
};




