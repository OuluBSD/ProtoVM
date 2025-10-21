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
	
	//Port& GetPower() {return power;}
	
};




