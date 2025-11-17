#include "TubeArithmeticUnits.h"
#include <algorithm>
#include <numeric>

// TubeArithmeticUnit implementation
TubeArithmeticUnit::TubeArithmeticUnit(int width) : width(width) {
    if (width < 1) width = 1;
    if (width > 32) width = 32;
    
    initialize();
}

void TubeArithmeticUnit::initialize() {
    inputA.resize(width, false);
    inputB.resize(width, false);
    result.resize(width, false);
    
    // Create full adders for the arithmetic operations
    adders.resize(width);
    for (int i = 0; i < width; i++) {
        adders[i] = std::make_unique<TubeFullAdder>();
        inputAPins.push_back(i);
        inputBPins.push_back(width + i);
        resultPins.push_back(2 * width + i);
    }
    
    carryInPin = 2 * width;
    operationPin = 2 * width + 1;
    carryOutPin = 2 * width + 2;
    overflowPin = 2 * width + 3;
    zeroPin = 2 * width + 4;
    negativePin = 2 * width + 5;
    clockPin = 2 * width + 6;
}

bool bool TubeArithmeticUnit::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeArithmeticUnit::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    // Handle input A
    for (int i = 0; i < width; i++) {
        if (conn_id == static_cast<uint16>(inputAPins[i]) && data_bytes == sizeof(double)) {
            inputA[i] = voltageToLogic(*reinterpret_cast<double*>(data));
            return true;
        }
    }
    
    // Handle input B
    for (int i = 0; i < width; i++) {
        if (conn_id == static_cast<uint16>(inputBPins[i]) && data_bytes == sizeof(double)) {
            inputB[i] = voltageToLogic(*reinterpret_cast<double*>(data));
            return true;
        }
    }
    
    if (conn_id == carryInPin && data_bytes == sizeof(double)) {
        carryIn = voltageToLogic(*reinterpret_cast<double*>(data));
        return true;
    } else if (conn_id == operationPin && data_bytes == sizeof(double)) {
        operation = static_cast<int>(*reinterpret_cast<double*>(data));
        return true;
    }
    
    return false;
}

bool TubeArithmeticUnit::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
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
    
    if (conn_id == overflowPin && data_bytes == sizeof(double)) {
        double voltage = logicToVoltage(overflow);
        memcpy(data, &voltage, sizeof(double));
        return true;
    }
    
    if (conn_id == zeroPin && data_bytes == sizeof(double)) {
        double voltage = logicToVoltage(zero);
        memcpy(data, &voltage, sizeof(double));
        return true;
    }
    
    if (conn_id == negativePin && data_bytes == sizeof(double)) {
        double voltage = logicToVoltage(negative);
        memcpy(data, &voltage, sizeof(double));
        return true;
    }
    
    return false;
}

bool TubeArithmeticUnit::Tick() {
    performOperation();
    updateFlags();
    return true;
}

void TubeArithmeticUnit::setInputA(const std::vector<bool>& value) {
    int bitsToSet = std::min(static_cast<int>(value.size()), width);
    for (int i = 0; i < bitsToSet; i++) {
        inputA[i] = value[i];
    }
}

void TubeArithmeticUnit::setInputB(const std::vector<bool>& value) {
    int bitsToSet = std::min(static_cast<int>(value.size()), width);
    for (int i = 0; i < bitsToSet; i++) {
        inputB[i] = value[i];
    }
}

void TubeArithmeticUnit::performOperation() {
    // Default implementation: just pass through input A
    result = inputA;
    carryOut = false;
}

void TubeArithmeticUnit::updateFlags() {
    // Zero flag: set if result is all zeros
    zero = true;
    for (int i = 0; i < width; i++) {
        if (result[i]) {
            zero = false;
            break;
        }
    }
    
    // Negative flag: set based on most significant bit
    negative = result[width - 1];
    
    // Overflow flag: needs to be set in derived classes based on the specific operation
    overflow = false;
}


// TubeAdder implementation
TubeAdder::TubeAdder(int width) : TubeArithmeticUnit(width) {
    // Adder inherits the basic structure from TubeArithmeticUnit
}

void TubeAdder::performOperation() {
    switch (operation) {
        case 0:  // Addition
            performAddition();
            break;
        case 1:  // Subtraction (using 2's complement)
            performSubtraction();
            break;
        case 8:  // Increment
            performIncrement();
            break;
        default:
            // Default: just set result to inputA
            result = inputA;
            carryOut = false;
            break;
    }
}

