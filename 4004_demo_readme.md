# Intel 4004 Character Output Demo

This project demonstrates a simple Intel 4004 program that outputs the character 'A'.

## Program Description

The 4004 program in `4004_putchar.bin` performs the following operations:
1. Sets up register pair R0R1 to point to memory address 0x10 (FIM instruction)
2. Reads the data at that address into the accumulator (RDM instruction)  
3. Writes the accumulator value to I/O port 0 (WR0 instruction)
4. The data at address 0x10 is 0x41, which is the ASCII code for 'A'

### 4004 Instructions Used:
- `FIM R0R1, 0x10` (Instruction: 0x20, data: 0x10) - Sets register pair 0 to value 0x10
- `RDM` (Instruction: 0x50) - Reads memory at R0-R1 to accumulator
- `WR0` (Instruction: 0x70) - Writes accumulator to I/O port 0
- `NOP` (Instruction: 0x00) - No operation, for halt loop

### Binary Content:
The binary file contains 18 bytes:
- Bytes 0-3: 0x20, 0x10, 0x50, 0x70 (the program instructions)
- Bytes 4-15: 0x00 (padding)
- Byte 16: 0x41 (the character 'A' to be output)
- Bytes 17+: additional padding if needed

## How to Run

1. Build the ProtoVM simulator using `./build.sh`
2. Run with the command:
   ```
   ./build/ProtoVM minimax4004 --load-binary 4004_putchar.bin 0x0 --ticks 20
   ```

## Expected Output

When properly connected, the program should:
- Load the value 0x41 ('A') from memory address 0x10 into the accumulator
- Output the value 0x41 to I/O port 0
- In a real hardware setup, this would appear as the character 'A' on an output device

## Troubleshooting

If you get connection errors, it may be due to incomplete wiring in the MiniMax4004 circuit. The 4004 CPU implementation is complete, but requires properly connected memory and I/O components to function correctly.