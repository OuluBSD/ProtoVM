#include "TubeFlipFlops.h"
#include "TubeLogicGates.h"

// TubeLatchFlipFlop implementation
TubeLatchFlipFlop::TubeLatchFlipFlop() {
    // Initialize state
    qState = false;
    qBarState = true;
}

bool TubeLatchFlipFlop::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeLatchFlipFlop::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == dataPin && data_bytes == sizeof(double)) {
        // For latch/flip-flop, handle data input
        double value;
        memcpy(&value, data, sizeof(double));
        // This would be handled in derived classes
        return true;
    } else if (conn_id == clockPin && data_bytes == sizeof(double)) {
        // For latch/flip-flop, handle clock input
        double value;
        memcpy(&value, data, sizeof(double));
        // This would be handled in derived classes
        return true;
    } else if (conn_id == setPin && data_bytes == sizeof(double)) {
        double value;
        memcpy(&value, data, sizeof(double));
        if (voltageToLogic(value)) {
            qState = true;
            qBarState = false;
        }
        return true;
    } else if (conn_id == resetPin && data_bytes == sizeof(double)) {
        double value;
        memcpy(&value, data, sizeof(double));
        if (voltageToLogic(value)) {
            qState = false;
            qBarState = true;
        }
        return true;
    } else if (conn_id == bPlusPin && data_bytes == sizeof(double)) {
        // Handle power supply
        return true;
    }
    return false;
}

bool TubeLatchFlipFlop::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == qPin && data_bytes == sizeof(double)) {
        double output = logicToVoltage(qState);
        memcpy(data, &output, sizeof(double));
        return true;
    } else if (conn_id == qBarPin && data_bytes == sizeof(double)) {
        double output = logicToVoltage(qBarState);
        memcpy(data, &output, sizeof(double));
        return true;
    }
    return false;
}

bool TubeLatchFlipFlop::Tick() {
    processOperation();
    updateOutputs();
    return true;
}

void TubeLatchFlipFlop::updateOutputs() {
    // Outputs are updated based on internal state
    // This is called after processOperation()
}


// TubeSRLatch implementation
TubeSRLatch::TubeSRLatch() {
    // Set pin assignments
    clockPin = -1;  // SR latch doesn't have a clock
    dataPin = -1;   // No single data pin
    setPin = 0;     // S input
    resetPin = 1;   // R input
    qPin = 2;       // Q output
    qBarPin = 3;    // Q bar output
    bPlusPin = 4;   // Power
    groundPin = 5;  // Ground
}

void TubeSRLatch::processOperation() {
    // The logic is handled in PutRaw when inputs change
    // This is a simple SR latch: if S=1, R=0 -> Q=1; if S=0, R=1 -> Q=0; if S=R=0 -> no change; S=R=1 -> invalid
}


// TubeDLatch implementation
TubeDLatch::TubeDLatch() {
    // Set pin assignments
    clockPin = 0;    // Enable/Clock input
    dataPin = 1;     // D input
    setPin = 2;      // Set input (asynchronous)
    resetPin = 3;    // Reset input (asynchronous)
    qPin = 4;        // Q output
    qBarPin = 5;     // Q bar output
    bPlusPin = 6;    // Power
    groundPin = 7;   // Ground
}

bool TubeDLatch::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == dataPin && data_bytes == sizeof(double)) {
        double value;
        memcpy(&value, data, sizeof(double));
        dInput = voltageToLogic(value);
        return true;
    } else if (conn_id == clockPin && data_bytes == sizeof(double)) {
        double value;
        memcpy(&value, data, sizeof(double));
        enableInput = voltageToLogic(value);
        // If enable is high, latch the D input
        if (enableInput) {
            qState = dInput;
            qBarState = !dInput;
        }
        return true;
    } else if (conn_id == setPin && data_bytes == sizeof(double)) {
        double value;
        memcpy(&value, data, sizeof(double));
        if (voltageToLogic(value)) {
            qState = true;
            qBarState = false;
        }
        return true;
    } else if (conn_id == resetPin && data_bytes == sizeof(double)) {
        double value;
        memcpy(&value, data, sizeof(double));
        if (voltageToLogic(value)) {
            qState = false;
            qBarState = true;
        }
        return true;
    } else if (conn_id == bPlusPin && data_bytes == sizeof(double)) {
        return true;
    }
    return false;
}

