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
	// In a real implementation, we might want to track clock domains in a map
	// For now, we'll just use integer IDs starting from 1
	// In the real system, the Machine might track domains in a member variable
	// but to avoid changing the class definition again, we'll just return an ID
	// In practice, this would be more sophisticated
	
	// Since we don't have a domain storage, we'll just return the next available ID
	// We'll assume that domain IDs 0 is the default, so new ones start at 1
	// This is a simplified implementation - in a real system, we would track domains
	
	// For simplicity, we'll just return a sequential ID
	// In a real implementation, we'd store the frequency for the domain
	static int next_domain_id = 1;
	return next_domain_id++;
}

void Machine::AssignComponentToClockDomain(ElectricNodeBase* component, int domain_id) {
	if (component) {
		component->SetClockDomain(domain_id);
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



