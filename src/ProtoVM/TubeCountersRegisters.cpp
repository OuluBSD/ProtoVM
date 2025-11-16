#include "TubeCountersRegisters.h"
#include <algorithm>

// TubeCounter implementation
TubeCounter::TubeCounter(int width) : width(width) {
    if (width < 1) width = 1;
    if (width > 32) width = 32;  // Reasonable limit
    
    initialize();
}

void TubeCounter::initialize() {
    // Create flip-flops for each bit
    flipFlops.resize(width);
    currentValue.resize(width, false);
    
    for (int i = 0; i < width; i++) {
        flipFlops[i] = std::make_unique<TubeDFlipFlop>();
    }
    
    // Initialize to 0
    reset();
}

bool TubeCounter::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeCounter::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == clockPin && data_bytes == sizeof(double)) {
        bool clockSignal = voltageToLogic(*reinterpret_cast<double*>(data));
        
        // Detect rising edge
        if (!clockPrev && clockSignal) {
            clock();
        }
        clockPrev = clockSignal;
        return true;
    } else if (conn_id == resetPin && data_bytes == sizeof(double)) {
        bool resetSignal = voltageToLogic(*reinterpret_cast<double*>(data));
        
        // Detect rising edge for reset
        if (!resetPrev && resetSignal) {
            reset();
        }
        resetPrev = resetSignal;
        return true;
    } else if (conn_id == enablePin && data_bytes == sizeof(double)) {
        bool enableSignal = voltageToLogic(*reinterpret_cast<double*>(data));
        countingEnabled = enableSignal;
        return true;
    }
    return false;
}

bool TubeCounter::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == 0 && data_bytes == sizeof(double)) {  // Return current count as double
        double count = static_cast<double>(getCount());
        memcpy(data, &count, sizeof(double));
        return true;
    }
    return false;
}

bool TubeCounter::Tick() {
    processOperation();
    return true;
}

void TubeCounter::reset() {
    for (int i = 0; i < width; i++) {
        currentValue[i] = false;
    }
}

void TubeCounter::set(bool load, const std::vector<bool>& value) {
    if (load && !value.empty()) {
        int bitsToSet = std::min(static_cast<int>(value.size()), width);
        for (int i = 0; i < bitsToSet; i++) {
            currentValue[i] = value[i];
        }
    }
}

int TubeCounter::getCount() const {
    int count = 0;
    for (int i = 0; i < width; i++) {
        if (currentValue[i]) {
            count |= (1 << i);
        }
    }
    return count;
}

std::vector<bool> TubeCounter::getBinaryValue() const {
    return currentValue;
}

void TubeCounter::clock() {
    if (!countingEnabled) return;
    
    // Convert current value to integer
    int current = getCount();
    int newValue;
    
    if (countUp) {
        newValue = current + 1;
    } else {
        newValue = current - 1;
    }
    
    // Handle overflow/underflow for modulo counters
    if (maxCount > 0) {
        if (countUp && newValue > maxCount) {
            newValue = 0;
        } else if (!countUp && newValue < 0) {
            newValue = maxCount;
        }
    }
    
    // Convert back to binary
    for (int i = 0; i < width; i++) {
        currentValue[i] = (newValue >> i) & 1;
    }
    
    // Update flip-flops with new values
    // This would be done by connecting the flip-flops in a real implementation
    for (int i = 0; i < width; i++) {
        // Simulate updating each flip-flop
    }
}

void TubeCounter::processOperation() {
    // Process any continuous operations here
    // For a counter, the main operation happens on clock edge
}


// TubeBinaryCounter implementation
TubeBinaryCounter::TubeBinaryCounter(int width) : TubeCounter(width) {
    setMaxCount((1 << width) - 1);  // Maximum value for binary counter
}

void TubeBinaryCounter::clock() {
    if (!countingEnabled) return;
    
    // Handle carry propagation manually for binary counter
    bool carry = countUp;  // Start with 1 for increment, 0 for decrement
    
    if (countUp) {
        // Increment: add 1
        for (int i = 0; i < width && carry; i++) {
            bool oldBit = currentValue[i];
            currentValue[i] = !oldBit;  // Flip this bit
            carry = oldBit && currentValue[i];  // Carry if we went from 1 to 0
        }
        
        // Handle overflow if needed
        if (maxCount >= 0 && getCount() > maxCount) {
            reset();
        }
    } else {
        // Decrement: subtract 1
        for (int i = 0; i < width && carry; i++) {
            bool oldBit = currentValue[i];
            currentValue[i] = !oldBit;  // Flip this bit
            carry = !oldBit && currentValue[i];  // Borrow if we went from 0 to 1
        }
        
        // Handle underflow if needed
        if (maxCount >= 0 && getCount() < 0) {
            // Set to max count
            int maxVal = maxCount;
            for (int i = 0; i < width; i++) {
                currentValue[i] = (maxVal >> i) & 1;
            }
        }
    }
}

