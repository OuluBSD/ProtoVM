#include "ProtoVM.h"




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
	for (const ProcessOp& op : l.rt_ops) {
		bool op_changed = false;
		switch (op.type) {
		//case ProcessType::READ:
		case ProcessType::WRITE:
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
			
		case ProcessType::TICK:
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
			break;
			
		default:
			LOG("Machine::RunRtOpsWithChangeDetection: unhandled ProcessType");
			return false;
		}
		
		if (op_changed) {
			changed = true;
		}
		op_i++;
	}
	return true;
}

Pcb& Machine::AddPcb() {
	Pcb& p = pcbs.Add();
	p.mach = this;
	return p;
}