void TubeDLatch::processOperation() {
    // For a D latch, when enable is high, Q follows D
    // When enable is low, Q holds its value
    // This is handled in PutRaw
}


// TubeDFlipFlop implementation
TubeDFlipFlop::TubeDFlipFlop() {
    // Set pin assignments
    clockPin = 0;    // Clock input
    dataPin = 1;     // D input
    setPin = 2;      // Set input (asynchronous)
    resetPin = 3;    // Reset input (asynchronous)
    qPin = 4;        // Q output
    qBarPin = 5;     // Q bar output
    bPlusPin = 6;    // Power
    groundPin = 7;   // Ground
    
    // Initialize states
    masterState = false;
    slaveState = false;
    previousClock = false;
    qState = false;
    qBarState = true;
}

bool TubeDFlipFlop::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == dataPin && data_bytes == sizeof(double)) {
        double value;
        memcpy(&value, data, sizeof(double));
        dInput = voltageToLogic(value);
        return true;
    } else if (conn_id == clockPin && data_bytes == sizeof(double)) {
        double value;
        memcpy(&value, data, sizeof(double));
        clockInput = voltageToLogic(value);
        
        // Detect clock edge
        bool clockEdge = false;
        if (risingEdgeTriggered && !previousClock && clockInput) {
            clockEdge = true;  // Rising edge
        } else if (!risingEdgeTriggered && previousClock && !clockInput) {
            clockEdge = true;  // Falling edge
        }
        
        if (clockEdge) {
            // On clock edge, transfer D to master, then master to slave
            masterState = dInput;
            slaveState = masterState;
            qState = slaveState;
            qBarState = !slaveState;
        }
        
        previousClock = clockInput;
        return true;
    } else if (conn_id == setPin && data_bytes == sizeof(double)) {
        double value;
        memcpy(&value, data, sizeof(double));
        if (voltageToLogic(value)) {
            qState = true;
            qBarState = false;
            masterState = true;
            slaveState = true;
        }
        return true;
    } else if (conn_id == resetPin && data_bytes == sizeof(double)) {
        double value;
        memcpy(&value, data, sizeof(double));
        if (voltageToLogic(value)) {
            qState = false;
            qBarState = true;
            masterState = false;
            slaveState = false;
        }
        return true;
    } else if (conn_id == bPlusPin && data_bytes == sizeof(double)) {
        return true;
    }
    return false;
}

void TubeDFlipFlop::processOperation() {
    // The operation is handled in PutRaw for edge-based updates
    // This maintains the current state between clock edges
}


// TubeJKFlipFlop implementation
TubeJKFlipFlop::TubeJKFlipFlop() {
    // Set pin assignments
    clockPin = 0;    // Clock input
    dataPin = -1;    // J input (will repurpose dataPin)
    int jPin = 1;    // J input
    int kPin = 2;    // K input
    setPin = 3;      // Set input (asynchronous)
    resetPin = 4;    // Reset input (asynchronous)
    qPin = 5;        // Q output
    qBarPin = 6;     // Q bar output
    bPlusPin = 7;    // Power
    groundPin = 8;   // Ground
}

