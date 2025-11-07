#include "TimingAnalysis.h"

// Implementation of TimingAnalyzer methods
TimingAnalyzer::TimingAnalyzer(Machine* mach) : machine(mach), current_tick(0) {
}

void TimingAnalyzer::DiscoverAllTimingPaths() {
    timing_paths.Clear();
    
    if (!machine) return;
    
    // For each PCB in the machine
    for (int pcb_idx = 0; pcb_idx < machine->pcbs.GetCount(); pcb_idx++) {
        Pcb& pcb = machine->pcbs[pcb_idx];
        
        // For each component on the PCB, discover paths from it
        for (int comp_idx = 0; comp_idx < pcb.GetNodeCount(); comp_idx++) {
            ElectricNodeBase* comp = &pcb.GetNode(comp_idx);
            DiscoverTimingPathFrom(comp);
        }
    }
}

void TimingAnalyzer::DiscoverTimingPathFrom(ElectricNodeBase* start_component) {
    // This is a simplified path discovery algorithm
    // In a real implementation, this would implement a more sophisticated algorithm
    
    if (!start_component) return;
    
    // For now, just create a simple path with the component itself
    TimingPath path;
    path.components.Add(start_component);
    path.path_name = "PathFrom_" + start_component->GetName();
    path.total_delay = start_component->GetDelayTicks();  // Use the component's delay
    
    timing_paths.Add(path);
}

void TimingAnalyzer::DiscoverTimingPathTo(ElectricNodeBase* end_component) {
    // This would find all paths that end at the specified component
    // Implementation similar to DiscoverTimingPathFrom but in reverse
    if (!end_component) return;
    
    // For now, we'll just add this component to our list
    TimingPath path;
    path.components.Add(end_component);
    path.path_name = "PathTo_" + end_component->GetName();
    path.total_delay = end_component->GetDelayTicks();
    
    timing_paths.Add(path);
}

void TimingAnalyzer::AnalyzeTimingPath(TimingPath& path) {
    // Calculate the total delay for this path
    path.total_delay = 0;
    for (int i = 0; i < path.components.GetCount(); i++) {
        path.total_delay += path.components[i]->GetDelayTicks();
    }
}

void TimingAnalyzer::AnalyzePropagationDelays() {
    // Analyze propagation delays for all discovered timing paths
    for (int i = 0; i < timing_paths.GetCount(); i++) {
        AnalyzeTimingPath(timing_paths[i]);
    }
    
    // Also calculate delays for individual components based on their configuration
    if (!machine) return;
    
    for (int pcb_idx = 0; pcb_idx < machine->pcbs.GetCount(); pcb_idx++) {
        Pcb& pcb = machine->pcbs[pcb_idx];
        
        for (int comp_idx = 0; comp_idx < pcb.GetNodeCount(); comp_idx++) {
            ElectricNodeBase& comp = pcb.GetNode(comp_idx);
            
            // Perform any component-specific delay analysis
            // For example, check if the component has timing constraints
            if (comp.GetSetupTimeTicks() > 0 || comp.GetHoldTimeTicks() > 0) {
                // This component has timing requirements that should be analyzed
                LOG("Component " << comp.GetName() << " has timing constraints: "
                     << "Setup=" << comp.GetSetupTimeTicks() << "t, "
                     << "Hold=" << comp.GetHoldTimeTicks() << "t");
            }
        }
    }
}

int TimingAnalyzer::CalculatePathDelay(const Vector<ElectricNodeBase*>& path) const {
    int total_delay = 0;
    for (int i = 0; i < path.GetCount(); i++) {
        total_delay += path[i]->GetDelayTicks();
    }
    return total_delay;
}

void TimingAnalyzer::ReportPropagationDelays() const {
    LOG("=== PROPAGATION DELAY ANALYSIS REPORT ===");
    LOG("Total timing paths analyzed: " << timing_paths.GetCount());
    
    for (int i = 0; i < timing_paths.GetCount(); i++) {
        const TimingPath& path = timing_paths[i];
        LOG("Path " << i << ": " << path.path_name 
             << " (Components: " << path.components.GetCount() 
             << ", Total Delay: " << path.total_delay << " ticks)");
        
        // List components in this path
        for (int j = 0; j < path.components.GetCount(); j++) {
            LOG("  [" << j << "] " << path.components[j]->GetName() 
                 << " (Delay: " << path.components[j]->GetDelayTicks() << "t)");
        }
    }
    
    if (timing_paths.IsEmpty()) {
        LOG("No timing paths found for analysis");
    }
    
    LOG("==========================================");
}