void TubeBinaryCounter::processOperation() {
    // Binary counter operation handled in clock method
}


// TubeBCDCounter implementation
TubeBCDCounter::TubeBCDCounter(int width) : TubeCounter(width) {
    // BCD counter max value is 9 for each 4-bit group
    // For simplicity, we'll implement a basic BCD counter
    setMaxCount(9);  // Single digit BCD
}

void TubeBCDCounter::clock() {
    if (!countingEnabled) return;
    
    int current = getCount();
    int newValue = countUp ? current + 1 : current - 1;
    
    // Implement BCD overflow/underflow
    if (newValue > 9) newValue = 0;
    if (newValue < 0) newValue = 9;
    
    // Convert back to binary
    for (int i = 0; i < width; i++) {
        currentValue[i] = (newValue >> i) & 1;
    }
}

void TubeBCDCounter::processOperation() {
    // BCD counter operation handled in clock method
}


// TubeRingCounter implementation
TubeRingCounter::TubeRingCounter(int width) : TubeCounter(width) {
    // Initialize to a single bit set (ring pattern)
    std::vector<bool> pattern(width, false);
    if (width > 0) pattern[0] = true;  // Start with first bit high
    initializePattern(pattern);
}

void TubeRingCounter::initializePattern(const std::vector<bool>& pattern) {
    if (!pattern.empty()) {
        initialPattern = pattern;
        int bitsToSet = std::min(static_cast<int>(pattern.size()), width);
        
        // Set to initial pattern
        for (int i = 0; i < bitsToSet; i++) {
            currentValue[i] = pattern[i];
        }
        for (int i = bitsToSet; i < width; i++) {
            currentValue[i] = false;
        }
    } else {
        // Default to single bit set
        std::vector<bool> defaultPattern(width, false);
        if (width > 0) defaultPattern[0] = true;
        initialPattern = defaultPattern;
        
        for (int i = 0; i < width; i++) {
            currentValue[i] = (i == 0) ? true : false;
        }
    }
}

void TubeRingCounter::clock() {
    if (!countingEnabled) return;
    
    if (countUp) {
        // Shift left (rotate): move each bit to the next position, wrap around
        bool lastBit = currentValue[width-1];
        for (int i = width-1; i > 0; i--) {
            currentValue[i] = currentValue[i-1];
        }
        currentValue[0] = lastBit;
    } else {
        // Shift right (rotate): move each bit to the previous position, wrap around
        bool firstBit = currentValue[0];
        for (int i = 0; i < width-1; i++) {
            currentValue[i] = currentValue[i+1];
        }
        currentValue[width-1] = firstBit;
    }
}

void TubeRingCounter::processOperation() {
    // Ring counter operation handled in clock method
}


// TubeJohnsonCounter implementation
TubeJohnsonCounter::TubeJohnsonCounter(int width) : TubeCounter(width) {
    // Initialize to all zeros
    std::vector<bool> pattern(width, false);
    if (width > 0) pattern[0] = true;  // Start with first bit high
    initializePattern(pattern);
}

void TubeJohnsonCounter::initializePattern(const std::vector<bool>& pattern) {
    if (!pattern.empty()) {
        initialPattern = pattern;
        int bitsToSet = std::min(static_cast<int>(pattern.size()), width);
        
        // Set to initial pattern
        for (int i = 0; i < bitsToSet; i++) {
            currentValue[i] = pattern[i];
        }
        for (int i = bitsToSet; i < width; i++) {
            currentValue[i] = false;
        }
    } else {
        // Default to first bit set
        std::vector<bool> defaultPattern(width, false);
        if (width > 0) defaultPattern[0] = true;
        initialPattern = defaultPattern;
        
        for (int i = 0; i < width; i++) {
            currentValue[i] = (i == 0) ? true : false;
        }
    }
}

