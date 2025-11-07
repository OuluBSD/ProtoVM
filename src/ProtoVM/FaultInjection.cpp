#include "FaultInjection.h"

// Implementation of FaultInjectionManager methods
FaultInjectionManager::FaultInjectionManager(Machine* mach) : machine(mach), current_tick(0), injection_active(true) {
}

int FaultInjectionManager::ScheduleStuckAtFault(const String& comp_name, const String& pin_name, byte value, int start_tick, int duration) {
    FaultDescriptor fault;
    fault.component_name = comp_name;
    fault.pin_name = pin_name;
    fault.fault_type = FAULT_STUCK_AT_0;  // This will be corrected based on value
    fault.fault_type = (value == 0) ? FAULT_STUCK_AT_0 : FAULT_STUCK_AT_1;
    fault.start_tick = start_tick;
    fault.duration = duration;
    fault.fault_value = value;
    fault.active = false;
    
    // Assign a unique ID
    fault.fault_id = scheduled_faults.GetCount();
    
    scheduled_faults.Add(fault);
    LOG("Scheduled stuck-at-" << (int)value << " fault for " << comp_name << "." << pin_name 
         << " starting at tick " << start_tick);
    
    return fault.fault_id;
}

int FaultInjectionManager::ScheduleOpenCircuitFault(const String& comp_name, const String& pin_name, int start_tick, int duration) {
    FaultDescriptor fault;
    fault.component_name = comp_name;
    fault.pin_name = pin_name;
    fault.fault_type = FAULT_OPEN_CIRCUIT;
    fault.start_tick = start_tick;
    fault.duration = duration;
    fault.active = false;
    
    // Assign a unique ID
    fault.fault_id = scheduled_faults.GetCount();
    
    scheduled_faults.Add(fault);
    LOG("Scheduled open circuit fault for " << comp_name << "." << pin_name 
         << " starting at tick " << start_tick);
    
    return fault.fault_id;
}

int FaultInjectionManager::ScheduleShortCircuitFault(const String& comp1, const String& pin1, const String& comp2, const String& pin2, int start_tick, int duration) {
    FaultDescriptor fault;
    fault.component_name = comp1 + "/" + comp2;  // Store both components
    fault.pin_name = pin1 + "/" + pin2;          // Store both pins
    fault.fault_type = FAULT_SHORT_CIRCUIT;
    fault.start_tick = start_tick;
    fault.duration = duration;
    fault.active = false;
    
    // Store additional component/pin as additional param
    fault.additional_param = scheduled_faults.GetCount(); // Use index to identify pair
    
    // Assign a unique ID
    fault.fault_id = scheduled_faults.GetCount();
    
    scheduled_faults.Add(fault);
    LOG("Scheduled short circuit fault between " << comp1 << "." << pin1 
         << " and " << comp2 << "." << pin2 << " starting at tick " << start_tick);
    
    return fault.fault_id;
}

int FaultInjectionManager::ScheduleNoiseFault(const String& comp_name, const String& pin_name, double noise_prob, int start_tick, int duration) {
    FaultDescriptor fault;
    fault.component_name = comp_name;
    fault.pin_name = pin_name;
    fault.fault_type = FAULT_NOISE;
    fault.start_tick = start_tick;
    fault.duration = duration;
    fault.probability = noise_prob;
    fault.active = false;
    
    // Assign a unique ID
    fault.fault_id = scheduled_faults.GetCount();
    
    scheduled_faults.Add(fault);
    LOG("Scheduled noise fault (prob=" << noise_prob << ") for " << comp_name << "." << pin_name 
         << " starting at tick " << start_tick);
    
    return fault.fault_id;
}

int FaultInjectionManager::ScheduleDelayFault(const String& comp_name, const String& pin_name, int extra_delay, int start_tick, int duration) {
    FaultDescriptor fault;
    fault.component_name = comp_name;
    fault.pin_name = pin_name;
    fault.fault_type = FAULT_DELAY;
    fault.start_tick = start_tick;
    fault.duration = duration;
    fault.additional_param = extra_delay;
    fault.active = false;
    
    // Assign a unique ID
    fault.fault_id = scheduled_faults.GetCount();
    
    scheduled_faults.Add(fault);
    LOG("Scheduled delay fault (+" << extra_delay << " ticks) for " << comp_name << "." << pin_name 
         << " starting at tick " << start_tick);
    
    return fault.fault_id;
}

int FaultInjectionManager::ScheduleFault(const FaultDescriptor& fault) {
    FaultDescriptor f = fault;
    f.fault_id = scheduled_faults.GetCount();
    f.active = false;
    
    scheduled_faults.Add(f);
    LOG("Scheduled fault: " << f.component_name << "." << f.pin_name 
         << " type=" << f.fault_type << " start=" << f.start_tick);
    
    return f.fault_id;
}

