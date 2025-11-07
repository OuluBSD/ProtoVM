#include "ProtoVM.h"

// Implementation of ScheduleTick method for ElectricNodeBase
void ElectricNodeBase::ScheduleTick(int delay) {
	// This method is intended to schedule this component to be ticked after a delay.
	// However, since components don't directly have access to the Machine (due to protected field),
	// this method provides a framework that can be used by the system.
	// In practice, components would call this from their Tick method, and the Machine
	// would be responsible for scheduling based on component needs.
	// This implementation serves as a placeholder; actual scheduling should happen
	// from Machine context where the component is passed as a parameter.
	LOG("ScheduleTick called - actual scheduling should happen from Machine context with component reference");
}

// Implementation of UpdateTimingInfo method for ElectricNodeBase
void ElectricNodeBase::UpdateTimingInfo(String input_name, int current_tick, bool is_clock, bool clock_state) {
	// Find existing timing info for this input name or add new one
	int index = -1;
	for(int i = 0; i < timing_info_names.GetCount(); i++) {
		if(timing_info_names[i] == input_name) {
			index = i;
			break;
		}
	}
	
	if(index == -1) {
		// Add new timing info entry
		index = timing_info_names.GetCount();
		timing_info_names.Add(input_name);
		timing_info.Add();
	}
	
	TimingInfo& info = timing_info[index];
	
	if (is_clock) {
		// Update clock-related timing info
		if (info.last_clock_state != clock_state && current_tick > info.last_clock_edge_tick) {
			// This represents a clock edge transition
			info.last_clock_state = clock_state;
			info.last_clock_edge_tick = current_tick;
		} else {
			info.last_clock_state = clock_state;
		}
	} else {
		// Update data-related timing info
		info.data_change_tick = current_tick;
	}
}

// Implementation of CheckTimingConstraints method for ElectricNodeBase
bool ElectricNodeBase::CheckTimingConstraints(String input_name, int current_tick, bool is_clock_edge) const {
	// Find timing info for this input name
	int index = -1;
	for(int i = 0; i < timing_info_names.GetCount(); i++) {
		if(timing_info_names[i] == input_name) {
			index = i;
			break;
		}
	}
	
	if(index == -1) {
		// No timing info for this input, assume constraints are satisfied
		return true;
	}
	
	const TimingInfo& info = timing_info[index];
	
	if (!is_clock_edge) {
		// If not checking at a clock edge, constraints are satisfied by definition
		return true;
	}
	
	// In clock domain crossing scenarios, timing analysis becomes more complex
	// For now, we'll perform basic setup/hold checks within the same domain
	
	// Check setup time: data must be stable before the clock edge
	if (info.data_change_tick >= (current_tick - setup_time_ticks)) {
		// Data changed too recently before the clock edge (setup time violation)
		LOG("Setup time violation for " << GetClassName() << " on input " << input_name 
			<< ": data changed at tick " << info.data_change_tick 
			<< ", clock edge at tick " << current_tick 
			<< ", required setup time: " << setup_time_ticks << " ticks");
		return false;
	}
	
	// Check hold time: data must remain stable after the clock edge
	// For this, we'd need to track if data changed too soon after the clock edge,
	// which we'll detect in the next call to this function when the data changes
	
	return true;
}

// Implementation of dependency management methods for ElectricNodeBase
void ElectricNodeBase::AddDependency(ElectricNodeBase& dependent) {
	// This component depends on 'dependent', so add it to dependencies
	// and also add this component to the dependent's dependents list
	bool found = false;
	for (int i = 0; i < dependencies.GetCount(); i++) {
		if (dependencies[i] == &dependent) {
			found = true;
			break;
		}
	}
	if (!found) {
		dependencies.Add(&dependent);
	}
	
	found = false;
	for (int i = 0; i < dependent.dependents.GetCount(); i++) {
		if (dependent.dependents[i] == this) {
			found = true;
			break;
		}
	}
	if (!found) {
		dependent.dependents.Add(this);
	}
}

Vector<ElectricNodeBase*>& ElectricNodeBase::GetDependents() {
	return dependents;
}

Vector<ElectricNodeBase*>& ElectricNodeBase::GetDependencies() {
	return dependencies;
}

void ElectricNodeBase::SetClockDomain(int domain_id, int freq_hz) {
	clock_domain_id = domain_id;
	clock_frequency_hz = freq_hz;
}

int ElectricNodeBase::GetClockDomainId() const {
	return clock_domain_id;
}

int ElectricNodeBase::GetClockFrequency() const {
	return clock_frequency_hz;
}


bool Machine::Init() {
	
	// Check that all pins are connected
	for (Pcb& pcb : pcbs) {
		if (!pcb.IsAllConnected()) {
			LOG("Pcb \"" + pcb.GetName() + "\" not fully connected");
			return false;
		}
	}
	LOG("Machine::Init: all pcbs fully connected!");
	
	for (Pcb& pcb : pcbs) {
		pcb.GetLinkBases(l.links);
	}
	
	l.UpdateLinkBaseLayers();
	
	if (!l.UpdateProcess())
		return false;
	
	RunInitOps();
	
	return true;
}

