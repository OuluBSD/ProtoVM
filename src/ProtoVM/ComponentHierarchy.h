#ifndef _ProtoVM_ComponentHierarchy_h_
#define _ProtoVM_ComponentHierarchy_h_

#include "ProtoVM.h"

// Base class for hierarchical components that contain other components
class HierarchicalComponent : public ElcBase {
private:
    Vector<ElcBase*> subcomponents;  // List of contained components
    
protected:
    Pcb& internal_pcb;  // Internal PCB to hold subcomponents
    
public:
    HierarchicalComponent(const String& name);
    virtual ~HierarchicalComponent();
    
    // Add a subcomponent to this hierarchical component
    template<typename T>
    T& AddSubcomponent(const String& name);
    
    // Get internal connections for linking with external components
    virtual ElcBase& GetInternalComponent(int index) { return *subcomponents[index]; }
    
    // Override base class methods
    bool Tick() override;
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    
    // Virtual method for custom hierarchy logic
    virtual void SetupSubcomponents() {}  // Called during construction to set up subcomponents
    virtual void ConnectSubcomponents() {}  // Called during initialization to connect subcomponents internally
};

// Example hierarchical components

// 4-bit CPU core with ALU, registers, and control logic
class Cpu4Bit : public HierarchicalComponent {
private:
    // Subcomponents
    ALU* alu;                    // Arithmetic Logic Unit
    Register4Bit* reg_a;         // Register A
    Register4Bit* reg_b;         // Register B
    Register4Bit* accumulator;   // Accumulator
    Counter4Bit* pc;             // Program Counter
    Counter4Bit* ir;             // Instruction Register
    
public:
    Cpu4Bit();
    
    void SetupSubcomponents() override;
    void ConnectSubcomponents() override;
    bool Tick() override;
};

// 8-bit CPU core with ALU, registers, and control logic
class Cpu8Bit : public HierarchicalComponent {
private:
    // Subcomponents
    ALU* alu;                    // Arithmetic Logic Unit
    Register8Bit* reg_a;         // Register A
    Register8Bit* reg_b;         // Register B
    Register8Bit* accumulator;   // Accumulator
    Counter8Bit* pc;             // Program Counter
    Counter8Bit* ir;             // Instruction Register
    
public:
    Cpu8Bit();
    
    void SetupSubcomponents() override;
    void ConnectSubcomponents() override;
    bool Tick() override;
};

// Memory bank - multiple memory units managed as one
class MemoryBank : public HierarchicalComponent {
private:
    int bank_count;              // Number of memory units in the bank
    Vector<RAM16x8*> memory_units; // Individual memory units
    
public:
    MemoryBank(int banks = 4);
    
    void SetupSubcomponents() override;
    void ConnectSubcomponents() override;
    bool Tick() override;
};

// Bus controller - manages multiple bus segments
class BusController : public HierarchicalComponent {
private:
    int bus_width;               // Width of the buses
    int segment_count;           // Number of segments
    
public:
    BusController(int width = 8, int segments = 2);
    
    void SetupSubcomponents() override;
    void ConnectSubcomponents() override;
    bool Tick() override;
};

// Generic N-bit component container
template<int N>
class NBitComponent : public HierarchicalComponent {
private:
    Vector<ElcBase*> bits;       // Individual bit components
    
public:
    NBitComponent(const String& name) : HierarchicalComponent(name) {
        SetName(name);
        SetupSubcomponents();
    }
    
    void SetupSubcomponents() override {
        // Create N individual bit components (using buffers as placeholders)
        for (int i = 0; i < N; i++) {
            Buffer& buf = AddSubcomponent<Buffer>(String().Cat() << "bit_" << i);
            bits.Add(&buf);
        }
    }
    
    void ConnectSubcomponents() override {
        // Connect the subcomponents internally if needed
        // Implementation depends on specific use case
    }
    
    bool Tick() override {
        // Tick all internal components
        for (int i = 0; i < bits.GetCount(); i++) {
            bits[i]->Tick();
        }
        return true;
    }
};

// 8-bit version of the N-bit component
using Bit8Component = NBitComponent<8>;

// 16-bit version of the N-bit component
using Bit16Component = NBitComponent<16>;

#endif