void TubeAdder::performAddition() {
    bool carry = carryIn;
    for (int i = 0; i < width; i++) {
        // Use the full adders to compute the sum
        int sum = inputA[i] + inputB[i] + carry;
        result[i] = sum & 1;
        carry = (sum > 1);
    }
    carryOut = carry;
    
    // Check for overflow (occurs when adding two positives gives negative or two negatives gives positive)
    if (width > 1) {
        bool aSign = inputA[width-1];
        bool bSign = inputB[width-1];
        bool rSign = result[width-1];
        overflow = (aSign == bSign) && (aSign != rSign);
    }
}

void TubeAdder::performSubtraction() {
    // Subtraction: A - B = A + (~B + 1) = A + ~B + 1
    // First, invert B
    std::vector<bool> invertedB(width);
    for (int i = 0; i < width; i++) {
        invertedB[i] = !inputB[i];
    }
    
    // Then add A + invertedB + 1
    bool carry = true;  // Add 1 to perform 2's complement
    for (int i = 0; i < width; i++) {
        int sum = inputA[i] + invertedB[i] + carry;
        result[i] = sum & 1;
        carry = (sum > 1);
    }
    carryOut = carry;
    
    // Check for overflow
    if (width > 1) {
        bool aSign = inputA[width-1];
        bool bSign = inputB[width-1];  // Original B sign
        bool rSign = result[width-1];
        overflow = (aSign != bSign) && (aSign != rSign);
    }
}

void TubeAdder::performIncrement() {
    // Increment: Add 1 to inputA
    bool carry = true;
    for (int i = 0; i < width; i++) {
        int sum = inputA[i] + (i == 0 ? 1 : 0) + (i > 0 ? (carry ? 1 : 0) : 0);
        result[i] = sum & 1;
        carry = (sum > 1);
    }
    carryOut = carry;
}


// TubeMultiplier implementation
TubeMultiplier::TubeMultiplier(int width) : TubeArithmeticUnit(width) {
    // Initialize for multiplication
}

void TubeMultiplier::performOperation() {
    performMultiplication();
}

std::vector<bool> TubeMultiplier::multiply(const std::vector<bool>& a, const std::vector<bool>& b) {
    std::vector<bool> result(2 * width, false);
    
    // Simple implementation: repeated addition
    for (int i = 0; i < width; i++) {
        if (b[i]) {  // If bit i of B is set
            // Add A shifted by i positions to the result
            std::vector<bool> shiftedA(2 * width, false);
            for (int j = 0; j < width; j++) {
                shiftedA[j + i] = a[j];
            }
            
            // Add shiftedA to result
            bool carry = false;
            for (int k = 0; k < 2 * width; k++) {
                int sum = result[k] + shiftedA[k] + carry;
                result[k] = sum & 1;
                carry = (sum > 1);
            }
        }
    }
    
    // Return only the lower width bits for simplicity in this context
    std::vector<bool> finalResult(width);
    for (int i = 0; i < width; i++) {
        finalResult[i] = result[i];
    }
    
    return finalResult;
}

void TubeMultiplier::performMultiplication() {
    result = multiply(inputA, inputB);
    
    // For now, set carryOut and overflow to false
    // In a real implementation, these would be computed based on result size
    carryOut = false;
    overflow = false;
}


// TubeDivider implementation
TubeDivider::TubeDivider(int width) : TubeArithmeticUnit(width) {
    // Initialize for division
}

void TubeDivider::performOperation() {
    performDivision();
}

std::pair<std::vector<bool>, std::vector<bool>> TubeDivider::divide(const std::vector<bool>& dividend, const std::vector<bool>& divisor) {
    // Convert inputs to integers for division
    int divd = 0, divs = 0;
    for (int i = 0; i < width; i++) {
        if (dividend[i]) divd |= (1 << i);
        if (divisor[i]) divs |= (1 << i);
    }
    
    std::vector<bool> quotient(width, false);
    std::vector<bool> remainder(width, false);
    
    if (divs == 0) {
        // Division by zero - set all bits high as error condition
        for (int i = 0; i < width; i++) {
            quotient[i] = true;
            remainder[i] = true;
        }
    } else {
        int q = divd / divs;
        int r = divd % divs;
        
        // Convert back to binary
        for (int i = 0; i < width; i++) {
            quotient[i] = (q >> i) & 1;
            remainder[i] = (r >> i) & 1;
        }
    }
    
    return std::make_pair(quotient, remainder);
}