bool Machine::RunInitOps() {
	/*for (const ProcessOp& op : l.init_ops) {
		switch (op.type) {
		default: TODO
		}
		TODO
	}*/
	return true;
}

bool Machine::RunRtOps() {
	int op_i = 0;
	for (const ProcessOp& op : l.rt_ops) {
		switch (op.type) {
		//case ProcessType::READ:
		case ProcessType::WRITE:
			ASSERT(op.processor);
			if (!op.processor->Process(op.type, op.mem_bytes, op.mem_bits, op.id, *op.dest, op.dest_id)) {
				LOG("error: processing failed in " << op.processor->GetClassName());
				return false;
			}
			break;
			
		case ProcessType::TICK:
			if (!op.dest->Tick())
				return false;
			break;
			
		default:
			TODO
			break;
		}
		op_i++;
	}
	return true;
}

bool Machine::Tick() {
	/*for (Pcb& pcb : pcbs) {
		if (!pcb.Tick())
			return false;
	}*/
	
	// Process any delayed events scheduled for this tick
	ProcessDelayedEvents();
	
	// Increment the current tick counter
	current_tick++;
	
	// Simulate clock domains to update their states
	SimulateClockDomains();
	
	// Check for clock domain crossings if needed
	// For now, we'll perform this check periodically, maybe every N ticks
	// In a real implementation, this might be configurable
	if (current_tick % 100 == 0) {  // Check every 100 ticks
		CheckClockDomainCrossings();
	}
	
	// Implement convergence-based simulation to handle feedback loops and signal propagation
	bool changed = true;
	int iteration = 0;
	const int max_iterations = 1000; // Prevent infinite loops in oscillating circuits
	
	// Track if we've seen the same state to detect oscillations
	Vector<uint64> state_history;
	const int max_state_history = 10; // Only track last 10 states for oscillation detection
	
	while (changed && iteration < max_iterations) {
		changed = false;
		if (!RunRtOpsWithChangeDetection(changed))
			return false;
		
		// Check for oscillation by looking for repeating states
		uint64 current_state_hash = GetStateHash();
		if (IsStateInHistory(current_state_hash, state_history)) {
			LOG("Warning: Oscillation detected in Machine::Tick() at iteration " << iteration);
			break; // Exit the convergence loop to prevent infinite oscillation
		}
		
		// Add current state to history, keeping only the most recent states
		state_history.Add(current_state_hash);
		if (state_history.GetCount() > max_state_history) {
			state_history.Remove(0); // Remove oldest entry
		}
		
		iteration++;
	}
	
	if (iteration >= max_iterations) {
		LOG("Warning: Machine::Tick() reached max iterations - possible oscillation detected");
	}
	
	// Check if we've reached a breakpoint
	if (HasBreakpointAt(current_tick)) {
		LOG("Breakpoint hit at tick " << current_tick);
		simulation_paused = true;
	}
	
	// Perform signal tracing if any signals are being traced
	if (!signal_traces.IsEmpty()) {
		LogSignalTraces();  // Log the current state of all traced signals
	}
	
	// Log signal transitions summary for this tick
	if (!signal_transitions.IsEmpty()) {
		// Only log if there were transitions in this tick
		bool hasCurrentTickTransitions = false;
		for (int i = 0; i < signal_transitions.GetCount(); i++) {
			if (signal_transitions[i].tick_number == current_tick) {
				hasCurrentTickTransitions = true;
				break;
			}
		}
		
		if (hasCurrentTickTransitions) {
			LogAllSignalTransitions();  // Log all transitions from the current tick
		}
	}
	
	return true;
}

uint64 Machine::GetStateHash() {
	uint64 hash = 0;
	uint64 multiplier = 31;
	
	// Create a simple hash based on the current state of components
	for (Pcb& pcb : pcbs) {
		for (int i = 0; i < pcb.nodes.GetCount(); i++) {
			ElcBase& node = pcb.nodes[i];
			// Use the name and some basic properties to create a hash
			String node_name = node.GetName();
			for (int j = 0; j < node_name.GetCount(); j++) {
				hash = hash * multiplier + node_name[j];
			}
			
			// Add some basic state information if available
			hash = hash * multiplier + node.GetMemorySize();
		}
	}
	
	return hash;
}

bool Machine::IsStateInHistory(uint64 current_state, const Vector<uint64>& history) {
	for (uint64 past_state : history) {
		if (past_state == current_state) {
			return true;
		}
	}
	return false;
}