void TimingAnalyzer::CheckSetupHoldTimes() {
    // Check setup and hold times for all components
    if (!machine) return;
    
    for (int pcb_idx = 0; pcb_idx < machine->pcbs.GetCount(); pcb_idx++) {
        Pcb& pcb = machine->pcbs[pcb_idx];
        
        for (int comp_idx = 0; comp_idx < pcb.GetNodeCount(); comp_idx++) {
            ElectricNodeBase& comp = pcb.GetNode(comp_idx);
            
            // Check if the component has setup/hold time requirements
            if (comp.GetSetupTimeTicks() > 0 || comp.GetHoldTimeTicks() > 0) {
                // In a real implementation, this would analyze the timing
                // relationship between connected components
                LOG("Checking timing for component: " << comp.GetName());
                
                // Example: Check if the component has critical timing paths
                if (comp.GetSetupTimeTicks() > 0) {
                    LOG("  Setup time requirement: " << comp.GetSetupTimeTicks() << " ticks");
                }
                if (comp.GetHoldTimeTicks() > 0) {
                    LOG("  Hold time requirement: " << comp.GetHoldTimeTicks() << " ticks");
                }
            }
        }
    }
}

void TimingAnalyzer::CheckMaxDelayConstraints() {
    // Check if any paths exceed maximum allowed delay constraints
    // This would typically involve user-specified constraints
    
    for (int i = 0; i < timing_paths.GetCount(); i++) {
        const TimingPath& path = timing_paths[i];
        // In a real implementation, we might have max delay constraints
        // For now, we'll just note very long paths
        if (path.total_delay > 50) {  // Arbitrary threshold for demonstration
            AddViolation(path.path_name, "MAX_DELAY", 
                        String().Cat() << "Path delay " << path.total_delay 
                                       << " exceeds recommended limit");
        }
    }
}