void TubeDivider::performDivision() {
    auto [quotient, remainder] = divide(inputA, inputB);
    result = quotient;
    
    // For now, set carryOut and overflow to false
    carryOut = false;
    overflow = false;
}


// TubeALUExtended implementation
TubeALUExtended::TubeALUExtended(int width) : TubeArithmeticUnit(width) {
    // Initialize for ALU operations
}

void TubeALUExtended::setOperation(int op) {
    operation = op;
}

void TubeALUExtended::performOperation() {
    switch (operation) {
        case ADD:
            // Fall through to use the adder's implementation
            {
                bool carry = carryIn;
                for (int i = 0; i < width; i++) {
                    int sum = inputA[i] + inputB[i] + carry;
                    result[i] = sum & 1;
                    carry = (sum > 1);
                }
                carryOut = carry;
                
                // Check for overflow
                if (width > 1) {
                    bool aSign = inputA[width-1];
                    bool bSign = inputB[width-1];
                    bool rSign = result[width-1];
                    overflow = (aSign == bSign) && (aSign != rSign);
                }
            }
            break;
            
        case SUB:
            // Subtraction using 2's complement
            {
                std::vector<bool> invertedB(width);
                for (int i = 0; i < width; i++) {
                    invertedB[i] = !inputB[i];
                }
                
                bool carry = true;  // Add 1 to perform 2's complement
                for (int i = 0; i < width; i++) {
                    int sum = inputA[i] + invertedB[i] + carry;
                    result[i] = sum & 1;
                    carry = (sum > 1);
                }
                carryOut = carry;
                
                if (width > 1) {
                    bool aSign = inputA[width-1];
                    bool bSign = inputB[width-1];  // Original B sign
                    bool rSign = result[width-1];
                    overflow = (aSign != bSign) && (aSign != rSign);
                }
            }
            break;
            
        case AND:
            performAND();
            break;
            
        case OR:
            performOR();
            break;
            
        case XOR:
            performXOR();
            break;
            
        case NOT:
            performNOT();
            break;
            
        case SHIFT_LEFT:
            performShiftLeft();
            break;
            
        case SHIFT_RIGHT:
            performShiftRight();
            break;
            
        case INC:
            // Increment inputA by 1
            {
                bool carry = true;
                result[0] = !inputA[0];
                for (int i = 1; i < width; i++) {
                    bool oldResult = result[i];
                    result[i] = inputA[i] ^ carry;
                    carry = oldResult && result[i];  // If we went from 1 to 0 at position i
                }
                carryOut = carry;
            }
            break;
            
        case DEC:
            // Decrement inputA by 1
            {
                bool borrow = true;
                for (int i = 0; i < width; i++) {
                    bool oldBit = inputA[i];
                    result[i] = !inputA[i] ^ borrow;  // Flip bit if we're still borrowing
                    borrow = !oldBit && result[i];    // Borrow if we went from 0 to 1
                }
                carryOut = !borrow;  // Invert borrow for carry out
            }
            break;
            
        case COMPARE:
            performCompare();
            break;
            
        default:
            // NOP
            result = inputA;
            break;
    }
}

void TubeALUExtended::performAND() {
    for (int i = 0; i < width; i++) {
        result[i] = inputA[i] && inputB[i];
    }
    carryOut = false;
    overflow = false;
}

void TubeALUExtended::performOR() {
    for (int i = 0; i < width; i++) {
        result[i] = inputA[i] || inputB[i];
    }
    carryOut = false;
    overflow = false;
}

void TubeALUExtended::performXOR() {
    for (int i = 0; i < width; i++) {
        result[i] = inputA[i] ^ inputB[i];
    }
    carryOut = false;
    overflow = false;
}

void TubeALUExtended::performNOT() {
    for (int i = 0; i < width; i++) {
        result[i] = !inputA[i];  // NOT A
    }
    carryOut = false;
    overflow = false;
}

void TubeALUExtended::performShiftLeft() {
    for (int i = 0; i < width - 1; i++) {
        result[i] = inputA[i+1];
    }
    result[width-1] = false;  // Shift in 0
    carryOut = inputA[0];     // Carry out is the bit that was shifted out
    overflow = false;
}

void TubeALUExtended::performShiftRight() {
    for (int i = 1; i < width; i++) {
        result[i] = inputA[i-1];
    }
    result[0] = false;  // Shift in 0 (logical shift)
    carryOut = inputA[width-1];  // Carry out is the bit that was shifted out
    overflow = false;
}

