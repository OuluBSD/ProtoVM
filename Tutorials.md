# ProtoVM Tutorials

This document provides step-by-step tutorials for building complex circuits with ProtoVM.

## Tutorial 1: Building a Simple 4-bit Adder

In this tutorial, we'll build a 4-bit binary adder using basic logic gates.

### Step 1: Understanding the Components
A 4-bit adder consists of 4 full adders. Each full adder takes 3 inputs (A, B, Carry-in) and produces 2 outputs (Sum, Carry-out).

### Step 2: Creating a Full Adder Component
First, let's create a full adder using basic gates:

```cpp
class FullAdder : public ElcBase {
private:
    bool a, b, cin;  // Inputs
    bool sum, cout;  // Outputs

public:
    FullAdder() {
        AddSink("A");
        AddSink("B"); 
        AddSink("CIN");
        AddSource("SUM").SetMultiConn();
        AddSource("COUT").SetMultiConn();
    }

    bool Tick() override {
        sum = a ^ b ^ cin;  // XOR for sum
        cout = (a & b) | (cin & (a ^ b));  // Carry logic
        return true;
    }

    // Implementation of Process and PutRaw methods...
};
```

### Step 3: Creating the 4-bit Adder
Now we'll create a 4-bit adder by connecting 4 full adders:

```cpp
class Adder4Bit : public ElcBase {
private:
    FullAdder fa[4];  // Four full adders
    bool a[4], b[4];  // Input bits
    bool s[4];        // Sum bits
    bool carry_in, carry_out;  // Carry in/out

public:
    Adder4Bit() {
        // Input connections
        for(int i = 0; i < 4; i++) {
            AddSink(String().Cat() << "A" << i);
            AddSink(String().Cat() << "B" << i);
            AddSource(String().Cat() << "S" << i).SetMultiConn();
        }
        AddSink("CIN");
        AddSource("COUT").SetMultiConn();
    }

    bool Tick() override {
        bool carry = carry_in;
        for(int i = 0; i < 4; i++) {
            // Connect carry between full adders
            // Implementation connects inputs to full adders
            // and propagates carry from one to the next
        }
        carry_out = carry;  // Final carry out
        return true;
    }
};
```

## Tutorial 2: Building a Memory Component

Here we create a simple 4x4 memory (4 locations, 4 bits each):

### Step 1: Design the Memory Structure
- Address inputs: 2 bits to select one of 4 locations
- Data inputs: 4 bits to write
- Data outputs: 4 bits to read
- Control: Read/Write enable

### Step 2: Implementation
```cpp
class SimpleMemory : public ElcBase {
private:
    byte memory[4];  // 4 memory locations
    bool addr[2];    // Address (2 bits)
    bool din[4];     // Data input (4 bits)
    bool dout[4];    // Data output (4 bits)
    bool we;         // Write enable

public:
    SimpleMemory() {
        // Address inputs
        AddSink("ADDR0");
        AddSink("ADDR1");
        // Data I/O
        for(int i = 0; i < 4; i++) {
            AddSink(String().Cat() << "DIN" << i);
            AddSource(String().Cat() << "DOUT" << i).SetMultiConn();
        }
        AddSink("WE");  // Write enable
    }

    bool Tick() override {
        int addr_val = (addr[1] ? 2 : 0) + (addr[0] ? 1 : 0);
        
        if(we) {  // Write operation
            memory[addr_val] = 0;
            for(int i = 0; i < 4; i++) {
                if(din[i]) memory[addr_val] |= (1 << i);
            }
        }
        
        // Read operation (occurs every tick)
        byte data = memory[addr_val];
        for(int i = 0; i < 4; i++) {
            dout[i] = (data >> i) & 1;
        }
        return true;
    }
};
```

## Tutorial 3: Building a Simple CPU

A minimal CPU consists of:
- Arithmetic Logic Unit (ALU)
- Registers
- Control unit
- Program counter
- Instruction decoder

### Implementation Strategy:
1. Create each component separately (ALU, registers, etc.)
2. Use HierarchicalComponent to connect them together
3. Implement the execution cycle: Fetch-Decode-Execute

```cpp
class SimpleCPU : public HierarchicalComponent {
private:
    ALU* alu;
    Register8Bit* accumulator;
    Register8Bit* program_counter;
    // Other registers and control logic...

public:
    SimpleCPU() : HierarchicalComponent("SIMPLE_CPU") {
        SetupSubcomponents();
    }

    void SetupSubcomponents() override {
        alu = &AddSubcomponent<ALU>("ALU");
        accumulator = &AddSubcomponent<Register8Bit>("ACCUMULATOR");
        program_counter = &AddSubcomponent<Register8Bit>("PC");
        // Add other components...
    }

    bool Tick() override {
        // Execute one CPU cycle
        // 1. Fetch instruction (not shown in detail here)
        // 2. Decode instruction
        // 3. Execute instruction
        return true;
    }
};
```

## Tutorial 4: Advanced: Building a Complete Computer System

Let's combine multiple components to create a simple computer system:

1. CPU (as built in Tutorial 3)
2. Memory (as built in Tutorial 2)
3. I/O devices
4. Clock generation
5. Reset circuitry

### System Integration:
- Connect CPU data bus to memory data bus
- Connect CPU address bus to memory address bus
- Add memory mapping and decoding
- Implement clock distribution
- Add reset distribution

This approach demonstrates how to build increasingly complex systems from basic components in ProtoVM.

## Best Practices for Circuit Design

1. **Start Small**: Begin with simple components and test them individually before integration
2. **Use Hierarchical Design**: Group related components into subsystems
3. **Plan Connections Carefully**: Draw a block diagram before implementing
4. **Test Incrementally**: Verify each stage of your design
5. **Document Your Design**: Use meaningful names for components and connections
6. **Consider Timing**: Be aware of signal propagation delays in complex circuits
7. **Use Simulation Tools**: Take advantage of the tracing and analysis tools in ProtoVM