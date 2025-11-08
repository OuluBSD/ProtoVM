# How to Write and Assemble Programs for Intel 4004

This document provides instructions on how to write and assemble programs for the Intel 4004 microprocessor using the HLA3 assembler kit.

## Overview of the 4004 Architecture

The Intel 4004 is a 4-bit microprocessor with:
- 12-bit address space (4096 bytes)
- 4-bit data bus
- 16 registers (4-bit each): R0-R15
- 45 instructions
- Stack for subroutine calls (3 levels)
- Dedicated 4-bit accumulator (A register)

## Writing 4004 Assembly Programs

### Basic Syntax

Assembly programs for 4004 follow this general format:

```
[label:] mnemonic [operands] [; comment]
```

### Example Program Structure

```
; Example 4004 Assembly Program
    ORG $000        ; Set program origin address
START:
    LDM #5          ; Load immediate value 5 into accumulator
    WRM             ; Write accumulator to memory at R0-R1
    JCN NZ, LOOP    ; Jump to LOOP if accumulator is not zero
    HLT             ; Halt program (custom instruction)
    
LOOP:
    RDM             ; Read memory at R0-R1 to accumulator
    SBM             ; Subtract memory (at R0-R1) from accumulator
    WRM             ; Write result back to memory
    JUN START       ; Jump unconditionally to START
```

## Assembler Directives

### Origin Directive
```
ORG $nnn            ; Set the starting address for code/data
```

### Data Definition Directives
```
DB value            ; Define byte (4-bit value for 4004)
DW value            ; Define word (8-bit value for 4004)
DS count            ; Define storage (skip locations)
```

### End Directive
```
END                 ; End of program
```

## Addressing Modes

The 4004 supports several addressing modes:

1. **Immediate Addressing**: `LDM #nn` - Load immediate value
2. **Register Addressing**: `ADD Rn` - Operate on register n
3. **Indirect Memory Addressing**: Memory operations use R0-R1 as address pointer

## Essential Instructions

### Arithmetic Instructions
- `LDM #nn`: Load accumulator with immediate value
- `ADD Rn`: Add register n to accumulator
- `SBM`: Subtract memory from accumulator
- `CLC`: Clear carry flag
- `CLB`: Clear both carry flags

### Control Flow Instructions
- `JCN cc, addr`: Conditional jump based on condition cc
- `JUN addr`: Jump unconditionally
- `JMS addr`: Jump to subroutine
- `FIM Rn, #nn`: Fetch immediate, load 8-bit value into register pair

### I/O Instructions
- `RD0`, `RD1`, `RD2`, `RD3`: Read ROM digit
- `WR0`, `WR1`, `WR2`, `WR3`: Write to output register
- `WRM`: Write accumulator to memory
- `RDM`: Read memory to accumulator

### Memory Instructions
- `WRM`: Write accumulator to memory (at R0-R1 address)
- `RDM`: Read memory (at R0-R1 address) to accumulator
- `WBN Rn`: Write register Rn to memory

## Assembly Process

### Creating the Assembly File

1. Write your assembly code in a text file with `.asm` or `.s` extension
2. Use proper syntax and addressing modes
3. Ensure all labels and addresses are valid for 12-bit addressing space

Example assembly file (`program.asm`):
```
    ORG $000
MAIN:
    LDM #0          ; Initialize accumulator
    WRM             ; Store in memory
    LDM #1          ; Load value to add
    ADD R3          ; Add register 3
    WRM             ; Store result
    JCN NZ, MAIN    ; Loop if not zero
    HLT             ; Halt
```

### Assembling the Program

Using the HLA3 assembler:

```bash
cd external/hla3/
./hla3 -4004 -o program.obj program.asm
```

This generates:
- `program.obj`: Object file with machine code
- `program.lst`: Listing file with addresses and opcodes

## Loading Programs into the Simulator

### Binary File Format

The 4004 simulator expects programs in binary format, where each byte represents a 4-bit instruction (packed as two instructions per byte) or data.

Example binary structure:
```
Byte 0: [Instruction 1 (4 bits)] [Instruction 2 (4 bits)]
Byte 1: [Instruction 3 (4 bits)] [Instruction 4 (4 bits)]
...
```

### Using the Simulator's ROM Loading

The simulator's IC4001 component supports loading programs:

1. Convert assembled object code to the appropriate format
2. Load using the simulator's ROM interface
3. Set initial register values if needed
4. Execute the program

## Programming Tips and Best Practices

### Register Usage Convention
- R0-R1: Memory address pointer
- R2-R3: General purpose or second address pointer
- R4-R7: Temporary storage
- R8-R15: Application-specific use

### Memory Organization
- Address $000-$0FF: System area
- Address $100-$1FF: I/O ports
- Address $200-$FFF: Program and data area

### Handling the 4-bit Limitation
- Operations are 4-bit, so plan for multiple operations to handle larger values
- Use register pairs for 8-bit operations (e.g., R0R1 for address)
- Chain operations for larger calculations

### Subroutine Management
- Use the 3-level stack efficiently
- Preserve registers that need to be maintained across calls
- Ensure return addresses are managed properly

## Debugging Tips

### Common Issues
1. Address space limitations - ensure addresses fit in 12 bits
2. Register overflow - 4-bit values wrap at 16
3. Stack overflow - limited to 3 levels of nesting
4. Carry flag management in arithmetic operations

### Debugging Commands
The simulator CLI typically supports:
- `step` - Execute one instruction
- `registers` - Display register values
- `memory` address - Display memory contents
- `breakpoint` address - Set breakpoint at address

## Example Workflow

1. **Write** assembly code in `myprogram.asm`
2. **Assemble** with HLA3: `hla3 -4004 myprogram.asm`
3. **Convert** to simulator format
4. **Load** into simulator's ROM
5. **Run** and debug as needed
6. **Optimize** and repeat

## Conclusion

Writing programs for the Intel 4004 requires understanding its 4-bit architecture and limited resources. With careful planning and efficient use of registers and memory, you can create functional programs despite the constraints of this early microprocessor.