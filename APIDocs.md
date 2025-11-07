# ProtoVM Digital Logic Simulator - API Documentation

## Overview
ProtoVM is a digital logic simulator that allows you to model and simulate digital circuits including CPUs, memory, buses, and basic logic gates. This documentation covers the API for component creation and simulation.

## Core Architecture

### Machine Class
The `Machine` class is the central coordinator that manages component interactions and executes the simulation algorithm.

Key methods:
- `bool Init()` - Initialize the machine and check connections
- `bool Tick()` - Execute one simulation tick
- `Pcb& AddPcb()` - Add a new printed circuit board to the machine
- `ScheduleEvent(int delay, std::function<bool()> action)` - Schedule an event with delay

### ElectricNodeBase Class
The base class for all electronic components in the simulation.

Key methods:
- `bool Tick()` - Called for each simulation tick
- `bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id)` - Process operations between components
- `bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits)` - Receive raw data from other components

### Pcb Class
Represents a printed circuit board containing multiple components.

Key methods:
- `template<typename T> T& Add(const String& name)` - Add a component to the PCB

## Creating Custom Components

To create a custom component, inherit from `ElcBase` (which inherits from `ElectricNodeBase`) and implement the required virtual methods.

### Example Component Implementation

```cpp
class MyComponent : public ElcBase {
private:
    bool input = 0;
    bool output = 0;

public:
    MyComponent() {
        // Define input and output connectors
        AddSink("IN");      // Input connector
        AddSource("OUT").SetMultiConn();  // Output connector (can connect to multiple destinations)
    }

    bool Tick() override {
        // Update internal state based on inputs
        output = !input;  // For example, create an inverter
        return true;
    }

    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, 
                 ElectricNodeBase& dest, uint16 dest_conn_id) override {
        if (type == WRITE) {
            if (conn_id == 1) {  // OUT connector (conn_id 0 is IN, 1 is OUT)
                return dest.PutRaw(dest_conn_id, (byte*)&output, 0, 1);
            }
        }
        return true;
    }

    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override {
        if (conn_id == 0) {  // IN connector
            input = (*data & 1) ? true : false;
        }
        return true;
    }
};
```

### Adding Connectors

Use these methods to define input and output connections:
- `AddSink(name)` - Define an input connector
- `AddSource(name)` - Define an output connector 
- `AddBidirectional(name)` - Define a bidirectional connector
- `.SetMultiConn()` - Allow multiple connections to this source

### Process Types

- `ProcessType::WRITE` - Data flowing from source to sink
- `ProcessType::TICK` - Component state update
- `ProcessType::READ` - Data flow in reverse direction (rarely used)

## Standard Components

### Basic Logic Gates
- `ElcNand`, `ElcNor`, `ElcNot`, `ElcXor`, `ElcXnor` - 2-input logic gates
- `AndGate3`, `AndGate4`, `OrGate3`, `OrGate4` - Multi-input gates from StandardLibrary

### Memory Components
- `Register4Bit`, `Register8Bit` - Storage registers
- `FlipFlopD`, `FlipFlopJK` - Flip-flop components
- `RAM16x8`, `ROM16x8` - Memory components from StandardLibrary

### Arithmetic Components
- `ALU` - Arithmetic Logic Unit with multiple operations
- `FullAdder`, `Adder4Bit`, `AdderSubtractor4Bit` - Arithmetic units

### Multiplexing Components
- `Mux2to1`, `Mux4to1` - Multiplexers
- `Demux1to2`, `Demux1to4` - Demultiplexers
- `Decoder2to4`, `Decoder3to8` - Decoders
- `Encoder4to2`, `Encoder8to3` - Encoders

### Clock Components
- `ClockDivider` - Divide clock frequency
- `ClockGate` - Control clock propagation
- `PLL` - Phase-locked loop for frequency synthesis

## Component Hierarchy

Complex components can be built using the `HierarchicalComponent` class:

```cpp
class MyComplexCircuit : public HierarchicalComponent {
public:
    MyComplexCircuit() : HierarchicalComponent("MY_COMPLEX_CIRCUIT") {
        SetupSubcomponents();
    }
    
    void SetupSubcomponents() override {
        // Add subcomponents to the internal PCB
        alu = &AddSubcomponent<ALU>("ALU");
        reg_a = &AddSubcomponent<Register8Bit>("REG_A");
        reg_b = &AddSubcomponent<Register8Bit>("REG_B");
    }
    
    bool Tick() override {
        // Tick all subcomponents
        alu->Tick();
        reg_a->Tick();
        reg_b->Tick();
        return true;
    }
};
```

## Timing and Synchronization

### Clock Domains
- Each component can be assigned to a clock domain using `SetClockDomain()`
- Different clock domains can run at different frequencies
- Use `CreateClockDomain()` to create new clock domains in the Machine

### Timing Analysis
- Use `TimingAnalyzer` to analyze propagation delays and timing paths
- Check setup and hold time requirements with `CheckSetupHoldTimes()`

## Simulation Features

### Signal Tracing
- Use `AddSignalToTrace()` to monitor specific signals
- View transitions with `ShowSignalTraceLog()` in the CLI

### Performance Profiling
- Use `StartProfiling()` and `StopProfiling()` to measure performance
- Get results with `ReportProfilingResults()`

### Breakpoints
- Add breakpoints with `AddBreakpoint(tick_number)`
- Check status with `IsPaused()` and resume with `Resume()`

## CLI Commands

Interactive commands when using `--cli` option:
- `list` - List all components
- `inspect <component_name>` - Show detailed component state
- `state` - Show all component states
- `run [n]` - Run simulation for n ticks
- `trace <component> <pin>` - Add signal trace
- `tracelog` - Show signal transition log
- `visualize` - Show circuit connections
- `netlist` - Generate circuit netlist

## Best Practices

1. **Component Design**:
   - Always initialize connector values in constructor
   - Check all necessary conn_id values in PutRaw and Process methods
   - Use proper connector naming conventions

2. **State Management**:
   - Store internal state in member variables
   - Update state in Tick() method
   - Use HasChanged() and SetChanged() for optimization

3. **Connection Handling**:
   - Verify all pins are properly connected before simulation
   - Use SetMultiConn() for outputs that might connect to multiple inputs
   - Handle both directions of data flow appropriately

4. **Timing Considerations**:
   - Model propagation delays where appropriate
   - Consider setup and hold time requirements
   - Be aware of clock domain crossings

5. **Performance**:
   - Use change detection to avoid unnecessary computations
   - Consider using topological ordering for component evaluation
   - Implement proper reset behavior for components

## Troubleshooting

- If getting connection errors, ensure all required pins are connected
- For oscillation warnings, check for combinational loops in your circuit
- For timing issues, verify clock domains and signal propagation paths
- Use CLI inspection commands to debug component states