void TubeALUExtended::performCompare() {
    int aVal = 0, bVal = 0;
    for (int i = 0; i < width; i++) {
        if (inputA[i]) aVal |= (1 << i);
        if (inputB[i]) bVal |= (1 << i);
    }
    
    if (aVal == bVal) {
        compareResult = EQUAL;
        result[0] = false;
        result[1] = false;
    } else if (aVal > bVal) {
        compareResult = GREATER;
        result[0] = true;
        result[1] = false;
    } else {
        compareResult = LESS;
        result[0] = false;
        result[1] = true;
    }
    
    // Clear the rest of the result
    for (int i = 2; i < width; i++) {
        result[i] = false;
    }
    
    carryOut = false;
    overflow = false;
}


// TubeArithmeticProcessingUnit implementation
TubeArithmeticProcessingUnit::TubeArithmeticProcessingUnit(int width) : width(width) {
    alu = std::make_unique<TubeALUExtended>(width);
    adder = std::make_unique<TubeAdder>(width);
    multiplier = std::make_unique<TubeMultiplier>(width);
    divider = std::make_unique<TubeDivider>(width);
    
    result.resize(width, false);
    operandA.resize(width, false);
    operandB.resize(width, false);
}

void TubeArithmeticProcessingUnit::setOperandA(const std::vector<bool>& value) {
    int bitsToSet = std::min(static_cast<int>(value.size()), width);
    for (int i = 0; i < bitsToSet; i++) {
        operandA[i] = value[i];
        result[i] = value[i];  // Initialize with A
    }
}

void TubeArithmeticProcessingUnit::setOperandB(const std::vector<bool>& value) {
    int bitsToSet = std::min(static_cast<int>(value.size()), width);
    for (int i = 0; i < bitsToSet; i++) {
        operandB[i] = value[i];
    }
}

void TubeArithmeticProcessingUnit::setOperation(TubeALUExtended::Operation op) {
    alu->setOperation(op);
}

void TubeArithmeticProcessingUnit::execute() {
    // For now, we'll just use the ALU as the primary arithmetic unit
    alu->setInputA(operandA);
    alu->setInputB(operandB);
    alu->performOperation();
    
    // Get results
    result = alu->getResult();
    carry = alu->getCarryOut();
    overflow = alu->getOverflow();
    zero = alu->getZero();
    negative = alu->getNegative();
}


// TubeBCDArithmeticUnit implementation
TubeBCDArithmeticUnit::TubeBCDArithmeticUnit(int digits) : digits(digits) {
    bits = digits * 4;
    if (digits < 1) digits = 1;
    if (digits > 8) digits = 8;  // Limit to 32 bits (8 BCD digits)
    
    initialize();
}

void TubeBCDArithmeticUnit::initialize() {
    // Initialize vectors
    inputA.resize(bits, false);
    inputB.resize(bits, false);
    result.resize(bits, false);
    
    // Create adders for the arithmetic operations
    binaryAdders.resize(digits);
    correctionAdders.resize(digits);
    for (int i = 0; i < digits; i++) {
        binaryAdders[i] = std::make_unique<TubeAdder>(4);  // 4-bit adder for each digit
        correctionAdders[i] = std::make_unique<TubeAdder>(4);  // For BCD correction
        inputAPins.push_back(i * 4);      // Start of each 4-bit group
        inputBPins.push_back(bits + i * 4);  // Start of each 4-bit group
        resultPins.push_back(2 * bits + i * 4);  // Start of each 4-bit group
    }
    
    carryInPin = 3 * bits;
    operationPin = 3 * bits + 1;
    carryOutPin = 3 * bits + 2;
    clockPin = 3 * bits + 3;
}

bool bool TubeBCDArithmeticUnit::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeBCDArithmeticUnit::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    // Handle input A (BCD format)
    for (int digit = 0; digit < digits; digit++) {
        for (int bit = 0; bit < 4; bit++) {
            int pinIndex = digit * 4 + bit;
            if (conn_id == static_cast<uint16>(inputAPins[digit] + bit) && data_bytes == sizeof(double)) {
                inputA[pinIndex] = voltageToLogic(*reinterpret_cast<double*>(data));
                return true;
            }
        }
    }
    
    // Handle input B (BCD format)
    for (int digit = 0; digit < digits; digit++) {
        for (int bit = 0; bit < 4; bit++) {
            int pinIndex = bits + digit * 4 + bit;
            if (conn_id == static_cast<uint16>(inputBPins[digit] + bit) && data_bytes == sizeof(double)) {
                inputB[pinIndex] = voltageToLogic(*reinterpret_cast<double*>(data));
                return true;
            }
        }
    }
    
    if (conn_id == carryInPin && data_bytes == sizeof(double)) {
        carryIn = voltageToLogic(*reinterpret_cast<double*>(data));
        return true;
    } else if (conn_id == operationPin && data_bytes == sizeof(double)) {
        operation = static_cast<int>(*reinterpret_cast<double*>(data));
        return true;
    }
    
    return false;
}