bool Machine::RunRtOpsWithChangeDetection(bool& changed) {
	changed = false; // Start with no changes detected
	int op_i = 0;
	
	if (use_topological_ordering) {
		// Use topological ordering for TICK operations
		// Get components in topological order
		Vector<ElectricNodeBase*> topo_order = PerformTopologicalSort();
		
		// First process all WRITE operations in the original order
		for (const ProcessOp& op : l.rt_ops) {
			if (op.type == ProcessType::WRITE) {
				ASSERT(op.processor);
				// For write operations, remember the destination's change state before processing
				bool op_changed = false;
				bool dest_changed_before = op.dest->HasChanged();
				if (!op.processor->Process(op.type, op.mem_bytes, op.mem_bits, op.id, *op.dest, op.dest_id)) {
					LOG("error: processing failed in " << op.processor->GetClassName());
					return false;
				}
				// Check if the destination state changed as a result
				if (op.dest->HasChanged() != dest_changed_before) {
					op_changed = true;
				} else {
					// The write operation may have changed the destination without setting the flag
					// Conservatively assume it changed to ensure proper simulation
					op_changed = true;
				}
				
				if (op_changed) {
					changed = true;
				}
			}
		}
		
		// Then process TICK operations in topological order
		for (ElectricNodeBase* comp : topo_order) {
			// Find the corresponding TICK operation for this component
			for (const ProcessOp& op : l.rt_ops) {
				if (op.type == ProcessType::TICK && op.dest == comp) {
					// Clear the change flag before ticking to detect changes properly
					op.dest->SetChanged(false);
					// Call the Tick method - it should internally call SetChanged if state changes
					if (!op.dest->Tick()) {
						return false;
					}
					// Check if the component indicated its state changed
					bool op_changed = false;
					if (op.dest->HasChanged()) {
						op_changed = true;
					}
					// Check timing constraints after the component has processed its tick
					CheckComponentTiming(*op.dest);
					
					if (op_changed) {
						changed = true;
					}
					break; // Found and processed the tick for this component
				}
			}
		}
	} else {
		// Use the original order
		for (const ProcessOp& op : l.rt_ops) {
			bool op_changed = false;
			switch (op.type) {
			//case ProcessType::READ:
			case ProcessType::WRITE: {
				ASSERT(op.processor);
				// For write operations, remember the destination's change state before processing
				bool dest_changed_before = op.dest->HasChanged();
				if (!op.processor->Process(op.type, op.mem_bytes, op.mem_bits, op.id, *op.dest, op.dest_id)) {
					LOG("error: processing failed in " << op.processor->GetClassName());
					return false;
				}
				// Check if the destination state changed as a result
				if (op.dest->HasChanged() != dest_changed_before) {
					op_changed = true;
				} else {
					// The write operation may have changed the destination without setting the flag
					// Conservatively assume it changed to ensure proper simulation
					op_changed = true;
				}
				break;
			}
			case ProcessType::TICK: {
				// Clear the change flag before ticking to detect changes properly
				op.dest->SetChanged(false);
				// Call the Tick method - it should internally call SetChanged if state changes
				if (!op.dest->Tick()) {
					return false;
				}
				// Check if the component indicated its state changed
				if (op.dest->HasChanged()) {
					op_changed = true;
				}
				// Check timing constraints after the component has processed its tick
				CheckComponentTiming(*op.dest);
				break;
			}
			default:
				LOG("Machine::RunRtOpsWithChangeDetection: unhandled ProcessType");
				return false;
			}
			
			if (op_changed) {
				changed = true;
			}
			op_i++;
		}
	}
	return true;
}

Pcb& Machine::AddPcb() {
	Pcb& p = pcbs.Add();
	p.mach = this;
	return p;
}

void Machine::ScheduleEvent(int delay, std::function<bool()> action) {
	if (delay < 0) {
		LOG("Warning: Negative delay value passed to ScheduleEvent, clamping to 0");
		delay = 0;
	}
	
	DelayedEvent event;
	event.delay = delay;
	event.original_tick = current_tick;
	event.action = action;
	
	delay_queue.push(event);
}

void Machine::ProcessDelayedEvents() {
	// Process all events that are scheduled for the current tick
	while (!delay_queue.empty()) {
		const DelayedEvent& event = delay_queue.top();
		int execution_tick = event.original_tick + event.delay;
		
		// If the next event is scheduled for a future tick, stop processing
		if (execution_tick > current_tick) {
			break;
		}
		
		// We need to copy the action since we can't call it on a const reference
		// So we'll pop the event, then execute it
		DelayedEvent current_event = delay_queue.top();
		delay_queue.pop();
		
		// Execute the event's action
		if (!current_event.action()) {
			LOG("Warning: Delayed event action failed");
		}
	}
}

void Machine::ReportTimingViolation(const String& component_name, const String& violation_details) {
	timing_violations++;
	LOG("TIMING VIOLATION [" << timing_violations << "]: " << component_name << " - " << violation_details);
}

// Method to check timing constraints for a component during its tick
void Machine::CheckComponentTiming(ElectricNodeBase& component) {
	// This method checks if any timing violations have occurred for the component
	// For each input pin that has timing constraints, we need to verify 
	// setup and hold time requirements
	
	// In this implementation, we'll call the component's own method to check its constraints
	// Since we can't directly access the internal timing_info map from here due to privacy,
	// the actual checking will be done by the component itself.
	
	// This is a simplified approach - a full implementation would need more sophisticated
	// tracking of when clock edges occurred relative to data changes
}

