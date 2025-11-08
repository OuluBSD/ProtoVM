# Intel 4004 Assembler Code Generation

This document outlines how code generation would work for an Intel 4004 assembler, including opcode mappings and machine code generation.

## 4004 Instruction Set Architecture

The Intel 4004 has 45 instructions with a 12-bit address space. Instructions are either single-byte (8 bits) or double-byte (16 bits).

## Opcode Mapping

### Single-Byte Instructions (1xxx format)
- `NOP` (0x00) - No Operation
- `JCN` (0x1xxx) - Conditional Jump: 1xxx where x is the condition
  - `JCN C` - Jump if Carry (0x16)
  - `JCN NC` - Jump if No Carry (0x17)
  - `JCN Z` - Jump if Zero (0x14)
  - `JCN NZ` - Jump if Not Zero (0x15)

### Single-Byte Instructions (2xxx format) - Register Operations
- `FIM` (0x2xxx) - Fetch Immediate: 2x where x is register pair (R0R1, R2R3, etc.)

### Single-Byte Instructions (4xxx format) - Subroutine Calls
- `JMS` (0x4xxx) - Jump to Subroutine: 4x where x is register pair

### Single-Byte Instructions (5xxx format) - Memory Operations
- `RDM` (0x50) - Read Memory

### Single-Byte Instructions (6xxx-7xxx format) - I/O Operations
- `RD0`, `RD1`, `RD2`, `RD3` (0x60-0x63) - Read ROM Digit
- `WR0`, `WR1`, `WR2`, `WR3` (0x70-0x73) - Write to Output Register

### Single-Byte Instructions (8xxx format) - Memory Write
- `WRM` (0x80) - Write Memory

### Single-Byte Instructions (Axxx-E0xx format) - Arithmetic
- `SBM` (0xA0) - Subtract Memory from Accumulator
- `CLB` (0xE0) - Clear Both carry flags
- `CLC` (0xF0) - Clear Carry flag

## Machine Code Generation Process

The assembler would convert assembly instructions to machine code as follows:

1. **Parse instruction and operands**
2. **Lookup opcode in mapping table**
3. **Validate addressing mode and operands**
4. **Generate binary instruction**
5. **Handle address resolution for jumps and calls**

## Example Code Generation

### Assembly Code:
```
    ORG $000
START:
    LDM #5          ; Load 5 into accumulator
    ADD R3          ; Add register 3 to accumulator
    JCN C, $100     ; Jump to $100 if carry
    JMS $200        ; Call subroutine at $200
    WRM             ; Write accumulator to memory
    HLT             ; Halt (would be custom instruction)
```

### Generated Machine Code:
```
$000: [LDM opcode + immediate data 5]
$001: [ADD R3 opcode]
$002: [JCN C opcode + high byte of $100]
$003: [low byte of $100]
$004: [JMS opcode + high byte of $200]
$005: [low byte of $200]
$006: [WRM opcode]
```

## Code Generation Functions

The assembler would include functions like:

```cpp
// Generate machine code for a single instruction
uint16 GenerateInstruction(const Instruction& instr) {
    switch(instr.opcode) {
        case OP_LDM:
            return 0x00 | (instr.immediate & 0x0F);  // Example
        case OP_JCN:
            return 0x10 | (instr.condition << 4) | ((instr.address >> 8) & 0x0F);
        // ... other cases
    }
    return 0; // Invalid instruction
}

// Handle multi-byte instructions
void GenerateMultiByteInstruction(const Instruction& instr, uint8* output) {
    uint16 code = GenerateInstruction(instr);
    output[0] = (code >> 8) & 0xFF;  // High byte
    output[1] = code & 0xFF;         // Low byte
}
```

## Address Resolution

For instructions with addresses (JCN, JMS, JUN), the assembler would:

1. Calculate the 12-bit address
2. Handle forward and backward references
3. Generate appropriate machine code format
4. Handle address overflow checking (12-bit limit)

## Memory Layout

The 4004's 12-bit address space (4096 bytes) would be organized as:
- $000-$0FF: System/ROM area
- $100-$1FF: I/O area
- $200-$FFF: User program/data space

This code generation approach would allow for complete assembly of Intel 4004 programs into executable machine code.