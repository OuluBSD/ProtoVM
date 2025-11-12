# ProtoVM - Enhanced Digital Logic Simulator

## Overview

This is an enhanced version of ProtoVM, a custom digital logic simulator that models real hardware components. The system has been significantly improved to properly handle the complexities of digital circuit behavior.

## Key Features

### 1. Convergence-Based Simulation
- Multi-tick algorithm that iterates until signals stabilize
- Proper handling of feedback loops and signal propagation
- Oscillation detection to prevent infinite loops

### 2. Enhanced Component Architecture
- Tri-state bus support for realistic data bus modeling
- Improved change detection to optimize processing
- Better integration between components (CPU, RAM, ROM, Logic Gates)

### 3. Realistic Hardware Modeling
- Proper bus arbitration with tri-state buffers
- Accurate signal propagation modeling
- Component state tracking for optimization

## Major Improvements

Compared to the original version, this enhanced ProtoVM includes:

1. **Multi-Tick Convergence Algorithm**: Instead of processing each component once per tick, the system now iterates until all signals have stabilized, similar to how real digital circuits behave.

2. **Tri-State Bus Support**: Buses now properly model tri-state logic, allowing multiple components to share the same bus lines while preventing electrical conflicts.

3. **Optimized Processing**: Components now track their internal state changes and only trigger reprocessing when actually needed, improving simulation performance.

4. **Feedback Loop Handling**: The system correctly handles combinatorial feedback that can occur in digital circuits, preventing incorrect behavior.

5. **Enhanced Debugging**: Improved logging and state reporting make it easier to understand what's happening during simulation.

## Components Included

- **6502 CPU Emulation** (IC6502): Accurate modeling of the classic MOS 6502 processor
- **Memory Components** (ICRamRom): Support for both ROM and RAM configurations
- **Logic Gates** (ElcNand, ElcNor, etc.): Basic digital logic building blocks
- **Buses**: Address and data buses with proper tri-state support
- **Support Components**: Pull-up resistors, capacitors, and other digital circuit elements

## Usage

The system can be built and run with:
```bash
./run.sh
```

This will execute a test scenario that demonstrates the enhanced digital simulation capabilities.

## Technical Details

### Simulation Engine
- **Machine Class**: Central coordinator that manages component interactions
- **Convergence Algorithm**: Iterates until signal stability is achieved
- **Change Detection**: Tracks component state changes to optimize processing

### Component Architecture
- **ElectricNodeBase**: Base class for all electronic components
- **Bus Classes**: Specialized components for address/data bus handling
- **IC Classes**: Integrated circuit implementations (CPU, memory, etc.)

### Signal Propagation
- **Tri-State Logic**: Proper modeling of bus drivers and receivers
- **Signal Integrity**: Prevention of bus contention and electrical conflicts
- **Timing Accuracy**: More realistic modeling of signal propagation delays

## Future Development

Potential areas for further enhancement include:
- Detailed timing analysis and constraint checking
- Advanced bus protocols (SPI, I2C, etc.)
- More complex processor architectures
- Integration with formal verification tools
- Graphical visualization of circuit behavior