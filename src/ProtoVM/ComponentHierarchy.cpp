#include "ComponentHierarchy.h"
#include "StandardLibrary.h"

// Implementation of HierarchicalComponent
HierarchicalComponent::HierarchicalComponent(const String& name) : internal_pcb() {
    SetName(name);
    internal_pcb.SetName(String().Cat() << name << "_internal");
    
    // Call the virtual setup method to allow derived classes to add subcomponents
    SetupSubcomponents();
}

HierarchicalComponent::~HierarchicalComponent() {
    // Clean up subcomponents - they are owned by the internal PCB
    // The U++ framework will handle cleanup
}

template<typename T>
T& HierarchicalComponent::AddSubcomponent(const String& name) {
    T& comp = internal_pcb.Add<T>(name);
    subcomponents.Add(&comp);
    return comp;
}

bool HierarchicalComponent::Tick() {
    // Tick all internal components
    for (int i = 0; i < subcomponents.GetCount(); i++) {
        subcomponents[i]->Tick();
    }
    
    // Call the virtual method for custom logic
    ConnectSubcomponents();
    
    return true;
}

bool HierarchicalComponent::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    // Process method implementation will be specific to each hierarchical component
    // For now, we'll just return true
    return true;
}

bool HierarchicalComponent::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    // PutRaw method implementation will be specific to each hierarchical component
    // For now, we'll just return true
    return true;
}

// Explicit instantiation for common types
template Buffer& HierarchicalComponent::AddSubcomponent<Buffer>(const String& name);
template Register4Bit& HierarchicalComponent::AddSubcomponent<Register4Bit>(const String& name);
template Register8Bit& HierarchicalComponent::AddSubcomponent<Register8Bit>(const String& name);
template ALU& HierarchicalComponent::AddSubcomponent<ALU>(const String& name);
template Counter4Bit& HierarchicalComponent::AddSubcomponent<Counter4Bit>(const String& name);
template Counter8Bit& HierarchicalComponent::AddSubcomponent<Counter8Bit>(const String& name);
template RAM16x8& HierarchicalComponent::AddSubcomponent<RAM16x8>(const String& name);

// Implementation of Cpu4Bit
Cpu4Bit::Cpu4Bit() : HierarchicalComponent("CPU4BIT") {
    SetupSubcomponents();
}

void Cpu4Bit::SetupSubcomponents() {
    alu = &AddSubcomponent<ALU>("ALU");
    reg_a = &AddSubcomponent<Register4Bit>("REG_A");
    reg_b = &AddSubcomponent<Register4Bit>("REG_B");
    accumulator = &AddSubcomponent<Register4Bit>("ACCUMULATOR");
    pc = &AddSubcomponent<Counter4Bit>("PC");
    ir = &AddSubcomponent<Counter4Bit>("IR");
    
    // Add external interface connections
    AddSink("CLK");
    AddSink("RESET");
    AddSink("ENABLE");
    
    // Add external data/address buses
    for (int i = 0; i < 4; i++) {
        AddSink(String().Cat() << "DATA_IN" << i);
        AddSource(String().Cat() << "DATA_OUT" << i).SetMultiConn();
        AddSink(String().Cat() << "ADDR" << i);
    }
    
    // Add control signals
    AddSink("READ");
    AddSink("WRITE");
    AddSink("ALU_OP0");
    AddSink("ALU_OP1");
    AddSink("ALU_OP2");
    AddSink("ALU_OP3");
}

void Cpu4Bit::ConnectSubcomponents() {
    // Internal connections would be established here
    // For this example, we're just connecting clock to all subcomponents
    // In a real implementation, complex interconnections would be defined
}

bool Cpu4Bit::Tick() {
    // Implement CPU-specific logic
    // For this example, we'll just tick all subcomponents
    
    // Tick subcomponents
    alu->Tick();
    reg_a->Tick();
    reg_b->Tick();
    accumulator->Tick();
    pc->Tick();
    ir->Tick();
    
    return true;
}

// Implementation of Cpu8Bit
Cpu8Bit::Cpu8Bit() : HierarchicalComponent("CPU8BIT") {
    SetupSubcomponents();
}

