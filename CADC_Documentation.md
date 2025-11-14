# F-14 CADC (Central Air Data Computer) Architecture Documentation

## Overview

The F-14 CADC (Central Air Data Computer) was developed by Garrett AiResearch for the F-14 Tomcat. This implementation in ProtoVM simulates the key features of the real hardware, including its chipset approach and specialized algorithms for air data computation.

## Architecture

### Chipset Components

The CADC uses a chipset approach with multiple specialized chips:

- **PMU (Parallel Multiplier Unit)**: Performs multiplication operations using Booth's algorithm
- **PDU (Parallel Divider Unit)**: Performs division operations using non-restoring algorithm
- **SLF (Special Logic Function)**: Implements data limiting, transfer functions, and logical operations
- **SLU (Steering Logic Unit)**: Handles data routing between modules
- **RAS (Random Access Storage)**: 16-word storage for temporary values
- **ROM (Read-Only Memory)**: Stores microcode for computations

### Timing and Serialization

- **Clock Frequency**: 375 kHz
- **Word Length**: 20 bits (19 data bits + 1 sign bit, two's complement)
- **Bit Time**: 2.66μs (at 375kHz)
- **Word Time**: 53.2μs for 20-bit serial processing
- **Word Types**: W0 (instruction fetch), W1 (data transfer)

## Implementation in ProtoVM

### CadcSystem Class

The `CadcSystem` class implements the complete CADC system with:

- Three pipeline modules (Multiply, Divide, Special Logic)
- System Executive Control
- Interconnection between modules
- Timing coordination
- Polynomial evaluation algorithms
- Air data computation algorithms

### Polynomial Evaluation

The implementation includes optimized polynomial evaluation using Horner's method:

```
F(X) = a_n*x^n + a_(n-1)*x^(n-1) + ... + a_1*x + a_0
```

The computation is: `((a_n*x + a_(n-1))*x + a_(n-2))*x + ... + a_1)*x + a_0`

### Air Data Computation Algorithms

The CADC computes key flight parameters:

- **Altitude**: From pressure altitude and temperature
- **Vertical Speed**: Rate of altitude change
- **Air Speed**: From impact and static pressure
- **Mach Number**: From air speed and temperature

## Usage

### Circuit Selection

The CADC system can be selected using the `minimaxcadc` circuit option:

```bash
./ProtoVM minimaxcadc
```

### MinimaxCADC Circuit

The `MinimaxCADC` circuit provides a complete computer system using the CADC architecture:

- Three pipeline modules (Multiply, Divide, Special Logic)
- System Executive Control
- Timing and control logic
- Input/Output mechanisms

### API

The following methods are available on the `CadcSystem` class:

- `EvaluatePolynomial(int20 x, const int20* coefficients, int degree)`: Evaluate polynomial using Horner's method
- `ComputeAltitude(int20 pressure_altitude, int20 temperature)`: Calculate altitude from pressure and temperature
- `ComputeVerticalSpeed(int20 altitude_old, int20 altitude_new)`: Calculate rate of altitude change
- `ComputeAirSpeed(int20 impact_pressure, int20 static_pressure)`: Calculate airspeed from pressure readings
- `ComputeMachNumber(int20 air_speed, int20 temperature)`: Calculate Mach number from airspeed and temperature

## Key Features

### Pipeline Concurrency

The CADC architecture implements pipeline concurrency with three processing modules that can operate simultaneously on different parts of a computation.

### Polynomial Evaluation

Specialized algorithms for evaluating polynomials that are common in air data computations, such as those used for:
- Pressure altitude calculations
- Air data corrections
- Calibration functions

### Real-time Computation

Designed for flight control applications with optimized algorithms for real-time air data parameter computation.

## Pin Interface

The `MinimaxCADC` circuit provides the following pin interface:

- Input sensors: `PRESSURE_IN`, `TEMP_IN`, `ANGLE_OF_ATTACK`
- Control inputs: `START`, `RESET`
- Status outputs: `BUSY`, `VALID_OUTPUT`
- Data outputs: `ALTITUDE_OUT`, `VERTICAL_SPEED_OUT`, `AIR_SPEED_OUT`, `MACH_NUMBER_OUT`
- System clock: `SYS_CLK`

## Implementation Details

### Convergence Algorithm

The system implements a multi-tick convergence algorithm that processes components until signals stabilize, properly modeling real digital circuit behavior with feedback loops.

### Bus Arbitration

Proper tri-state bus support with driver arbitration to prevent electrical conflicts and model realistic shared bus behavior.

### Timing Constraints

Modeling of setup and hold time requirements for accurate timing behavior and performance characteristics.