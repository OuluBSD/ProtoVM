#include "TubeStandardLogicLibrary.h"
#include <algorithm>

namespace TubeLogicLibrary {

namespace StandardICs {

// IC7400 implementation (Quad 2-input NAND gates)
IC7400::IC7400() {
    initialize();
}

void IC7400::initialize() {
    nandGates.resize(4);
    for (int i = 0; i < 4; i++) {
        nandGates[i] = std::make_unique<TubeNANDGate>(2);  // 2 inputs per gate
    }
    
    // Assign pin numbers based on typical 14-pin DIP layout
    // This is a simplified representation
    inputPinsA = {1, 4, 9, 12};   // A inputs of each gate
    inputPinsB = {2, 5, 10, 13};  // B inputs of each gate
    outputPins = {3, 6, 8, 11};   // Output of each gate
    vccPin = 14;                  // Power
    gndPin = 7;                   // Ground
}

bool IC7400::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool IC7400::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    // Handle inputs for each NAND gate
    for (int i = 0; i < 4; i++) {
        if (conn_id == static_cast<uint16>(inputPinsA[i])) {
            double aValue;
            memcpy(&aValue, data, sizeof(double));
            // In a real implementation, we would connect this pin to the appropriate gate input
            // For this simulation, we'll just process it directly
            return true;
        }
        if (conn_id == static_cast<uint16>(inputPinsB[i])) {
            double bValue;
            memcpy(&bValue, data, sizeof(double));
            // Process input for this NAND gate
            return true;
        }
    }
    
    return false;
}

bool IC7400::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    // Handle outputs from each NAND gate
    for (int i = 0; i < 4; i++) {
        if (conn_id == static_cast<uint16>(outputPins[i])) {
            bool value = true; // In a real implementation, this would come from the actual gate output
            double voltage = value ? 5.0 : 0.0;
            memcpy(data, &voltage, sizeof(double));
            return true;
        }
    }
    return false;
}

bool IC7400::Tick() {
    processLogic();
    return true;
}

void IC7400::processLogic() {
    for (int i = 0; i < 4; i++) {
        nandGates[i]->Tick(); // Process each gate
    }
}


// IC7404 implementation (Hex inverter)
IC7404::IC7404() {
    initialize();
}

void IC7404::initialize() {
    notGates.resize(6);
    for (int i = 0; i < 6; i++) {
        notGates[i] = std::make_unique<TubeNOTGate>();
    }
    
    // Assign pin numbers based on typical 14-pin DIP layout
    inputPins = {1, 3, 5, 9, 11, 13};  // A inputs of each gate
    outputPins = {2, 4, 6, 8, 10, 12}; // Output of each gate
    vccPin = 14;                        // Power
    gndPin = 7;                         // Ground
}

bool IC7404::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool IC7404::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    // Handle inputs for each NOT gate
    for (int i = 0; i < 6; i++) {
        if (conn_id == static_cast<uint16>(inputPins[i])) {
            double inputValue;
            memcpy(&inputValue, data, sizeof(double));
            // Process input for this NOT gate
            return true;
        }
    }
    
    return false;
}

bool IC7404::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    // Handle outputs from each NOT gate
    for (int i = 0; i < 6; i++) {
        if (conn_id == static_cast<uint16>(outputPins[i])) {
            bool value = false; // In a real implementation, this would come from the actual gate output
            double voltage = value ? 5.0 : 0.0; 
            memcpy(data, &voltage, sizeof(double));
            return true;
        }
    }
    return false;
}

bool IC7404::Tick() {
    processLogic();
    return true;
}

void IC7404::processLogic() {
    for (int i = 0; i < 6; i++) {
        notGates[i]->Tick(); // Process each gate
    }
}


// IC7474 implementation (Dual D-type flip-flop)
IC7474::IC7474() {
    initialize();
}

void IC7474::initialize() {
    flipFlops.resize(2);
    for (int i = 0; i < 2; i++) {
        flipFlops[i] = std::make_unique<TubeDFlipFlop>();
    }
    
    // Assign pin numbers based on typical 14-pin DIP layout
    dataPins = {2, 12};              // D inputs
    clockPins = {3, 11};             // Clock inputs
    presetPins = {4, 10};            // Preset inputs
    clearPins = {1, 13};             // Clear inputs
    outputPins = {5, 9};             // Q outputs
    outputInvertedPins = {6, 8};     // Q-bar outputs
    vccPin = 14;                     // Power
    gndPin = 7;                      // Ground
}