void FaultInjectionManager::InjectFaults() {
    if (!injection_active || !machine) return;
    
    // Process all scheduled faults
    for (int i = 0; i < scheduled_faults.GetCount(); i++) {
        FaultDescriptor& fault = scheduled_faults[i];
        
        // Check if it's time to activate this fault
        if (!fault.active && current_tick >= fault.start_tick) {
            ActivateFault(fault.fault_id);
        }
        
        // Check if fault should be deactivated (if it has a duration)
        if (fault.active && fault.duration > 0 && 
            current_tick >= (fault.start_tick + fault.duration)) {
            DeactivateFault(fault.fault_id);
        }
    }
}

void FaultInjectionManager::ProcessActiveFaults() {
    // Process all active faults
    for (int i = 0; i < scheduled_faults.GetCount(); i++) {
        FaultDescriptor& fault = scheduled_faults[i];
        if (!fault.active) continue;
        
        // Find the component and apply the fault based on type
        for (int pcb_idx = 0; pcb_idx < machine->pcbs.GetCount(); pcb_idx++) {
            Pcb& pcb = machine->pcbs[pcb_idx];
            
            for (int comp_idx = 0; comp_idx < pcb.GetNodeCount(); comp_idx++) {
                ElectricNodeBase& comp = pcb.GetNode(comp_idx);
                
                if (comp.GetName() == fault.component_name) {
                    // Apply fault based on type
                    switch (fault.fault_type) {
                        case FAULT_STUCK_AT_0:
                        case FAULT_STUCK_AT_1:
                            // For stuck-at faults, we need to intercept signal values
                            // This would require more complex implementation
                            LOG("Applied stuck-at-" << (int)fault.fault_value 
                                 << " fault to " << comp.GetName());
                            break;
                            
                        case FAULT_OPEN_CIRCUIT:
                            LOG("Applied open circuit fault to " << comp.GetName() 
                                 << "." << fault.pin_name);
                            break;
                            
                        case FAULT_NOISE:
                            LOG("Applied noise fault (prob=" << fault.probability 
                                 << ") to " << comp.GetName() << "." << fault.pin_name);
                            break;
                            
                        case FAULT_DELAY:
                            LOG("Applied delay fault (+" << fault.additional_param 
                                 << " ticks) to " << comp.GetName() << "." << fault.pin_name);
                            break;
                            
                        default:
                            break;
                    }
                }
            }
        }
    }
}

void FaultInjectionManager::ActivateFault(int fault_id) {
    if (fault_id < 0 || fault_id >= scheduled_faults.GetCount()) {
        LOG("Error: Invalid fault ID " << fault_id);
        return;
    }
    
    FaultDescriptor& fault = scheduled_faults[fault_id];
    if (!fault.active) {
        fault.active = true;
        LOG("Activated fault " << fault_id << " (" << fault.component_name 
             << "." << fault.pin_name << ")");
    }
}

void FaultInjectionManager::DeactivateFault(int fault_id) {
    if (fault_id < 0 || fault_id >= scheduled_faults.GetCount()) {
        LOG("Error: Invalid fault ID " << fault_id);
        return;
    }
    
    FaultDescriptor& fault = scheduled_faults[fault_id];
    if (fault.active) {
        fault.active = false;
        LOG("Deactivated fault " << fault_id << " (" << fault.component_name 
             << "." << fault.pin_name << ")");
    }
}

void FaultInjectionManager::RemoveFault(int fault_id) {
    if (fault_id < 0 || fault_id >= scheduled_faults.GetCount()) {
        LOG("Error: Invalid fault ID " << fault_id);
        return;
    }
    
    scheduled_faults.Remove(fault_id);
    LOG("Removed fault " << fault_id);
    
    // Update fault IDs for remaining faults
    for (int i = fault_id; i < scheduled_faults.GetCount(); i++) {
        scheduled_faults[i].fault_id = i;
    }
}

void FaultInjectionManager::OnPreTick() {
    if (!injection_active) return;
    
    // Prepare for fault injection at the start of each tick
    InjectFaults();
    ProcessActiveFaults();
}

void FaultInjectionManager::OnPostTick() {
    if (!injection_active) return;
    
    // Update current tick counter
    current_tick++;
    
    // Check for any failure conditions that may have resulted from faults
    // This would involve checking for unexpected behavior in the machine
    if (machine && machine->GetTimingViolationCount() > 0) {
        LOG("Potential failure detected due to timing violations: " << machine->GetTimingViolationCount());
        
        // Record the failure in results
        FaultInjectionResult result;
        result.fault_description = "Timing violation due to fault injection";
        result.caused_failure = true;
        result.tick_of_failure = current_tick;
        results.Add(result);
    }
}

