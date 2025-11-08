# Intel 4004 Instruction Syntax Definitions

This document outlines the syntax definitions for the Intel 4004 microprocessor instructions that would be used in an assembler.

## Instruction Format

The Intel 4004 has 45 instructions with a 12-bit address space and 4-bit data paths. Instructions are typically 8 bits (one 4-bit ROM word) or 16 bits (two 4-bit ROM words) in length.

## Instruction Categories

### 1. Register Instructions (4-bit registers R0-R15)
- `ADD Rn` - Add register to accumulator (A = A + Rn)
- `SUB Rn` - Subtract register from accumulator (A = A - Rn)
- `LDM #data` - Load accumulator with immediate data
- `RD0`, `RD1`, `RD2`, `RD3` - Read ROM digit (4 bits)
- `WR0`, `WR1`, `WR2`, `WR3` - Write accumulator to output register

### 2. Memory Instructions (accessed via R0-R1 as address pointer)
- `WRM` - Write accumulator to memory at address in R0-R1
- `WMP` - Write RAM port
- `WRR` - Write ROM port
- `WRG` - Write to RAM or ROM port group
- `WRF` - Write Function register
- `RDM` - Read memory at address in R0-R1 to accumulator
- `WBN Rn` - Write register N to memory at address in R0-R1
- `SBM` - Subtract memory from accumulator
- `RBM` - Read memory and branch if not zero

### 3. Control Instructions
- `JCN cond, addr` - Conditional jump
- `JUN addr` - Jump unconditionally
- `JMS addr` - Jump to subroutine
- `INC Rn` - Increment register
- `FIM Rn, #data` - Fetch immediate (load 8-bit immediate into register pair)
- `SRC Rn` - Send register to output control
- `FIN Rn` - Fetch input to register
- `JIN Rn` - Jump indirect through register pair

### 4. Status Instructions
- `CLB` - Clear both carry and auxiliary carry flags
- `CLC` - Clear carry flag
- `WMP` - Write memory port
- `WRR` - Write ROM port
- `WRG` - Write RAM/ROM port group
- `WRF` - Write function register

## Syntax Definition Example

```
instruction ::= mnemonic [operands]
mnemonic ::= 'ADD' | 'SUB' | 'LDM' | 'WRM' | 'RDM' | 'JCN' | 'JUN' | 'JMS' | etc.
operands ::= register | immediate | address | condition
register ::= 'R0' | 'R1' | ... | 'R15'
immediate ::= '#' hex_digit | '#' hex_digit hex_digit
address ::= '$' hex_addr | hex_addr
condition ::= 'C' | 'NC' | 'Z' | 'NZ' | 'T' | 'NT' | etc.
```

## Addressing Modes

1. **Immediate**: `#nn` (8-bit immediate value)
2. **Direct Memory**: `$nnnn` (12-bit address) - accessed via R0-R1 pointer
3. **Register**: `Rn` (n = 0-15)

## Example Instructions

```
LDM #5      ; Load accumulator with value 5
ADD R3      ; Add register 3 to accumulator
JCN C,$100  ; Jump to address $100 if carry flag set
JMS $200    ; Call subroutine at address $200
WRM         ; Write accumulator to memory at R0-R1
RDM         ; Read memory at R0-R1 to accumulator
```

## Pseudo-Instructions

```
ORG addr    ; Set origin address
DB data     ; Define byte
DW data     ; Define word
DS count    ; Define storage (skip count locations)
END         ; End of program
```

This syntax would be used to define the instruction parsing rules for an assembler targeting the Intel 4004 processor.