bool IC7474::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool IC7474::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    // Handle various inputs to the IC
    for (int i = 0; i < 2; i++) {
        if (conn_id == static_cast<uint16>(dataPins[i])) {
            double value;
            memcpy(&value, data, sizeof(double));
            // Set data input for flip-flop i
            return true;
        } else if (conn_id == static_cast<uint16>(clockPins[i])) {
            double value;
            memcpy(&value, data, sizeof(double));
            // Clock input for flip-flop i
            return true;
        } else if (conn_id == static_cast<uint16>(presetPins[i])) {
            double value;
            memcpy(&value, data, sizeof(double));
            // Preset input for flip-flop i
            return true;
        } else if (conn_id == static_cast<uint16>(clearPins[i])) {
            double value;
            memcpy(&value, data, sizeof(double));
            // Clear input for flip-flop i
            return true;
        }
    }
    
    return false;
}

bool IC7474::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    for (int i = 0; i < 2; i++) {
        if (conn_id == static_cast<uint16>(outputPins[i])) {
            double voltage = 0.0; // Actual value would depend on flip-flop state
            memcpy(data, &voltage, sizeof(double));
            return true;
        } else if (conn_id == static_cast<uint16>(outputInvertedPins[i])) {
            double voltage = 5.0; // Inverted output, so opposite of normal output
            memcpy(data, &voltage, sizeof(double));
            return true;
        }
    }
    return false;
}

bool IC7474::Tick() {
    processLogic();
    return true;
}

void IC7474::processLogic() {
    for (int i = 0; i < 2; i++) {
        flipFlops[i]->Tick();
    }
}


// IC7490 implementation (Decade counter)
IC7490::IC7490() {
    initialize();
}

void IC7490::initialize() {
    bcdCounter = std::make_unique<TubeBCDCounter>(4);  // 4-bit BCD counter
    
    // Assign pin numbers based on typical 14-pin DIP layout
    inputA = 14;           // Input A (MR1 and MR2 need to be held high for counting)
    inputB = 1;            // Input B 
    resetPins[0] = 2;      // Reset input (Master Reset 1)
    resetPins[1] = 3;      // Reset input (Master Reset 2)
    setPins[0] = 6;        // Set input (MS1)
    setPins[1] = 7;        // Set input (MS2)
    
    outputPins.resize(4);
    outputPins[0] = 12;    // QA (LSB)
    outputPins[1] = 9;     // QB
    outputPins[2] = 8;     // QC
    outputPins[3] = 11;    // QD (MSB)
    
    vccPin = 5;            // Power
    gndPin = 10;           // Ground
}

bool IC7490::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool IC7490::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == inputA || conn_id == inputB) {
        // Handle clock inputs
        return true;
    } else if (conn_id == resetPins[0] || conn_id == resetPins[1]) {
        // Handle reset
        return true;
    } else if (conn_id == setPins[0] || conn_id == setPins[1]) {
        // Handle set inputs
        return true;
    }
    return false;
}

bool IC7490::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    for (int i = 0; i < 4; i++) {
        if (conn_id == static_cast<uint16>(outputPins[i])) {
            bool value = bcdCounter->getBinaryValue()[i];
            double voltage = value ? 5.0 : 0.0;
            memcpy(data, &voltage, sizeof(double));
            return true;
        }
    }
    return false;
}

bool IC7490::Tick() {
    processLogic();
    return true;
}

void IC7490::processLogic() {
    bcdCounter->clock();
}

}

namespace Utils {

std::unique_ptr<TubeLogicGate> createOptimizedGate(
    TubeLogicGate::GateType type, 
    LogicFamily family, 
    int inputs) {
    
    switch (type) {
        case TubeLogicGate::AND:
            return std::make_unique<TubeANDGate>(inputs);
        case TubeLogicGate::OR:
            return std::make_unique<TubeORGate>(inputs);
        case TubeLogicGate::NAND:
            return std::make_unique<TubeNANDGate>(inputs);
        case TubeLogicGate::NOR:
            return std::make_unique<TubeNORGate>(inputs);
        case TubeLogicGate::XOR:
            return std::make_unique<TubeXORGate>();
        case TubeLogicGate::NOT:
            return std::make_unique<TubeNOTGate>();
        default:
            return std::make_unique<TubeANDGate>(inputs);
    }
}

std::unique_ptr<TubeRegister> createOptimizedRegister(
    int width, 
    LogicFamily family) {
    
    return std::make_unique<TubeRegister>(width);
}

std::unique_ptr<TubeCounter> createOptimizedCounter(
    int width, 
    LogicFamily family) {
    
    return std::make_unique<TubeBinaryCounter>(width);
}

std::unique_ptr<TubeMultiplexer> createOptimizedMux(
    int dataBits, 
    int selectBits, 
    LogicFamily family) {
    
    return std::make_unique<TubeMultiplexer>(dataBits, selectBits);
}

}