bool TubeJKFlipFlop::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == 1 && data_bytes == sizeof(double)) {  // J input
        double value;
        memcpy(&value, data, sizeof(double));
        jInput = voltageToLogic(value);
        return true;
    } else if (conn_id == 2 && data_bytes == sizeof(double)) {  // K input
        double value;
        memcpy(&value, data, sizeof(double));
        kInput = voltageToLogic(value);
        return true;
    } else if (conn_id == clockPin && data_bytes == sizeof(double)) {
        double value;
        memcpy(&value, data, sizeof(double));
        clockInput = voltageToLogic(value);
        
        // Detect clock edge
        bool clockEdge = false;
        if (risingEdgeTriggered && !previousClock && clockInput) {
            clockEdge = true;  // Rising edge
        } else if (!risingEdgeTriggered && previousClock && !clockInput) {
            clockEdge = true;  // Falling edge
        }
        
        if (clockEdge) {
            // Apply JK flip-flop logic:
            // J=0, K=0: no change
            // J=0, K=1: reset (Q=0)
            // J=1, K=0: set (Q=1)
            // J=1, K=1: toggle
            if (!jInput && !kInput) {
                // No change
            } else if (!jInput && kInput) {
                // Reset
                qState = false;
                qBarState = true;
            } else if (jInput && !kInput) {
                // Set
                qState = true;
                qBarState = false;
            } else if (jInput && kInput) {
                // Toggle
                qState = !qState;
                qBarState = !qBarState;
            }
        }
        
        previousClock = clockInput;
        return true;
    } else if (conn_id == setPin && data_bytes == sizeof(double)) {
        double value;
        memcpy(&value, data, sizeof(double));
        if (voltageToLogic(value)) {
            qState = true;
            qBarState = false;
        }
        return true;
    } else if (conn_id == resetPin && data_bytes == sizeof(double)) {
        double value;
        memcpy(&value, data, sizeof(double));
        if (voltageToLogic(value)) {
            qState = false;
            qBarState = true;
        }
        return true;
    } else if (conn_id == bPlusPin && data_bytes == sizeof(double)) {
        return true;
    }
    return false;
}

void TubeJKFlipFlop::processOperation() {
    // The operation is handled in PutRaw for edge-based updates
}


// TubeTFlipFlop implementation
TubeTFlipFlop::TubeTFlipFlop() {
    // Set pin assignments
    clockPin = 0;    // Clock input
    dataPin = 1;     // T input
    setPin = 2;      // Set input (asynchronous)
    resetPin = 3;    // Reset input (asynchronous)
    qPin = 4;        // Q output
    qBarPin = 5;     // Q bar output
    bPlusPin = 6;    // Power
    groundPin = 7;   // Ground
}

bool TubeTFlipFlop::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == dataPin && data_bytes == sizeof(double)) {
        double value;
        memcpy(&value, data, sizeof(double));
        tInput = voltageToLogic(value);
        return true;
    } else if (conn_id == clockPin && data_bytes == sizeof(double)) {
        double value;
        memcpy(&value, data, sizeof(double));
        clockInput = voltageToLogic(value);
        
        // Detect clock edge
        bool clockEdge = false;
        if (risingEdgeTriggered && !previousClock && clockInput) {
            clockEdge = true;  // Rising edge
        } else if (!risingEdgeTriggered && previousClock && !clockInput) {
            clockEdge = true;  // Falling edge
        }
        
        if (clockEdge && tInput) {
            // Toggle Q when T=1 and clock edge occurs
            qState = !qState;
            qBarState = !qBarState;
        }
        
        previousClock = clockInput;
        return true;
    } else if (conn_id == setPin && data_bytes == sizeof(double)) {
        double value;
        memcpy(&value, data, sizeof(double));
        if (voltageToLogic(value)) {
            qState = true;
            qBarState = false;
        }
        return true;
    } else if (conn_id == resetPin && data_bytes == sizeof(double)) {
        double value;
        memcpy(&value, data, sizeof(double));
        if (voltageToLogic(value)) {
            qState = false;
            qBarState = true;
        }
        return true;
    } else if (conn_id == bPlusPin && data_bytes == sizeof(double)) {
        return true;
    }
    return false;
}

void TubeTFlipFlop::processOperation() {
    // The operation is handled in PutRaw for edge-based updates
}


// TubeRegister implementation
TubeRegister::TubeRegister(int width) : width(width) {
    flipFlops.resize(width);
    currentValue.resize(width, false);
    
    // Create D flip-flops for each bit
    for (int i = 0; i < width; i++) {
        flipFlops[i] = std::make_unique<TubeDFlipFlop>();
    }
}