void Cpu8Bit::SetupSubcomponents() {
    alu = &AddSubcomponent<ALU>("ALU");
    reg_a = &AddSubcomponent<Register8Bit>("REG_A");
    reg_b = &AddSubcomponent<Register8Bit>("REG_B");
    accumulator = &AddSubcomponent<Register8Bit>("ACCUMULATOR");
    pc = &AddSubcomponent<Counter8Bit>("PC");
    ir = &AddSubcomponent<Counter8Bit>("IR");
    
    // Add external interface connections
    AddSink("CLK");
    AddSink("RESET");
    AddSink("ENABLE");
    
    // Add external data/address buses (8 bits each)
    for (int i = 0; i < 8; i++) {
        AddSink(String().Cat() << "DATA_IN" << i);
        AddSource(String().Cat() << "DATA_OUT" << i).SetMultiConn();
        AddSink(String().Cat() << "ADDR" << i);
    }
    
    // Add control signals
    AddSink("READ");
    AddSink("WRITE");
    AddSink("ALU_OP0");
    AddSink("ALU_OP1");
    AddSink("ALU_OP2");
    AddSink("ALU_OP3");
}

void Cpu8Bit::ConnectSubcomponents() {
    // Internal connections would be established here
    // For this example, we're just connecting clock to all subcomponents
    // In a real implementation, complex interconnections would be defined
}

bool Cpu8Bit::Tick() {
    // Implement CPU-specific logic
    // For this example, we'll just tick all subcomponents
    
    // Tick subcomponents
    alu->Tick();
    reg_a->Tick();
    reg_b->Tick();
    accumulator->Tick();
    pc->Tick();
    ir->Tick();
    
    return true;
}

// Implementation of MemoryBank
MemoryBank::MemoryBank(int banks) : HierarchicalComponent("MEM_BANK"), bank_count(banks) {
    SetupSubcomponents();
}

void MemoryBank::SetupSubcomponents() {
    // Create the specified number of memory units
    for (int i = 0; i < bank_count; i++) {
        RAM16x8& mem_unit = AddSubcomponent<RAM16x8>(String().Cat() << "MEM_UNIT_" << i);
        memory_units.Add(&mem_unit);
    }
    
    // Add external interface connections
    AddSink("CLK");
    AddSink("CS");  // Chip Select
    AddSink("WE");  // Write Enable
    AddSink("OE");  // Output Enable
    
    // Add address lines (4 bits for 16 locations per unit, but we need to handle bank selection)
    for (int i = 0; i < 8; i++) {  // Using 8 bits: 4 for bank select, 4 for address within bank
        AddSink(String().Cat() << "ADDR" << i);
    }
    
    // Add data lines (8 bits)
    for (int i = 0; i < 8; i++) {
        AddSink(String().Cat() << "DATA_IN" << i);
        AddSource(String().Cat() << "DATA_OUT" << i).SetMultiConn();
    }
}

void MemoryBank::ConnectSubcomponents() {
    // In a real implementation, we would connect address, data, and control signals
    // to the appropriate memory units based on the bank selection
}

bool MemoryBank::Tick() {
    // Tick all memory units in the bank
    for (int i = 0; i < memory_units.GetCount(); i++) {
        memory_units[i]->Tick();
    }
    
    return true;
}

// Implementation of BusController
BusController::BusController(int width, int segments) : HierarchicalComponent("BUS_CTRL"), 
                                                       bus_width(width), 
                                                       segment_count(segments) {
    SetupSubcomponents();
}

void BusController::SetupSubcomponents() {
    // Add external interface connections
    AddSink("CLK");
    AddSink("EN");  // Enable
    
    // Add connections for each bus segment
    for (int seg = 0; seg < segment_count; seg++) {
        for (int bit = 0; bit < bus_width; bit++) {
            AddSink(String().Cat() << "IN" << seg << "_" << bit);
            AddSource(String().Cat() << "OUT" << seg << "_" << bit).SetMultiConn();
        }
    }
    
    // Add control lines for bus arbitration
    for (int seg = 0; seg < segment_count; seg++) {
        AddSink(String().Cat() << "REQ" << seg);  // Request line for each segment
        AddSource(String().Cat() << "GRANT" << seg).SetMultiConn();  // Grant line for each segment
    }
}

void BusController::ConnectSubcomponents() {
    // In a real implementation, we would implement bus arbitration logic
    // and connect the appropriate input segments to output segments
}

bool BusController::Tick() {
    // Implement bus control logic
    // For this example, we'll just maintain state
    
    return true;
}