namespace System {

// TubALU implementation
TubALU::TubALU(int width) : width(width) {
    if (width < 1) width = 1;
    if (width > 32) width = 32;
    
    initialize();
}

void TubALU::initialize() {
    operandA.resize(width, false);
    operandB.resize(width, false);
    result.resize(width, false);
    
    // Create adders for arithmetic operations
    adders.resize(width);
    for (int i = 0; i < width; i++) {
        adders[i] = std::make_unique<TubeFullAdder>();
        inputAPins.push_back(i);
        inputBPins.push_back(width + i);
        resultPins.push_back(2 * width + i);
    }
    
    operationPin = 2 * width;
    carryOutPin = 2 * width + 1;
    clockPin = 2 * width + 2;
}

bool TubALU::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubALU::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    // Handle inputs A
    for (int i = 0; i < width; i++) {
        if (conn_id == static_cast<uint16>(inputAPins[i]) && data_bytes == sizeof(double)) {
            operandA[i] = voltageToLogic(*reinterpret_cast<double*>(data));
            return true;
        }
    }
    
    // Handle inputs B
    for (int i = 0; i < width; i++) {
        if (conn_id == static_cast<uint16>(inputBPins[i]) && data_bytes == sizeof(double)) {
            operandB[i] = voltageToLogic(*reinterpret_cast<double*>(data));
            return true;
        }
    }
    
    if (conn_id == operationPin && data_bytes == sizeof(double)) {
        operation = static_cast<Operation>(*reinterpret_cast<double*>(data));
        return true;
    }
    
    return false;
}

bool TubALU::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    // Handle result outputs
    for (int i = 0; i < width; i++) {
        if (conn_id == static_cast<uint16>(resultPins[i]) && data_bytes == sizeof(double)) {
            double voltage = logicToVoltage(result[i]);
            memcpy(data, &voltage, sizeof(double));
            return true;
        }
    }
    
    if (conn_id == carryOutPin && data_bytes == sizeof(double)) {
        double voltage = logicToVoltage(carryOut);
        memcpy(data, &voltage, sizeof(double));
        return true;
    }
    
    return false;
}

bool TubALU::Tick() {
    performOperation();
    return true;
}

void TubALU::performOperation() {
    switch (operation) {
        case ADD: {
            bool carry = false;
            for (int i = 0; i < width; i++) {
                int sum = operandA[i] + operandB[i] + carry;
                result[i] = sum & 1;
                carry = (sum > 1);
            }
            carryOut = carry;
            break;
        }
        case SUB: {
            // Subtraction using 2's complement: A - B = A + (~B + 1)
            std::vector<bool> negB(width);
            for (int i = 0; i < width; i++) {
                negB[i] = !operandB[i];  // Bitwise NOT
            }
            
            // Add 1 to negate (two's complement)
            bool carry = true;
            for (int i = 0; i < width; i++) {
                int sum = negB[i] + (i == 0 ? 1 : 0) + (i > 0 ? carry ? 1 : 0 : 0);
                negB[i] = sum & 1;
                carry = (sum > 1);
            }
            
            // Now add A + (-B)
            carry = false;
            for (int i = 0; i < width; i++) {
                int sum = operandA[i] + negB[i] + carry;
                result[i] = sum & 1;
                carry = (sum > 1);
            }
            carryOut = carry;
            break;
        }
        case AND: {
            for (int i = 0; i < width; i++) {
                result[i] = operandA[i] && operandB[i];
            }
            carryOut = false;
            break;
        }
        case OR: {
            for (int i = 0; i < width; i++) {
                result[i] = operandA[i] || operandB[i];
            }
            carryOut = false;
            break;
        }
        case XOR: {
            for (int i = 0; i < width; i++) {
                result[i] = operandA[i] ^ operandB[i];
            }
            carryOut = false;
            break;
        }
        case NOT: {
            // NOT operation applies to operand A
            for (int i = 0; i < width; i++) {
                result[i] = !operandA[i];
            }
            carryOut = false;
            break;
        }
        case SHIFT_LEFT: {
            // Shift A left by 1
            for (int i = 0; i < width - 1; i++) {
                result[i] = operandA[i+1];
            }
            result[width-1] = false;  // Shift in 0
            carryOut = operandA[0];   // Carry out is the bit that was shifted out
            break;
        }
        case SHIFT_RIGHT: {
            // Shift A right by 1
            for (int i = 1; i < width; i++) {
                result[i] = operandA[i-1];
            }
            result[0] = false;  // Shift in 0 (for logical shift)
            carryOut = operandA[width-1];  // Carry out is the bit that was shifted out
            break;
        }
        default:
            // NOP
            for (int i = 0; i < width; i++) {
                result[i] = operandA[i];
            }
            carryOut = false;
            break;
    }
}

}
}