void TubeJohnsonCounter::clock() {
    if (!countingEnabled) return;
    
    if (countUp) {
        // Johnson counter: shift left, feedback inverted last bit to first
        bool invertedLastBit = !currentValue[width-1];
        for (int i = width-1; i > 0; i--) {
            currentValue[i] = currentValue[i-1];
        }
        currentValue[0] = invertedLastBit;
    } else {
        // Reverse Johnson: shift right, feedback inverted first bit to last
        bool invertedFirstBit = !currentValue[0];
        for (int i = 0; i < width-1; i++) {
            currentValue[i] = currentValue[i+1];
        }
        currentValue[width-1] = invertedFirstBit;
    }
}

void TubeJohnsonCounter::processOperation() {
    // Johnson counter operation handled in clock method
}


// TubeBufferRegister implementation
TubeBufferRegister::TubeBufferRegister(int width) : width(width) {
    if (width < 1) width = 1;
    if (width > 32) width = 32;
    
    initialize();
}

void TubeBufferRegister::initialize() {
    // Create flip-flops
    flipFlops.resize(width);
    currentValue.resize(width, false);
    
    for (int i = 0; i < width; i++) {
        flipFlops[i] = std::make_unique<TubeDFlipFlop>();
        inputPins.push_back(i);      // Data input pins
        outputPins.push_back(width + i);  // Data output pins
    }
    
    clockPin = 2 * width;            // Clock pin
    loadPin = 2 * width + 1;         // Load pin
    outputEnablePin = 2 * width + 2; // Output enable pin
}

bool TubeBufferRegister::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeBufferRegister::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    // Handle data inputs
    for (int i = 0; i < width; i++) {
        if (conn_id == static_cast<uint16>(inputPins[i]) && data_bytes == sizeof(double)) {
            bool value = voltageToLogic(*reinterpret_cast<double*>(data));
            // Store pending input but don't update until clock
            // In a real implementation, this would connect to the flip-flop's D input
            return true;
        }
    }
    
    if (conn_id == clockPin && data_bytes == sizeof(double)) {
        // Clock all flip-flops simultaneously
        bool clockSignal = voltageToLogic(*reinterpret_cast<double*>(data));
        // This would clock all flip-flops in a real implementation
        return true;
    } else if (conn_id == loadPin && data_bytes == sizeof(double)) {
        // Load enable signal
        bool loadSignal = voltageToLogic(*reinterpret_cast<double*>(data));
        return true;
    } else if (conn_id == outputEnablePin && data_bytes == sizeof(double)) {
        bool enableSignal = voltageToLogic(*reinterpret_cast<double*>(data));
        outputEnabled = enableSignal;
        return true;
    }
    return false;
}

bool TubeBufferRegister::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    // Handle data outputs
    for (int i = 0; i < width; i++) {
        if (conn_id == static_cast<uint16>(outputPins[i]) && data_bytes == sizeof(double)) {
            bool output = outputEnabled ? currentValue[i] : false;
            double outputVoltage = logicToVoltage(output);
            memcpy(data, &outputVoltage, sizeof(double));
            return true;
        }
    }
    return false;
}

bool TubeBufferRegister::Tick() {
    updateOutputs();
    return true;
}

void TubeBufferRegister::load(const std::vector<bool>& data) {
    int bitsToLoad = std::min(static_cast<int>(data.size()), width);
    for (int i = 0; i < bitsToLoad; i++) {
        currentValue[i] = data[i];
    }
    // In a real implementation, this would cause the flip-flops to update
}

void TubeBufferRegister::updateOutputs() {
    // Outputs are updated based on internal state and output enable
    // This is mainly handled in GetRaw when requested
}


// TubeUniversalShiftRegister implementation
TubeUniversalShiftRegister::TubeUniversalShiftRegister(int width) : width(width) {
    if (width < 2) width = 2;  // Need at least 2 bits for shifting
    if (width > 16) width = 16;  // Reasonable limit
    
    initialize();
}

void TubeUniversalShiftRegister::initialize() {
    // Create flip-flops
    flipFlops.resize(width);
    currentValue.resize(width, false);
    parallelData.resize(width, false);
    
    for (int i = 0; i < width; i++) {
        flipFlops[i] = std::make_unique<TubeDFlipFlop>();
        parallelInputPins.push_back(i);
    }
    
    // Set up control pins
    clockPin = width;        // Clock
    modePin0 = width + 1;    // Mode select 0
    modePin1 = width + 2;    // Mode select 1
    dataRightPin = width + 3; // Serial input from right
    dataLeftPin = width + 4;  // Serial input from left
}