void TimingAnalyzer::CheckClockDomainCrossings() {
    // Check for signals crossing between different clock domains
    if (!machine) return;
    
    LOG("Checking for clock domain crossings...");
    
    for (int pcb_idx = 0; pcb_idx < machine->pcbs.GetCount(); pcb_idx++) {
        Pcb& pcb = machine->pcbs[pcb_idx];
        
        for (int i = 0; i < pcb.GetNodeCount(); i++) {
            ElectricNodeBase& srcNode = pcb.GetNode(i);
            
            // Look at each connector of this node
            for (int j = 0; j < srcNode.GetConnectorCount(); j++) {
                ElectricNodeBase::Connector& conn = srcNode.GetConnector(j);
                
                // If this is an output, check where it connects
                if (conn.is_src && conn.IsConnected()) {
                    for (int k = 0; k < conn.links.GetCount(); k++) {
                        if (conn.links[k].link) {
                            ElectricNodeBase::Connector* dest_conn = conn.links[k].link->sink;
                            if (dest_conn && dest_conn->base) {
                                // Check if the source and destination are in different clock domains
                                if (srcNode.GetClockDomainId() != dest_conn->base->GetClockDomainId()) {
                                    LOG("CLOCK DOMAIN CROSSING: " 
                                         << srcNode.GetClassName() << ":" << srcNode.GetName() 
                                         << " (domain " << srcNode.GetClockDomainId() << ") -> "
                                         << dest_conn->base->GetClassName() << ":" << dest_conn->base->GetName()
                                         << " (domain " << dest_conn->base->GetClockDomainId() << ")");
                                    
                                    AddViolation(
                                        srcNode.GetName() + " -> " + dest_conn->base->GetName(),
                                        "CLOCK_DOMAIN_CROSSING",
                                        String().Cat() << "Signal crosses between domain " 
                                                       << srcNode.GetClockDomainId() 
                                                       << " and domain " 
                                                       << dest_conn->base->GetClockDomainId()
                                    );
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void TimingAnalyzer::ReportTimingAnalysis() const {
    LOG("=== DETAILED TIMING ANALYSIS REPORT ===");
    LOG("Current simulation tick: " << current_tick);
    
    ReportPropagationDelays();
    
    LOG("Clock domain crossings: ");
    // This would be reported in CheckClockDomainCrossings
    
    LOG("========================================");
}

void TimingAnalyzer::ReportTimingViolations() const {
    LOG("=== TIMING VIOLATIONS REPORT ===");
    LOG("Total violations found: " << violations.GetCount());
    
    if (violations.IsEmpty()) {
        LOG("No timing violations detected");
    } else {
        for (int i = 0; i < violations.GetCount(); i++) {
            const TimingViolation& v = violations[i];
            LOG("[" << i << "] " << v.component_name 
                 << " - " << v.violation_type 
                 << ": " << v.details 
                 << " (Tick: " << v.tick_number << ")");
        }
    }
    
    LOG("===============================");
}

void TimingAnalyzer::AddViolation(const String& comp_name, const String& type, const String& details) {
    TimingViolation violation;
    violation.component_name = comp_name;
    violation.violation_type = type;
    violation.details = details;
    violation.tick_number = current_tick;
    
    violations.Add(violation);
    
    // Log the violation
    LOG("TIMING VIOLATION: " << comp_name << " - " << type << ": " << details);
}

void TimingAnalyzer::ClearResults() {
    timing_paths.Clear();
    violations.Clear();
}

void TimingAnalyzer::IdentifyCriticalPaths() {
    // Identify paths with the longest delays (critical paths)
    // Sort paths by delay in descending order
    Sort(timing_paths, [](const TimingPath& a, const TimingPath& b) {
        return a.total_delay > b.total_delay;
    });
}

void TimingAnalyzer::ReportCriticalPaths(int limit) const {
    LOG("=== CRITICAL PATHS REPORT (Top " << limit << ") ===");
    
    int report_count = min(limit, timing_paths.GetCount());
    for (int i = 0; i < report_count; i++) {
        const TimingPath& path = timing_paths[i];
        LOG("[" << i << "] " << path.path_name 
             << " - Delay: " << path.total_delay << " ticks");
        
        // List components in this path
        for (int j = 0; j < path.components.GetCount(); j++) {
            LOG("    [" << j << "] " << path.components[j]->GetName() 
                 << " (Delay: " << path.components[j]->GetDelayTicks() << "t)");
        }
    }
    
    LOG("=========================================");
}

// Implementation of TimedComponent methods
TimedComponent::TimedComponent() : ElcBase() {
    propagation_delay = 1;  // Default to 1 tick delay
    setup_time = 0;         // No setup time requirement by default
    hold_time = 0;          // No hold time requirement by default
    clock_to_q_delay = 2;   // Default clock-to-Q delay for sequential elements
    input_delays.AddCount(10, 1);  // Default to 1 tick delay for first 10 inputs
}

void TimedComponent::SetInputDelay(int input_idx, int delay) {
    if (input_idx >= input_delays.GetCount()) {
        input_delays.SetCount(input_idx + 1);
    }
    input_delays[input_idx] = delay;
}

int TimedComponent::GetInputDelay(int input_idx) const {
    if (input_idx >= 0 && input_idx < input_delays.GetCount()) {
        return input_delays[input_idx];
    }
    return 0;  // Default delay if index is out of bounds
}

void TimedComponent::AddFanInComponent(ElectricNodeBase* comp) {
    for (int i = 0; i < fan_in.GetCount(); i++) {
        if (fan_in[i] == comp) {
            return;  // Already added
        }
    }
    fan_in.Add(comp);
}

void TimedComponent::AddFanOutComponent(ElectricNodeBase* comp) {
    for (int i = 0; i < fan_out.GetCount(); i++) {
        if (fan_out[i] == comp) {
            return;  // Already added
        }
    }
    fan_out.Add(comp);
}

bool TimedComponent::Tick() {
    // In a real implementation, timing analysis would happen here
    return true;
}

bool TimedComponent::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    // Process implementation with potential timing considerations
    return ElcBase::Process(type, bytes, bits, conn_id, dest, dest_conn_id);
}

bool TimedComponent::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    // PutRaw implementation with potential timing considerations
    return ElcBase::PutRaw(conn_id, data, data_bytes, data_bits);
}

// Implementation of TimingProbe methods
TimingProbe::TimingProbe(ElectricNodeBase* comp, const String& pin) 
    : target_component(comp), target_pin(pin), last_change_tick(-1) {
    // Set up as a probe/sink to monitor the target pin
    if (target_component && !target_pin.IsEmpty()) {
        LOG("TimingProbe initialized to monitor: " << target_component->GetName() << "." << target_pin);
    }
}

void TimingProbe::SetTarget(ElectricNodeBase* comp, const String& pin) {
    target_component = comp;
    target_pin = pin;
    LOG("TimingProbe target updated to: " << (comp ? comp->GetName() : "nullptr") << "." << pin);
}

void TimingProbe::RecordChange(byte value, int tick) {
    timestamps.Add(tick);
    values.Add(value);
    last_change_tick = tick;
    
    LOG("TimingProbe recorded change: value=" << (int)value << " at tick=" << tick);
}

double TimingProbe::CalculateFrequency() const {
    if (timestamps.GetCount() < 2) {
        return 0.0;  // Not enough data points to calculate frequency
    }
    
    // Calculate based on the time between the first and last changes
    int time_span = timestamps[timestamps.GetCount()-1] - timestamps[0];
    if (time_span == 0) {
        return 0.0;
    }
    
    // Count the number of state changes
    int change_count = timestamps.GetCount() - 1;
    
    // For frequency calculation, we might want to consider transitions between 0 and 1
    // For now, we'll just return the rate of change events
    return (double)change_count / time_span;
}

double TimingProbe::CalculatePeriod() const {
    double freq = CalculateFrequency();
    if (freq > 0) {
        return 1.0 / freq;
    }
    return 0.0;
}

int TimingProbe::GetRiseTime(int threshold) const {
    // For simplicity, return a placeholder value
    // In a real implementation, this would calculate the rise time
    return 1;  // Placeholder
}

int TimingProbe::GetFallTime(int threshold) const {
    // For simplicity, return a placeholder value
    // In a real implementation, this would calculate the fall time
    return 1;  // Placeholder
}

int TimingProbe::GetPropagationDelay(const TimingProbe& other) const {
    // Calculate the propagation delay between this probe and another
    if (timestamps.IsEmpty() || other.timestamps.IsEmpty()) {
        return -1;  // No data available
    }
    
    // Find the closest timestamps between the two probes
    int last_this = timestamps[timestamps.GetCount()-1];
    int last_other = other.timestamps[other.timestamps.GetCount()-1];
    
    return abs(last_this - last_other);
}

void TimingProbe::ReportTimingMeasurements() const {
    LOG("=== TIMING PROBE MEASUREMENTS REPORT ===");
    LOG("Target: " << (target_component ? target_component->GetName() : "nullptr") << "." << target_pin);
    LOG("Total changes recorded: " << timestamps.GetCount());
    
    if (!timestamps.IsEmpty()) {
        LOG("First change at tick: " << timestamps[0]);
        LOG("Last change at tick: " << timestamps[timestamps.GetCount()-1]);
        LOG("Last change tick: " << last_change_tick);
        
        if (timestamps.GetCount() > 1) {
            double freq = CalculateFrequency();
            double period = CalculatePeriod();
            LOG("Calculated frequency: " << freq << " changes per tick");
            LOG("Calculated period: " << period << " ticks per change");
        }
    }
    
    LOG("========================================");
}

bool TimingProbe::Tick() {
    // In a real implementation, this would monitor the target signal
    // For now, we'll just return true
    return true;
}

bool TimingProbe::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    // Process implementation - could be used to receive data from the monitored component
    return true;
}

bool TimingProbe::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    // PutRaw implementation - could receive data from the monitored component
    // Record the change with the current tick number from the machine
    // This would need access to the current tick, which we might get from the machine context
    return true;
}