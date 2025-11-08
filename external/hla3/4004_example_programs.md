# Intel 4004 Example Programs

This document contains example programs that demonstrate the functionality of the Intel 4004 microprocessor.

## Program 1: Simple Counter

A basic counter that increments a value and stores it in memory.

```
; Simple Counter Program for Intel 4004
; Counts from 0 to 9 and loops back

    ORG $000        ; Program starts at address $000

START:
    LDM #0          ; Load accumulator with 0
    WRM             ; Write to memory (address in R0-R1)
    INC R0          ; Increment address pointer (simplified)

LOOP:
    RDM             ; Read memory to accumulator
    LDM #1          ; Load value 1
    ADD R3          ; Add 1 (assuming R3 contains 1)
    WRM             ; Write back to memory
    JCN NZ, LOOP    ; Jump to LOOP if not zero (simplified)
    
    ; Reset counter to 0
    LDM #0          ; Load 0
    WRM             ; Write to memory
    JUN START       ; Jump to start
    
    ; Program constants
    #1 DATA: R3 = 1 ; R3 contains value 1
```

## Program 2: Fibonacci Sequence

A program that calculates Fibonacci numbers and stores them in memory.

```
; Fibonacci Sequence Generator for Intel 4004
; Calculates Fibonacci numbers up to a certain value

    ORG $100        ; Program starts at address $100

FIBONACCI:
    ; Initialize first two Fibonacci numbers
    LDM #0          ; First number: 0
    WRM             ; Write to memory at R0-R1
    INC R0          ; Increment address pointer
    
    LDM #1          ; Second number: 1
    WRM             ; Write to memory
    INC R0          ; Increment address pointer
    
    ; Calculate next Fibonacci numbers
    LDM #0          ; Load first number
    WRM             ; Write to temp location
    LDM #1          ; Load second number
    WRM             ; Write to temp location
    
FIB_LOOP:
    ; Add last two numbers to get next Fibonacci number
    RDM             ; Read last number
    ADD R3          ; Add previous number (in R3)
    WRM             ; Write result to memory
    INC R0          ; Increment address pointer
    
    ; Update previous numbers
    ; (This would involve more complex register manipulation)
    
    ; Check if we've reached our limit
    ; For simplicity, we'll just loop a fixed number of times
    JCN NZ, FIB_LOOP
    
    HLT             ; Halt program (would be custom instruction)
```

## Program 3: Simple I/O Control

A program that reads input and writes to output based on conditions.

```
; Simple I/O Control for Intel 4004
; Reads inputs and controls outputs based on conditions

    ORG $200        ; Program starts at address $200

IO_CONTROL:
    ; Initialize
    LDM #0          ; Clear accumulator
    WMP             ; Write to memory port (initialize)
    
CHECK_INPUT:
    RD0             ; Read input 0
    JCN NZ, OUTPUT_HIGH  ; Jump if input is high
    
    ; Input is low, set output low
    LDM #0          ; Load low value
    WR0             ; Write low to output 0
    JUN CHECK_INPUT    ; Loop back
    
OUTPUT_HIGH:
    LDM #1          ; Load high value
    WR0             ; Write high to output 0
    JUN CHECK_INPUT    ; Loop back
```

## Program 4: Memory Copy Routine

A program that copies data from one memory location to another.

```
; Memory Copy Routine for Intel 4004
; Copies data from source address to destination address

    ORG $300        ; Program starts at address $300

COPY_ROUTINE:
    ; Initialize source and destination addresses
    ; (Assumes addresses are preloaded in R0-R1 and R2-R3)
    
COPY_LOOP:
    RDM             ; Read from source (address in R0-R1)
    ; Store in temporary register
    SRC R4          ; Send to register R4 temporarily
    
    ; Switch to destination address
    ; (This would involve setting R0-R1 to destination)
    
    LDM R4          ; Load value from R4
    WRM             ; Write to destination (address in R0-R1)
    
    ; Increment both source and destination addresses
    INC R0          ; Increment source address
    INC R2          ; Increment destination address
    
    ; Check if we've copied enough data
    ; (This would involve comparing with a counter)
    JCN NZ, COPY_LOOP  ; Continue if not done
    
    ; Terminate
    HLT             ; Halt (would be custom instruction)
```

## Program 5: Basic Arithmetic Calculator

A program that performs basic arithmetic operations.

```
; Basic Arithmetic Calculator for Intel 4004
; Performs simple addition/subtraction based on input

    ORG $400        ; Program starts at address $400

CALCULATOR:
    ; Initialize
    LDM #0          ; Clear accumulator
    WRM             ; Clear memory at R0-R1
    
    ; Get first operand
    RD0             ; Read first operand
    WRM             ; Store at current address
    
    ; Get operation (1 for add, 0 for subtract)
    RD1             ; Read operation code
    
    ; Get second operand
    RD2             ; Read second operand
    
    ; Perform operation based on operation code
    JCN NZ, ADD_OP  ; If operation code != 0, do addition
    
    ; Subtraction: first - second
    RDM             ; Get first operand
    SBM             ; Subtract memory (second operand)
    WRM             ; Store result
    JUN DISPLAY     ; Jump to display
    
ADD_OP:
    RDM             ; Get first operand
    ADD R3          ; Add second operand (in R3)
    WRM             ; Store result
    
DISPLAY:
    ; Output result
    RDM             ; Get result from memory
    WR3             ; Write to output
    JUN CALCULATOR  ; Loop to do another calculation
```

## Notes on Assembly Syntax

These example programs use a simplified assembly syntax for the Intel 4004. In a real assembler:

1. Labels would be resolved to actual addresses
2. Instructions would be converted to proper opcodes
3. Memory addresses would be properly handled using the R0-R1 addressing mechanism
4. The actual 4004 instruction set has specific constraints and formats that would need to be followed