bool TubeUniversalShiftRegister::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeUniversalShiftRegister::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == clockPin && data_bytes == sizeof(double)) {
        bool clockSignal = voltageToLogic(*reinterpret_cast<double*>(data));
        
        // Detect rising edge
        if (!clockPrev && clockSignal) {
            clock();
        }
        clockPrev = clockSignal;
        return true;
    } else if (conn_id == modePin0 && data_bytes == sizeof(double)) {
        bool mode0 = voltageToLogic(*reinterpret_cast<double*>(data));
        // Update mode based on both mode pins
        bool mode1 = shiftMode & 0x02 ? true : false;
        int modeBits = (mode1 ? 2 : 0) | (mode0 ? 1 : 0);
        shiftMode = static_cast<ShiftMode>(modeBits);
        return true;
    } else if (conn_id == modePin1 && data_bytes == sizeof(double)) {
        bool mode1 = voltageToLogic(*reinterpret_cast<double*>(data));
        // Update mode based on both mode pins
        bool mode0 = shiftMode & 0x01 ? true : false;
        int modeBits = (mode1 ? 2 : 0) | (mode0 ? 1 : 0);
        shiftMode = static_cast<ShiftMode>(modeBits);
        return true;
    } else if (conn_id == dataRightPin && data_bytes == sizeof(double)) {
        serialInputRight = voltageToLogic(*reinterpret_cast<double*>(data));
        return true;
    } else if (conn_id == dataLeftPin && data_bytes == sizeof(double)) {
        serialInputLeft = voltageToLogic(*reinterpret_cast<double*>(data));
        return true;
    } else {
        // Handle parallel inputs
        for (int i = 0; i < width; i++) {
            if (conn_id == static_cast<uint16>(parallelInputPins[i]) && data_bytes == sizeof(double)) {
                parallelData[i] = voltageToLogic(*reinterpret_cast<double*>(data));
                return true;
            }
        }
    }
    return false;
}

bool TubeUniversalShiftRegister::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == 0 && data_bytes == sizeof(double)) {  // Return value as double array
        // This is a simplified response - in practice, individual pins would be read
        double count = static_cast<double>(getCount());
        memcpy(data, &count, sizeof(double));
        return true;
    }
    return false;
}

bool TubeUniversalShiftRegister::Tick() {
    processOperation();
    return true;
}

void TubeUniversalShiftRegister::setParallelData(const std::vector<bool>& data) {
    int bitsToSet = std::min(static_cast<int>(data.size()), width);
    for (int i = 0; i < bitsToSet; i++) {
        parallelData[i] = data[i];
    }
}

void TubeUniversalShiftRegister::clock() {
    if (shiftMode == NOP) {
        // No operation - keep current state
        return;
    } else if (shiftMode == LOAD) {
        // Parallel load
        for (int i = 0; i < width; i++) {
            currentValue[i] = parallelData[i];
        }
    } else if (shiftMode == RIGHT) {
        // Shift right (Q[n] gets Q[n-1], Q[0] gets serial input)
        for (int i = width - 1; i > 0; i--) {
            currentValue[i] = currentValue[i-1];
        }
        currentValue[0] = serialInputRight;
    } else if (shiftMode == LEFT) {
        // Shift left (Q[0] gets Q[1], Q[n] gets serial input)
        for (int i = 0; i < width - 1; i++) {
            currentValue[i] = currentValue[i+1];
        }
        currentValue[width-1] = serialInputLeft;
    }
}

void TubeUniversalShiftRegister::processOperation() {
    // Continuous operations for the shift register
    // Main operations happen on clock
}


// TubeClockDivider implementation
TubeClockDivider::TubeClockDivider(int divideBy) : TubeCounter(32), divideBy(divideBy) {  // Use 32-bit counter for large divisions
    setDivisionFactor(divideBy);
    reset();
}

void TubeClockDivider::processOperation() {
    // Clock divider operation handled in clock method
}

void TubeClockDivider::clock() {
    if (!countingEnabled) return;
    
    // Increment the counter
    int current = getCount();
    current++;
    
    // Check if we've reached the division factor
    if (current >= divideBy) {
        current = 0;
        dividedClock = !dividedClock;  // Toggle the divided clock
    }
    
    // Store the new count value
    for (int i = 0; i < width; i++) {
        currentValue[i] = (current >> i) & 1;
    }
}

bool TubeClockDivider::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == 0 && data_bytes == sizeof(double)) {  // Return divided clock
        double output = dividedClock ? highLevel : lowLevel;
        memcpy(data, &output, sizeof(double));
        return true;
    }
    return TubeCounter::GetRaw(conn_id, data, data_bytes, data_bits);
}