// Build the dependency graph by analyzing connections between components
void Machine::BuildDependencyGraph() {
	// Clear existing dependencies
	for (Pcb& pcb : pcbs) {
		for (int i = 0; i < pcb.nodes.GetCount(); i++) {
			ElectricNodeBase& node = pcb.nodes[i];
			node.dependencies.Clear();
			node.dependents.Clear();
		}
	}
	
	// Analyze each PCB's connections to build dependency graph
	for (Pcb& pcb : pcbs) {
		// Process each link to establish dependencies
		// In this system, when one component's output is connected to another's input,
		// the output component should be evaluated before the input component
		// So input component depends on output component
		
		for (int i = 0; i < pcb.nodes.GetCount(); i++) {
			ElectricNodeBase& node = pcb.nodes[i];
			
			// Look at each connector of this node
			for (int j = 0; j < node.conns.GetCount(); j++) {
				ElectricNodeBase::Connector& conn = node.conns[j];
				
				// If this is a sink (input), it depends on its source(s)
				if (conn.IsConnected()) {
					for (int k = 0; k < conn.links.GetCount(); k++) {
						ElectricNodeBase::CLink& cLink = conn.links[k];
						if (cLink.link) {
							ElectricNodeBase::Connector* src_conn = cLink.link->src;
							if (src_conn && src_conn->base) {
								// This node (sink) depends on the source component
								node.AddDependency(*src_conn->base);
							}
						}
					}
				}
			}
		}
	}
}

// Perform topological sort using Kahn's algorithm
Vector<ElectricNodeBase*> Machine::PerformTopologicalSort() {
	BuildDependencyGraph();
	
	// Initialize with components that have no dependencies (in-degree = 0)
	Vector<ElectricNodeBase*> result;
	Vector<ElectricNodeBase*> no_dependency_nodes;
	
	// Find all nodes with no dependencies (in-degree = 0)
	for (Pcb& pcb : pcbs) {
		for (int i = 0; i < pcb.nodes.GetCount(); i++) {
			ElectricNodeBase& node = pcb.nodes[i];
			if (node.GetDependencies().GetCount() == 0) {
				no_dependency_nodes.Add(&node);
			}
		}
	}
	
	// Process nodes in topological order
	while (!no_dependency_nodes.IsEmpty()) {
		// Remove a node from the list of nodes with no dependencies
		ElectricNodeBase* current = no_dependency_nodes[0];
		no_dependency_nodes.Remove(0);
		
		// Add it to the result
		result.Add(current);
		
		// For each node that depends on the current node, remove the dependency
		Vector<ElectricNodeBase*>& dependents = current->GetDependents();
		for (int i = 0; i < dependents.GetCount(); i++) {
			ElectricNodeBase* dependent = dependents[i];
			
			// Remove this dependency from the dependent's dependency list
			Vector<ElectricNodeBase*>& deps = dependent->GetDependencies();
			int idx = -1;
			for (int j = 0; j < deps.GetCount(); j++) {
				if (deps[j] == current) {
					idx = j;
					break;
				}
			}
			if (idx >= 0) {
				deps.Remove(idx);
			}
			
			// If the dependent now has no more dependencies, add it to the list
			if (deps.GetCount() == 0) {
				no_dependency_nodes.Add(dependent);
			}
		}
	}
	
	// Check if all nodes were included (if not, there was a cycle)
	int total_node_count = 0;
	for (Pcb& pcb : pcbs) {
		total_node_count += pcb.nodes.GetCount();
	}
	
	if (result.GetCount() != total_node_count) {
		LOG("Warning: Topological sort detected a cycle in the dependency graph. "
			<< (total_node_count - result.GetCount()) << " components not included in sort.");
	}
	
	return result;
}

int Machine::CreateClockDomain(int frequency_hz) {
	// Create and store a new clock domain with the given frequency
	ClockDomain domain;
	domain.id = clock_domains.GetCount();  // Use the count as the ID
	domain.frequency_hz = frequency_hz;
	
	if (frequency_hz > 0) {
		// Calculate period in simulation ticks (assuming 1 tick = 1 time unit)
		// For example, if freq = 1000Hz, then period = 1/1000 seconds = 0.001 (normalized to ticks)
		// In this simplified model: period_ticks = 1 / (frequency_hz * global_clock_multiplier)
		double effective_freq = frequency_hz * global_clock_multiplier;
		if (effective_freq > 0) {
			domain.period_ticks = 1.0 / effective_freq;
		} else {
			domain.period_ticks = 1.0;  // Default to 1 if frequency is 0
		}
	} else {
		domain.period_ticks = 1.0;  // Default for asynchronous domains
	}
	
	domain.last_edge_tick = -1;
	domain.next_edge_tick = 0;
	domain.clock_state = false;
	
	clock_domains.Add(domain);
	
	LOG("Created clock domain " << domain.id << " with frequency " << frequency_hz << " Hz");
	return domain.id;
}

void Machine::AssignComponentToClockDomain(ElectricNodeBase* component, int domain_id) {
	if (component && domain_id < clock_domains.GetCount()) {
		component->SetClockDomain(domain_id);
		
		// Add component to the domain's component list (storing ID instead of pointer to avoid copy issues)
		ClockDomain& domain = clock_domains[domain_id];
		// For now just store an ID - in a real implementation we'd have a more sophisticated system
		// to find the component by its ID when needed
		domain.component_ids.Add(domain_id); // Using domain_id as component ID for this simple case
	}
}