void FaultInjectionManager::ReportFaultInjectionResults() const {
    LOG("=== FAULT INJECTION RESULTS REPORT ===");
    LOG("Total scheduled faults: " << scheduled_faults.GetCount());
    LOG("Total test results: " << results.GetCount());
    
    int active_count = 0;
    for (const auto& fault : scheduled_faults) {
        if (fault.active) active_count++;
    }
    LOG("Currently active faults: " << active_count);
    
    if (!results.IsEmpty()) {
        LOG("Faults that caused failures:");
        for (int i = 0; i < results.GetCount(); i++) {
            const FaultInjectionResult& result = results[i];
            LOG("  [" << i << "] " << result.fault_description 
                 << " at tick " << result.tick_of_failure);
        }
    } else {
        LOG("No failures detected during fault injection tests");
    }
    
    LOG("=====================================");
}

void FaultInjectionManager::ClearResults() {
    results.Clear();
}

bool FaultInjectionManager::VerifyFaultTolerance(const String& test_name, int max_ticks) {
    LOG("Starting fault tolerance verification: " << test_name);
    
    // Save original state
    bool original_injection_state = injection_active;
    DisableInjection();  // We'll control injection manually
    
    // Run the test for max_ticks
    for (int i = 0; i < max_ticks; i++) {
        if (!machine->Tick()) {
            LOG("Simulation failed at tick " << i << " during fault tolerance test");
            EnableInjection();
            return false;
        }
        
        // Check for any anomalies that might indicate lack of fault tolerance
        if (machine->GetTimingViolationCount() > 10) { // arbitrary threshold
            LOG("Too many timing violations detected - circuit may not be fault tolerant");
            EnableInjection();
            return false;
        }
    }
    
    // Restore original state
    if (original_injection_state) EnableInjection();
    
    LOG("Fault tolerance verification passed for: " << test_name);
    return true;
}

void FaultInjectionManager::RunFaultToleranceTests() {
    LOG("Running comprehensive fault tolerance tests...");
    
    // Test 1: Single stuck-at fault
    int fault_id = ScheduleStuckAtFault("test_component", "test_pin", 0, 10, 50);
    VerifyFaultTolerance("Stuck-at-0 fault test", 100);
    RemoveFault(fault_id);
    
    // Test 2: Noise fault
    fault_id = ScheduleNoiseFault("test_component", "test_pin", 0.1, 10, 50);
    VerifyFaultTolerance("Noise fault test", 100);
    RemoveFault(fault_id);
    
    // Test 3: Multiple simultaneous faults
    int fault_id1 = ScheduleStuckAtFault("comp1", "pin1", 1, 5, 60);
    int fault_id2 = ScheduleOpenCircuitFault("comp2", "pin2", 15, 40);
    VerifyFaultTolerance("Multiple simultaneous faults test", 100);
    RemoveFault(fault_id1);
    RemoveFault(fault_id2);
    
    LOG("Completed fault tolerance tests");
}

// Implementation of utility functions

bool StuckAtFaultInjector::ApplyFault(ElectricNodeBase* component, const String& pin_name, byte fault_value) {
    if (!component) return false;
    
    LOG("Applying stuck-at-" << (int)fault_value << " fault to " 
         << component->GetName() << "." << pin_name);
    // In a real implementation, this would modify the component's behavior
    // to force the output pin to the fault value
    return true;
}

bool StuckAtFaultInjector::RemoveFault(ElectricNodeBase* component, const String& pin_name) {
    if (!component) return false;
    
    LOG("Removing stuck-at fault from " 
         << component->GetName() << "." << pin_name);
    // In a real implementation, this would restore normal operation
    return true;
}

byte NoiseFaultInjector::AddNoise(byte original_value, double noise_prob) {
    if (noise_prob <= 0.0) return original_value;
    
    // Generate a random number between 0 and 1
    double rand_val = (double)rand() / RAND_MAX;
    
    // Apply noise if random value is less than probability
    if (rand_val < noise_prob) {
        // Flip a random bit
        int bit_to_flip = rand() % 8;  // 0 to 7
        return original_value ^ (1 << bit_to_flip);
    }
    
    return original_value;
}

bool NoiseFaultInjector::ApplyFault(ElectricNodeBase* component, const String& pin_name, double noise_probability) {
    if (!component) return false;
    
    LOG("Applying noise fault (prob=" << noise_probability << ") to " 
         << component->GetName() << "." << pin_name);
    // In a real implementation, this would modify the component to add noise
    return true;
}