void TubeRegister::setInput(const std::vector<bool>& data) {
    int bitsToSet = std::min(static_cast<int>(data.size()), width);
    for (int i = 0; i < bitsToSet; i++) {
        // In a real implementation, we would connect these to the flip-flop's data pin
        // For this simulation, we'll just store the values
        currentValue[i] = data[i];
    }
}

void TubeRegister::setInput(int bit, bool value) {
    if (bit >= 0 && bit < width) {
        currentValue[bit] = value;
    }
}

void TubeRegister::clock(bool clockSignal) {
    // Apply clock to all flip-flops
    // In a real system, each flip-flop would be connected to the clock
    for (int i = 0; i < width; i++) {
        // Simulate applying clock to each flip-flop
        // This would be done by passing the clock signal to each flip-flop's clock pin
        if (clockSignal) {
            // Update stored value with the input value
            // In a real implementation, the flip-flop would latch its D input
            flipFlops[i]->PutRaw(0, reinterpret_cast<byte*>(&clockSignal), sizeof(bool), 0); // This is simplified
        }
    }
    updateValue();
}

void TubeRegister::setAll() {
    for (int i = 0; i < width; i++) {
        bool setSignal = true;
        flipFlops[i]->PutRaw(2, reinterpret_cast<byte*>(&setSignal), sizeof(bool), 0); // Set pin
    }
    updateValue();
}

void TubeRegister::resetAll() {
    for (int i = 0; i < width; i++) {
        bool resetSignal = true;
        flipFlops[i]->PutRaw(3, reinterpret_cast<byte*>(&resetSignal), sizeof(bool), 0); // Reset pin
    }
    updateValue();
}

std::vector<bool> TubeRegister::getValue() const {
    std::vector<bool> result(width);
    for (int i = 0; i < width; i++) {
        result[i] = flipFlops[i]->getQ();
    }
    return result;
}

bool TubeRegister::getValue(int bit) const {
    if (bit >= 0 && bit < width) {
        return flipFlops[bit]->getQ();
    }
    return false;
}

void TubeRegister::updateValue() {
    for (int i = 0; i < width; i++) {
        currentValue[i] = flipFlops[i]->getQ();
    }
}


// TubeShiftRegister implementation
TubeShiftRegister::TubeShiftRegister(int width) : width(width) {
    flipFlops.resize(width);
    currentValue.resize(width, false);
    
    // Create D flip-flops for each stage
    for (int i = 0; i < width; i++) {
        flipFlops[i] = std::make_unique<TubeDFlipFlop>();
    }
}

void TubeShiftRegister::shiftLeft(bool serialInput) {
    // Shift left: Q[n] = Q[n-1], Q[0] = serialInput
    for (int i = width - 1; i > 0; i--) {
        currentValue[i] = currentValue[i-1];
    }
    currentValue[0] = serialInput;
    
    // Update each flip-flop
    for (int i = 0; i < width; i++) {
        // In a real implementation, we would connect the output of each flip-flop
        // to the input of the next one
    }
}

void TubeShiftRegister::shiftRight(bool serialInput) {
    // Shift right: Q[n-1] = Q[n], Q[n] = serialInput
    for (int i = 0; i < width - 1; i++) {
        currentValue[i] = currentValue[i+1];
    }
    currentValue[width-1] = serialInput;
    
    // Update each flip-flop
    for (int i = 0; i < width; i++) {
        // In a real implementation, we would connect the output of each flip-flop
        // to the input of the next one
    }
}

void TubeShiftRegister::clock(bool clockSignal) {
    // Apply clock to all flip-flops
    for (int i = 0; i < width; i++) {
        // In a proper implementation, we would connect the output of flip-flop i-1
        // to the input of flip-flop i, and provide the serial input to the first
    }
    updateValue();
}

void TubeShiftRegister::load(const std::vector<bool>& data) {
    int bitsToLoad = std::min(static_cast<int>(data.size()), width);
    for (int i = 0; i < bitsToLoad; i++) {
        currentValue[i] = data[i];
    }
    updateValue();
}

void TubeShiftRegister::updateValue() {
    // In a real system, this would read from the flip-flop outputs
    // For simulation purposes, we maintain a current value
}