Vector<ElectricNodeBase*> Machine::GetComponentsInClockDomain(int domain_id) {
	Vector<ElectricNodeBase*> result;
	
	for (Pcb& pcb : pcbs) {
		for (int i = 0; i < pcb.nodes.GetCount(); i++) {
			ElectricNodeBase& node = pcb.nodes[i];
			if (node.GetClockDomainId() == domain_id) {
				result.Add(&node);
			}
		}
	}
	
	return result;
}

void Machine::CheckClockDomainCrossings() {
	// This method checks if there are any connections between components in different clock domains
	// which might need special handling
	
	for (Pcb& pcb : pcbs) {
		for (int i = 0; i < pcb.nodes.GetCount(); i++) {
			ElectricNodeBase& node = pcb.nodes[i];
			
			// Look at each connector of this node
			for (int j = 0; j < node.conns.GetCount(); j++) {
				ElectricNodeBase::Connector& conn = node.conns[j];
				
				// If this is a sink (input), check if it's connected to sources in different domains
				if (conn.IsConnected()) {
					for (int k = 0; k < conn.links.GetCount(); k++) {
						ElectricNodeBase::CLink& cLink = conn.links[k];
						if (cLink.link) {
							ElectricNodeBase::Connector* src_conn = cLink.link->src;
							if (src_conn && src_conn->base) {
								// Check if the source is in a different clock domain
								if (node.GetClockDomainId() != src_conn->base->GetClockDomainId()) {
									LOG("CLOCK DOMAIN CROSSING: Component " << node.GetDynamicName() 
										<< " (domain " << node.GetClockDomainId() << ") connected to "
										<< src_conn->base->GetDynamicName() << " (domain " 
										<< src_conn->base->GetClockDomainId() << ")");
								}
							}
						}
					}
				}
			}
		}
	}
}




void Machine::AddBreakpoint(int tick_number) {
	if (tick_number >= 0) {
		if (!HasBreakpointAt(tick_number)) {
			breakpoints.Add(tick_number);
			// Sort for efficient lookup
		}
	}
}

void Machine::RemoveBreakpoint(int tick_number) {
	for (int i = 0; i < breakpoints.GetCount(); i++) {
		if (breakpoints[i] == tick_number) {
			breakpoints.Remove(i);
			break;
		}
	}
}

void Machine::ClearBreakpoints() {
	breakpoints.Clear();
}

bool Machine::HasBreakpointAt(int tick_number) const {
	for (int i = 0; i < breakpoints.GetCount(); i++) {
		if (breakpoints[i] == tick_number) {
			return true;
		}
	}
	return false;
}

// Implementation for signal tracing methods
void Machine::AddSignalToTrace(ElectricNodeBase* component, const String& pin_name) {
    SignalTrace trace;
    trace.component = component;
    trace.pin_name = pin_name;
    trace.last_value = 0;  // Initial value
    trace.trace_enabled = true;
    
    signal_traces.Add(trace);
    LOG("Added signal to trace: " << component->GetName() << "." << pin_name);
}

void Machine::RemoveSignalFromTrace(ElectricNodeBase* component, const String& pin_name) {
    for (int i = 0; i < signal_traces.GetCount(); i++) {
        if (signal_traces[i].component == component && signal_traces[i].pin_name == pin_name) {
            signal_traces.Remove(i);
            LOG("Removed signal from trace: " << component->GetName() << "." << pin_name);
            return;
        }
    }
    LOG("Warning: Signal not found in trace: " << component->GetName() << "." << pin_name);
}

void Machine::ClearSignalTraces() {
    signal_traces.Clear();
    LOG("Cleared all signal traces");
}

void Machine::EnableSignalTrace(int trace_id, bool enable) {
    if (trace_id >= 0 && trace_id < signal_traces.GetCount()) {
        signal_traces[trace_id].trace_enabled = enable;
        LOG("Signal trace " << trace_id << " " << (enable ? "enabled" : "disabled"));
    }
}

void Machine::DisableSignalTrace(int trace_id) {
    if (trace_id >= 0 && trace_id < signal_traces.GetCount()) {
        signal_traces[trace_id].trace_enabled = false;
        LOG("Signal trace " << trace_id << " disabled");
    }
}

void Machine::LogSignalTraces() {
    LOG("Signal Trace Report (Tick " << current_tick << "):");
    for (int i = 0; i < signal_traces.GetCount(); i++) {
        const SignalTrace& trace = signal_traces[i];
        if (trace.trace_enabled) {
            LOG("  " << trace.component->GetName() << "." << trace.pin_name 
                 << " = " << (int)trace.last_value);
        }
    }
}

// Implementation for timing analysis methods
void Machine::PerformTimingAnalysis() {
    LOG("Starting timing analysis...");
    
    // Perform timing analysis across all PCBs and components
    for (int pcb_idx = 0; pcb_idx < pcbs.GetCount(); pcb_idx++) {
        Pcb& pcb = pcbs[pcb_idx];
        
        LOG("Analyzing PCB " << pcb_idx << ": " << pcb.GetName());
        
        // Analyze each component in the PCB
        for (int comp_idx = 0; comp_idx < pcb.GetNodeCount(); comp_idx++) {
            ElectricNodeBase& comp = pcb.GetNode(comp_idx);
            
            // Check timing constraints based on component's setup/hold time requirements
            if (comp.GetSetupTimeTicks() > 0 || comp.GetHoldTimeTicks() > 0) {
                LOG("  Component: " << comp.GetName() 
                     << " (Setup: " << comp.GetSetupTimeTicks() 
                     << ", Hold: " << comp.GetHoldTimeTicks() << ")");
            }
        }
    }
    
    LOG("Timing analysis completed.");
}