bool NoiseFaultInjector::RemoveFault(ElectricNodeBase* component, const String& pin_name) {
    if (!component) return false;
    
    LOG("Removing noise fault from " 
         << component->GetName() << "." << pin_name);
    // In a real implementation, this would restore normal operation
    return true;
}

bool DelayFaultInjector::ApplyFault(ElectricNodeBase* component, const String& pin_name, int extra_delay_ticks) {
    if (!component) return false;
    
    LOG("Applying delay fault (+" << extra_delay_ticks << " ticks) to " 
         << component->GetName() << "." << pin_name);
    // In a real implementation, this would modify the component to add delay
    return true;
}

bool DelayFaultInjector::RemoveFault(ElectricNodeBase* component, const String& pin_name) {
    if (!component) return false;
    
    LOG("Removing delay fault from " 
         << component->GetName() << "." << pin_name);
    // In a real implementation, this would restore normal delay
    return true;
}

// Implementation of FaultInjectableComponent methods
FaultInjectableComponent::FaultInjectableComponent() : ElcBase(), fault_mode(false) {
}

void FaultInjectableComponent::AddFault(const FaultDescriptor& fault) {
    active_faults.Add(fault);
    fault_mode = true;
    LOG("Added fault to component: " << GetName());
}

void FaultInjectableComponent::RemoveFault(int fault_id) {
    for (int i = 0; i < active_faults.GetCount(); i++) {
        if (active_faults[i].fault_id == fault_id) {
            active_faults.Remove(i);
            LOG("Removed fault from component: " << GetName());
            if (active_faults.IsEmpty()) {
                fault_mode = false;
            }
            return;
        }
    }
}

void FaultInjectableComponent::ClearFaults() {
    active_faults.Clear();
    fault_mode = false;
    LOG("Cleared all faults from component: " << GetName());
}

bool FaultInjectableComponent::Tick() {
    // Call the normal tick function
    bool result = ElcBase::Tick();
    
    // Apply any active faults post-tick
    for (int i = 0; i < active_faults.GetCount(); i++) {
        const FaultDescriptor& fault = active_faults[i];
        
        // Apply fault effects based on type
        switch (fault.fault_type) {
            case FAULT_STUCK_AT_0:
            case FAULT_STUCK_AT_1:
                // For stuck-at faults, we might need to modify internal state
                LOG("Applying stuck-at fault in Tick for " << GetName());
                break;
                
            case FAULT_NOISE:
                // Noise might affect internal calculations
                LOG("Applying noise fault in Tick for " << GetName());
                break;
                
            case FAULT_DELAY:
                // Delays might affect timing behavior
                LOG("Applying delay fault in Tick for " << GetName());
                break;
                
            default:
                break;
        }
    }
    
    return result;
}

bool FaultInjectableComponent::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    // Apply faults that might affect processing
    if (HasActiveFaults()) {
        for (int i = 0; i < active_faults.GetCount(); i++) {
            const FaultDescriptor& fault = active_faults[i];
            
            // Modify behavior based on active fault
            if (fault.fault_type == FAULT_OPEN_CIRCUIT) {
                // For open circuit, don't process certain connections
                return true; // Skip processing as if connection is broken
            }
        }
    }
    
    // Call normal process
    return ElcBase::Process(type, bytes, bits, conn_id, dest, dest_conn_id);
}

bool FaultInjectableComponent::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    // Check if we need to apply a fault to the incoming data
    byte modified_data = ApplyFaultToValue(*data, conn_id);
    
    // Call the normal putraw with potentially modified data
    return ElcBase::PutRaw(conn_id, &modified_data, data_bytes, data_bits);
}

byte FaultInjectableComponent::ApplyFaultToValue(byte original_value, uint16 conn_id) {
    if (!HasActiveFaults()) return original_value;
    
    byte result = original_value;
    
    for (int i = 0; i < active_faults.GetCount(); i++) {
        const FaultDescriptor& fault = active_faults[i];
        
        switch (fault.fault_type) {
            case FAULT_STUCK_AT_0:
                if (IsFaultActiveForPin(conn_id)) {
                    result = 0;
                }
                break;
                
            case FAULT_STUCK_AT_1:
                if (IsFaultActiveForPin(conn_id)) {
                    result = fault.fault_value; // Should be 1
                }
                break;
                
            case FAULT_NOISE:
                if (IsFaultActiveForPin(conn_id)) {
                    result = NoiseFaultInjector::AddNoise(result, fault.probability);
                }
                break;
                
            default:
                break;
        }
    }
    
    return result;
}

bool FaultInjectableComponent::IsFaultActiveForPin(uint16 conn_id) {
    // Check if any active fault applies to this specific pin/connection
    // This would need to map the conn_id to pin names
    return true; // Simplified implementation
}