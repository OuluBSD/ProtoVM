#include <fstream>
#include <iostream>

int main() {
    // Create 4004 binary to output 'A'
    // Program: Load 'A' from memory address 0x10 to accumulator, then write to output port 0
    // Instructions:
    // 0x000: FIM R0R1, 0x10 (Set R0-R1 to point to address 0x0010) - 0x20, 0x10
    // 0x002: RDM (Read memory at R0-R1 into accumulator) - 0x50
    // 0x003: WR0 (Write accumulator to output port 0) - 0x70
    // 0x004: NOP (No operation) - 0x00 (for halt loop)

    // Data at address 0x10: 0x41 ('A' in ASCII)

    unsigned char program[] = {
        0x20, 0x10,  // FIM R0R1, 0x10
        0x50,        // RDM
        0x70,        // WR0
        0x00,        // NOP
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Pad to address 0x10
        0x00, 0x00, 0x00, 0x00, 0x41  // Address 0x10: 'A'
    };

    std::ofstream file("4004_putchar.bin", std::ios::binary);
    if (file) {
        file.write(reinterpret_cast<const char*>(program), sizeof(program));
        file.close();
        std::cout << "Binary file '4004_putchar.bin' created successfully with " << sizeof(program) << " bytes." << std::endl;
    } else {
        std::cerr << "Error creating binary file." << std::endl;
        return 1;
    }

    return 0;
}