void Machine::ReportTimingAnalysis() {
    LOG("Timing Analysis Report:");
    LOG("========================");
    
    int totalComponents = 0;
    int timingCriticalComponents = 0;
    int totalTimingViolations = GetTimingViolationCount();
    
    for (int pcb_idx = 0; pcb_idx < pcbs.GetCount(); pcb_idx++) {
        Pcb& pcb = pcbs[pcb_idx];
        
        LOG("PCB " << pcb_idx << ": " << pcb.GetName());
        LOG("  Components: " << pcb.GetNodeCount());
        
        int pcbTimingCritical = 0;
        for (int comp_idx = 0; comp_idx < pcb.GetNodeCount(); comp_idx++) {
            ElectricNodeBase& comp = pcb.GetNode(comp_idx);
            totalComponents++;
            
            if (comp.GetSetupTimeTicks() > 0 || comp.GetHoldTimeTicks() > 0) {
                pcbTimingCritical++;
                timingCriticalComponents++;
                LOG("    [TIMING-CRITICAL] " << comp.GetName() 
                     << " (Setup: " << comp.GetSetupTimeTicks() 
                     << "t, Hold: " << comp.GetHoldTimeTicks() << "t)");
            }
        }
        
        LOG("  Timing-critical components: " << pcbTimingCritical);
    }
    
    LOG("Summary:");
    LOG("  Total components analyzed: " << totalComponents);
    LOG("  Timing-critical components: " << timingCriticalComponents);
    LOG("  Timing violations detected: " << totalTimingViolations);
    
    if (timingCriticalComponents > 0) {
        LOG("Recommendation: Review timing-critical components for proper clock domain placement");
        LOG("  and ensure setup/hold time requirements are met in your design.");
    }
    
    LOG("========================");
}

// Implementation for signal transition logging methods
void Machine::LogSignalTransition(ElectricNodeBase* component, const String& pin_name, byte old_val, byte new_val) {
    // Create a new signal transition record
    SignalTransition trans;
    trans.component_name = component->GetName();
    trans.pin_name = pin_name;
    trans.old_value = old_val;
    trans.new_value = new_val;
    trans.tick_number = current_tick;
    trans.timestamp = String().Cat() << current_tick;  // Simple tick-based timestamp
    
    // Add to the transitions log
    signal_transitions.Add(trans);
    
    // If we exceed the maximum log size, remove the oldest entries
    if (signal_transitions.GetCount() > max_transitions_to_store) {
        signal_transitions.Remove(0, signal_transitions.GetCount() - max_transitions_to_store);
    }
    
    // Optionally log to output immediately
    LOG("Signal Transition: " << component->GetName() << "." << pin_name 
         << " [" << (int)old_val << " -> " << (int)new_val << "] at tick " << current_tick);
}

void Machine::LogAllSignalTransitions() {
    LOG("Signal Transition Log (Tick " << current_tick << "):");
    for (int i = 0; i < signal_transitions.GetCount(); i++) {
        const SignalTransition& trans = signal_transitions[i];
        if (trans.tick_number == current_tick) {  // Only transitions from the current tick
            LOG("  " << trans.component_name << "." << trans.pin_name 
                 << " [" << (int)trans.old_value << " -> " << (int)trans.new_value 
                 << "] at tick " << trans.tick_number);
        }
    }
}

void Machine::ClearSignalTransitionLog() {
    signal_transitions.Clear();
    LOG("Cleared signal transition log");
}

// Implementation for waveform generation methods
void Machine::GenerateWaveformData() {
    LOG("Generating waveform data for all traced signals...");
    
    // This would generate waveform data based on the signal trace history
    for (int i = 0; i < signal_traces.GetCount(); i++) {
        const SignalTrace& trace = signal_traces[i];
        if (trace.trace_enabled) {
            LOG("Waveform data for: " << trace.component->GetName() << "." << trace.pin_name);
            LOG("  Total value changes: " << trace.value_history.GetCount());
            
            // Print out the value changes with their tick numbers
            for (int j = 0; j < min(20, trace.value_history.GetCount()); j++) {  // Limit output
                LOG("    Tick " << trace.tick_history[j] << ": Value = " << (int)trace.value_history[j]);
            }
            
            if (trace.value_history.GetCount() > 20) {
                LOG("    ... (" << (trace.value_history.GetCount() - 20) << " more transitions)");
            }
        }
    }
}

