# Tasks for CADC Implementation

## Overview
This document outlines the tasks required to implement a complete CADC (Central Air Data Computer) system similar to the minimax4004 approach.

## Main Implementation Tasks

### 1. Core CADC Component Implementation
- [ ] Implement the Parallel Multiplier Unit (PMU) with Booth's algorithm
- [ ] Implement the Parallel Divider Unit (PDU) with non-restoring division
- [ ] Implement the Special Logic Function (SLF) with data limiting and logic operations
- [ ] Implement the Steering Logic Unit (SLU) with addressable routing
- [ ] Implement Random Access Storage (RAS) with 16-word storage
- [ ] Implement Read-Only Memory (ROM) with microcode storage

### 2. System Integration
- [ ] Create a main CADC system class integrating all components
- [ ] Implement system timing with 375 kHz clock and 20-bit word structure
- [ ] Implement pipeline concurrency for the three processing modules
- [ ] Add proper serialization/deserialization for serial data processing
- [ ] Implement system executive control for input/output management

### 3. MinimaxCADC Circuit
- [ ] Create a minimaxCADC circuit similar to minimax4004 but using CADC components
- [ ] Implement proper bus connections between CADC modules
- [ ] Add clock generation and control logic for CADC timing
- [ ] Implement proper reset and control signals

### 4. Testing and Validation
- [ ] Create unit tests for individual CADC components
- [ ] Create integration tests for the complete CADC system
- [ ] Implement polynomial evaluation tests (the primary CADC application)
- [ ] Validate timing behavior and pipeline operation

### 5. CADC Binary and Test Programs
- [ ] Create cadc_putchar.bin example program similar to 4004_putchar.bin
  - Should contain microcode for basic polynomial evaluation: F(X) = a3*x^3 + a2*x^2 + a1*x + a0
  - Should demonstrate PMU (multiplier) and SLF (adder) working in pipeline
  - Should simulate computing basic air data parameter
- [ ] Implement basic CADC algorithms in microcode
- [ ] Create test programs for polynomial evaluation
- [ ] Implement air data computation examples

### 6. Shell Scripts for CADC
- [ ] Create run_cadc_demo.sh similar to run_4004_demo.sh
- [ ] Create run_cadc_program.sh similar to run_4004_program.sh
- [ ] Create run_cadc_tests.sh similar to run_4004_tests.sh
- [ ] Create test_cadc_output.sh similar to test_4004_output.sh

### 7. Documentation and Examples
- [ ] Update README with CADC information
- [ ] Create CADC tutorial documentation
- [ ] Add CADC examples to the tutorial section
- [ ] Update API documentation with CADC components