bool TubeBCDArithmeticUnit::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    // Handle result outputs (BCD format)
    for (int digit = 0; digit < digits; digit++) {
        for (int bit = 0; bit < 4; bit++) {
            int pinIndex = 2 * bits + digit * 4 + bit;
            if (conn_id == static_cast<uint16>(resultPins[digit] + bit) && data_bytes == sizeof(double)) {
                double voltage = logicToVoltage(result[pinIndex]);
                memcpy(data, &voltage, sizeof(double));
                return true;
            }
        }
    }
    
    if (conn_id == carryOutPin && data_bytes == sizeof(double)) {
        double voltage = logicToVoltage(carryOut);
        memcpy(data, &voltage, sizeof(double));
        return true;
    }
    
    return false;
}

bool TubeBCDArithmeticUnit::Tick() {
    performOperation();
    return true;
}

void TubeBCDArithmeticUnit::setInputA(const std::vector<bool>& bcdValue) {
    int bitsToSet = std::min(static_cast<int>(bcdValue.size()), bits);
    for (int i = 0; i < bitsToSet; i++) {
        inputA[i] = bcdValue[i];
    }
}

void TubeBCDArithmeticUnit::setInputB(const std::vector<bool>& bcdValue) {
    int bitsToSet = std::min(static_cast<int>(bcdValue.size()), bits);
    for (int i = 0; i < bitsToSet; i++) {
        inputB[i] = bcdValue[i];
    }
}

void TubeBCDArithmeticUnit::performOperation() {
    if (operation == 0) {  // Addition
        performBCDAddition();
    } else if (operation == 1) {  // Subtraction
        performBCDSubtraction();
    } else {
        // NOP - just pass through input A
        result = inputA;
        carryOut = false;
    }
}

void TubeBCDArithmeticUnit::performBCDAddition() {
    bool carry = carryIn;
    
    // Perform addition for each BCD digit (4 bits)
    for (int digit = 0; digit < digits; digit++) {
        // Extract the 4 bits for this digit
        std::vector<bool> digitA(4), digitB(4);
        for (int bit = 0; bit < 4; bit++) {
            digitA[bit] = inputA[digit * 4 + bit];
            digitB[bit] = inputB[digit * 4 + bit];
        }
        
        // Perform binary addition
        std::unique_ptr<TubeAdder> tempAdder = std::make_unique<TubeAdder>(4);
        tempAdder->setInputA(digitA);
        tempAdder->setInputB(digitB);
        tempAdder->setCarryIn(carry);
        tempAdder->setOperation(0);  // Addition
        tempAdder->performOperation();
        
        std::vector<bool> binarySum = tempAdder->getResult();
        bool digitCarry = tempAdder->getCarryOut();
        
        // Check if correction is needed (binary value > 9)
        int decimalValue = 0;
        for (int i = 0; i < 4; i++) {
            if (binarySum[i]) decimalValue |= (1 << i);
        }
        
        if (decimalValue > 9 || digitCarry) {
            // Apply BCD correction: add 6 (0110)
            std::vector<bool> correction = {false, true, true, false};  // 6 in binary
            std::unique_ptr<TubeAdder> correctionAdder = std::make_unique<TubeAdder>(4);
            correctionAdder->setInputA(binarySum);
            correctionAdder->setInputB(correction);
            correctionAdder->setCarryIn(false);
            correctionAdder->setOperation(0);  // Addition
            correctionAdder->performOperation();
            
            // Store corrected result
            std::vector<bool> corrected = correctionAdder->getResult();
            for (int bit = 0; bit < 4; bit++) {
                result[digit * 4 + bit] = corrected[bit];
            }
            
            // Carry to next digit
            carry = digitCarry || correctionAdder->getCarryOut();
        } else {
            // No correction needed
            for (int bit = 0; bit < 4; bit++) {
                result[digit * 4 + bit] = binarySum[bit];
            }
            carry = digitCarry;
        }
    }
    
    carryOut = carry;
}