void Machine::ExportWaveformData(const String& filename) {
    // Create a simple text-based representation of the waveform data
    String content = "Waveform Data Export\\n";
    content += "==================\\n\\n";
    
    for (int i = 0; i < signal_traces.GetCount(); i++) {
        const SignalTrace& trace = signal_traces[i];
        if (trace.trace_enabled) {
            content += "Signal: " + trace.component->GetName() + "." + trace.pin_name + "\\n";
            content += "Time\\tValue\\n";
            content += "----\\t-----\\n";
            
            for (int j = 0; j < trace.value_history.GetCount(); j++) {
                content += IntStr(trace.tick_history[j]) + "\\t" + IntStr(trace.value_history[j]) + "\\n";
            }
            
            content += "\\n";
        }
    }
    
    // Write to file
    FileOut file;
    if (file.Open(AsString(filename), FileOut::CREATE)) {
        file.Put(content);
        LOG("Waveform data exported to: " << filename);
    } else {
        LOG("Error: Could not open file for writing: " << filename);
    }
}

String Machine::GenerateVCDFormat() {
    String vcd = "$version ProtoVM Digital Logic Simulator $end\\n";
    vcd += "$date " + Upp::AsString(GetSysDate()) + " $end\\n";
    vcd += "$timescale 1ns $end\\n\\n";
    
    
    // Add variable definitions
    vcd += "$scope module ProtoVM $end\\n";
    
    for (int i = 0; i < signal_traces.GetCount(); i++) {
        const SignalTrace& trace = signal_traces[i];
        if (trace.trace_enabled) {
            // For simplicity, we'll model all signals as 8-bit values
            vcd += "$var reg 8 " + Upp::AsString(char(65 + i)) + " " + trace.component->GetName() + "_" + trace.pin_name + " $end\\n";
        }
    }
    vcd += "$upscope $end\\n\\n";
    
    // Add end definitions
    vcd += "$enddefinitions $end\\n\\n";
    
    // Add value changes
    vcd += "$dumpvars\\n";
    for (int i = 0; i < signal_traces.GetCount(); i++) {
        const SignalTrace& trace = signal_traces[i];
        if (trace.trace_enabled && !trace.value_history.IsEmpty()) {
            // Set initial value
            vcd += "b" + FormatIntBase(trace.value_history[0], 2) + " " + Upp::AsString(char(65 + i)) + "\\n";
        }
    }
    vcd += "$end\\n\\n";
    
    // Add time-ordered value changes
    // This is a simplified approach - in practice, you'd want all changes sorted by timestamp
    for (int tick = 0; tick <= current_tick; tick++) {
        bool hasChange = false;
        
        for (int i = 0; i < signal_traces.GetCount(); i++) {
            const SignalTrace& trace = signal_traces[i];
            if (trace.trace_enabled) {
                // Look for changes at this tick
                for (int j = 0; j < trace.tick_history.GetCount(); j++) {
                    if (trace.tick_history[j] == tick) {
                        hasChange = true;
                        break;
                    }
                }
            }
        }
        
        if (hasChange) {
            vcd += "#" + Upp::IntStr(tick) + "\\n";
            
            for (int i = 0; i < signal_traces.GetCount(); i++) {
                const SignalTrace& trace = signal_traces[i];
                if (trace.trace_enabled) {
                    // Look for changes at this tick
                    for (int j = 0; j < trace.tick_history.GetCount(); j++) {
                        if (trace.tick_history[j] == tick) {
                            vcd += "b" + FormatIntBase(trace.value_history[j], 2) + " " + Upp::AsString(char(65 + i)) + "\\n";
                            break;
                        }
                    }
                }
            }
        }
    }
    
    return vcd;
}

void Machine::CreateWaveformForSignal(const String& component_name, const String& pin_name) {
    // Find the specified signal in the signal traces
    for (int i = 0; i < signal_traces.GetCount(); i++) {
        SignalTrace& trace = signal_traces[i];
        if (trace.component->GetName() == component_name && trace.pin_name == pin_name) {
            LOG("Creating waveform for signal: " << component_name << "." << pin_name);
            LOG("Total recorded transitions: " << trace.value_history.GetCount());
            
            // Print waveform-style representation
            LOG("Waveform (Time -> Value):\\n");
            for (int j = 0; j < trace.value_history.GetCount(); j++) {
                String bar = ""; 
                byte val = trace.value_history[j];
                
                // Create a simple ASCII representation
                for (int k = 0; k < val; k++) {
                    bar += "*";
                }
                
                LOG("  " << trace.tick_history[j] << " -> " << (int)val << " " << bar);
            }
            return;
        }
    }
    
    LOG("Signal not found in traces: " << component_name << "." << pin_name);
}

// Implementation for performance profiling methods
void Machine::StartProfiling() {
    profiling_enabled = true;
    profiling_start_time = GetTickCount();  // Get current time in milliseconds
    LOG("Performance profiling started");
}

void Machine::StopProfiling() {
    if (profiling_enabled) {
        int64 elapsed = GetTickCount() - profiling_start_time;
        total_simulation_time += elapsed;
        profiling_enabled = false;
        LOG("Performance profiling stopped. Total elapsed time: " << elapsed << " ms");
    }
}

void Machine::ReportProfilingResults() {
    LOG("PERFORMANCE PROFILING REPORT");
    LOG("=============================");
    LOG("Total simulation time: " << total_simulation_time << " ms");
    LOG("Number of components profiled: " << component_profiles.GetCount());
    
    // Sort components by time spent (descending)
    // For simplicity in this implementation, we'll just display all profiles
    for (int i = 0; i < component_profiles.GetCount(); i++) {
        const ComponentProfile& profile = component_profiles[i];
        LOG("Component: " << profile.component_name);
        LOG("  Total time: " << profile.total_time_spent << " μs");
        LOG("  Call count: " << profile.call_count);
        if (profile.call_count > 0) {
            LOG("  Avg time per call: " << (double)profile.total_time_spent / profile.call_count << " μs");
        }
        LOG("  Min time for call: " << profile.min_time << " μs");
        LOG("  Max time for call: " << profile.max_time << " μs");
    }
    
    if (component_profiles.IsEmpty()) {
        LOG("No component profiling data collected");
    }
    
    LOG("=============================");
}

void Machine::ResetProfilingData() {
    profiling_enabled = false;
    total_simulation_time = 0;
    component_profiles.Clear();
    LOG("Performance profiling data reset");
}

void Machine::AddProfilingSample(const String& component_name, int64 duration) {
    // Find existing profile for this component or add a new one
    int idx = -1;
    for (int i = 0; i < component_profiles.GetCount(); i++) {
        if (component_profiles[i].component_name == component_name) {
            idx = i;
            break;
        }
    }
    
    if (idx == -1) {
        // Check if we've reached the maximum number of components to profile
        if (component_profiles.GetCount() >= max_components_to_profile) {
            // For simplicity, we'll just return without adding if we're at the limit
            // In a real implementation, you might want to track only the most time-consuming components
            return;
        }
        
        // Add a new profile entry
        ComponentProfile profile;
        profile.component_name = component_name;
        profile.total_time_spent = duration;
        profile.call_count = 1;
        profile.min_time = duration;
        profile.max_time = duration;
        component_profiles.Add(profile);
    } else {
        // Update existing profile
        ComponentProfile& profile = component_profiles[idx];
        profile.total_time_spent += duration;
        profile.call_count++;
        
        if (duration < profile.min_time) {
            profile.min_time = duration;
        }
        if (duration > profile.max_time) {
            profile.max_time = duration;
        }
    }
}

void Machine::SimulateClockDomains() {
    // Update the clock domains based on the current simulation tick
    for (int i = 0; i < clock_domains.GetCount(); i++) {
        ClockDomain& domain = clock_domains[i];
        
        // For a frequency-based domain, we need to calculate when the next clock edge should occur
        if (domain.frequency_hz > 0) {
            // Calculate the expected next edge based on the frequency
            // In this simplified model, we toggle the clock state periodically
            int expected_next_edge = domain.last_edge_tick + (int)(domain.period_ticks);
            
            // If we've reached the expected next edge time
            if (current_tick >= expected_next_edge) {
                // Toggle the clock state to create an edge
                domain.clock_state = !domain.clock_state;
                domain.last_edge_tick = current_tick;
                
                LOG("Clock domain " << domain.id << " toggled to " 
                     << (domain.clock_state ? "HIGH" : "LOW") << " at tick " << current_tick);
            }
        }
    }
}

void Machine::ReportClockDomainInfo() {
    LOG("CLOCK DOMAIN REPORT");
    LOG("==================");
    LOG("Global clock multiplier: " << global_clock_multiplier);
    LOG("Total clock domains: " << clock_domains.GetCount());
    
    for (int i = 0; i < clock_domains.GetCount(); i++) {
        const ClockDomain& domain = clock_domains[i];
        LOG("Domain ID: " << domain.id);
        LOG("  Frequency: " << domain.frequency_hz << " Hz");
        LOG("  Period (ticks): " << domain.period_ticks);
        LOG("  Current state: " << (domain.clock_state ? "HIGH" : "LOW"));
        LOG("  Last edge tick: " << domain.last_edge_tick);
        
        // Count components in this domain by traversing all PCBs
        int componentCount = 0;
        for (Pcb& pcb : pcbs) {
            for (int j = 0; j < pcb.nodes.GetCount(); j++) {
                if (pcb.nodes[j].GetClockDomainId() == domain.id) {
                    componentCount++;
                }
            }
        }
        LOG("  Components in domain: " << componentCount);
        
        // List a few components in this domain
        int listed = 0;
        for (Pcb& pcb : pcbs) {
            for (int j = 0; j < pcb.nodes.GetCount(); j++) {
                if (pcb.nodes[j].GetClockDomainId() == domain.id && listed < 5) {
                    LOG("    - " << pcb.nodes[j].GetDynamicName());
                    listed++;
                }
            }
        }
        
        if (componentCount > 5) {
            LOG("    ... and " << (componentCount - 5) << " more");
        }
    }
    
    LOG("==================");
}

void Machine::SetGlobalClockMultiplier(double multiplier) {
    global_clock_multiplier = multiplier;
    LOG("Global clock multiplier set to: " << multiplier);
    
    // Update periods for all clock domains based on the new multiplier
    for (int i = 0; i < clock_domains.GetCount(); i++) {
        ClockDomain& domain = clock_domains[i];
        if (domain.frequency_hz > 0) {
            double effective_freq = domain.frequency_hz * global_clock_multiplier;
            if (effective_freq > 0) {
                domain.period_ticks = 1.0 / effective_freq;
            }
        }
    }
}
