#include "TubeLogicGates.h"
#include "TubeModels.h"
#include <numeric>

// TubeLogicGate implementation
TubeLogicGate::TubeLogicGate() {
    // Initialize with a single input as default
    inputPins = {0};
    outputPin = 1;
    bPlusPin = 2;
    groundPin = 3;
    
    // Initialize input states
    inputStates.resize(1, 0.0);
}

bool TubeLogicGate::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeLogicGate::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    // Check if this is an input pin
    for (size_t i = 0; i < inputPins.size(); i++) {
        if (conn_id == static_cast<uint16>(inputPins[i]) && data_bytes == sizeof(double)) {
            memcpy(&inputStates[i], data, sizeof(double));
            return true;
        }
    }
    
    // Handle power supply connections
    if (conn_id == bPlusPin && data_bytes == sizeof(double)) {
        // B+ voltage - might affect logic levels in real implementation
        return true;
    }
    
    return false;
}

bool TubeLogicGate::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == static_cast<uint16>(outputPin) && data_bytes == sizeof(double)) {
        memcpy(data, &outputState, sizeof(double));
        return true;
    }
    return false;
}

bool TubeLogicGate::Tick() {
    updateOutput();
    return true;
}

void TubeLogicGate::updateOutput() {
    int result = performLogicOperation();
    outputState = logicToVoltage(result);
    
    // Apply tube characteristics - slight delay and soft transitions
    // In a real simulation, we would model the tube's dynamic response
    static double previousOutput = 0.0;
    outputState = 0.7 * outputState + 0.3 * previousOutput;
    previousOutput = outputState;
}


// TubeNOTGate implementation
TubeNOTGate::TubeNOTGate() {
    inputPins = {0};
    outputPin = 1;
    bPlusPin = 2;
    groundPin = 3;
    
    inputStates.resize(1, 0.0);
}

int TubeNOTGate::performLogicOperation() {
    // NOT operation: output = !input
    int input = voltageToLogic(inputStates[0]);
    return input ? 0 : 1;
}


// TubeORGate implementation
TubeORGate::TubeORGate(int inputs) {
    setInputs(inputs);
}

void TubeORGate::setInputs(int numInputs) {
    numInputs = std::max(2, std::min(8, numInputs)); // Limit to 2-8 inputs
    inputPins.resize(numInputs);
    for (int i = 0; i < numInputs; i++) {
        inputPins[i] = i;
    }
    
    outputPin = numInputs;
    bPlusPin = numInputs + 1;
    groundPin = numInputs + 2;
    
    inputStates.resize(numInputs, 0.0);
}

int TubeORGate::performLogicOperation() {
    for (double input : inputStates) {
        if (voltageToLogic(input) == 1) {
            return 1;  // OR returns 1 if any input is 1
        }
    }
    return 0;  // All inputs are 0
}


// TubeANDGate implementation
TubeANDGate::TubeANDGate(int inputs) {
    setInputs(inputs);
}

void TubeANDGate::setInputs(int numInputs) {
    numInputs = std::max(2, std::min(8, numInputs)); // Limit to 2-8 inputs
    inputPins.resize(numInputs);
    for (int i = 0; i < numInputs; i++) {
        inputPins[i] = i;
    }
    
    outputPin = numInputs;
    bPlusPin = numInputs + 1;
    groundPin = numInputs + 2;
    
    inputStates.resize(numInputs, 0.0);
}

int TubeANDGate::performLogicOperation() {
    for (double input : inputStates) {
        if (voltageToLogic(input) == 0) {
            return 0;  // AND returns 0 if any input is 0
        }
    }
    return 1;  // All inputs are 1
}


// TubeNANDGate implementation
TubeNANDGate::TubeNANDGate(int inputs) {
    setInputs(inputs);
}

void TubeNANDGate::setInputs(int numInputs) {
    numInputs = std::max(2, std::min(8, numInputs)); // Limit to 2-8 inputs
    inputPins.resize(numInputs);
    for (int i = 0; i < numInputs; i++) {
        inputPins[i] = i;
    }
    
    outputPin = numInputs;
    bPlusPin = numInputs + 1;
    groundPin = numInputs + 2;
    
    inputStates.resize(numInputs, 0.0);
}

int TubeNANDGate::performLogicOperation() {
    for (double input : inputStates) {
        if (voltageToLogic(input) == 0) {
            return 1;  // NAND returns 1 if any input is 0
        }
    }
    return 0;  // All inputs are 1, so output is 0
}


// TubeNORGate implementation
TubeNORGate::TubeNORGate(int inputs) {
    setInputs(inputs);
}

void TubeNORGate::setInputs(int numInputs) {
    numInputs = std::max(2, std::min(8, numInputs)); // Limit to 2-8 inputs
    inputPins.resize(numInputs);
    for (int i = 0; i < numInputs; i++) {
        inputPins[i] = i;
    }
    
    outputPin = numInputs;
    bPlusPin = numInputs + 1;
    groundPin = numInputs + 2;
    
    inputStates.resize(numInputs, 0.0);
}

int TubeNORGate::performLogicOperation() {
    for (double input : inputStates) {
        if (voltageToLogic(input) == 1) {
            return 0;  // NOR returns 0 if any input is 1
        }
    }
    return 1;  // All inputs are 0
}


// TubeXORGate implementation
TubeXORGate::TubeXORGate() {
    inputPins = {0, 1};  // Two inputs for XOR
    outputPin = 2;
    bPlusPin = 3;
    groundPin = 4;
    
    inputStates.resize(2, 0.0);
}

int TubeXORGate::performLogicOperation() {
    int input1 = voltageToLogic(inputStates[0]);
    int input2 = voltageToLogic(inputStates[1]);
    
    // XOR returns 1 if inputs are different
    return input1 != input2 ? 1 : 0;
}


// TubeLogicFamily static members
double TubeLogicFamily::standardLowLevel = 0.0;
double TubeLogicFamily::standardHighLevel = 5.0;
double TubeLogicFamily::standardThreshold = 2.5;

std::unique_ptr<TubeNOTGate> TubeLogicFamily::createNOT() {
    auto gate = std::make_unique<TubeNOTGate>();
    gate->setLowLevel(standardLowLevel);
    gate->setHighLevel(standardHighLevel);
    gate->setThreshold(standardThreshold);
    return gate;
}

std::unique_ptr<TubeORGate> TubeLogicFamily::createOR(int inputs) {
    auto gate = std::make_unique<TubeORGate>(inputs);
    gate->setLowLevel(standardLowLevel);
    gate->setHighLevel(standardHighLevel);
    gate->setThreshold(standardThreshold);
    return gate;
}

std::unique_ptr<TubeANDGate> TubeLogicFamily::createAND(int inputs) {
    auto gate = std::make_unique<TubeANDGate>(inputs);
    gate->setLowLevel(standardLowLevel);
    gate->setHighLevel(standardHighLevel);
    gate->setThreshold(standardThreshold);
    return gate;
}

std::unique_ptr<TubeNANDGate> TubeLogicFamily::createNAND(int inputs) {
    auto gate = std::make_unique<TubeNANDGate>(inputs);
    gate->setLowLevel(standardLowLevel);
    gate->setHighLevel(standardHighLevel);
    gate->setThreshold(standardThreshold);
    return gate;
}

std::unique_ptr<TubeNORGate> TubeLogicFamily::createNOR(int inputs) {
    auto gate = std::make_unique<TubeNORGate>(inputs);
    gate->setLowLevel(standardLowLevel);
    gate->setHighLevel(standardHighLevel);
    gate->setThreshold(standardThreshold);
    return gate;
}

std::unique_ptr<TubeXORGate> TubeLogicFamily::createXOR() {
    auto gate = std::make_unique<TubeXORGate>();
    gate->setLowLevel(standardLowLevel);
    gate->setHighLevel(standardHighLevel);
    gate->setThreshold(standardThreshold);
    return gate;
}

void TubeLogicFamily::setStandardVoltageLevels(double low, double high, double threshold) {
    standardLowLevel = low;
    standardHighLevel = high;
    standardThreshold = threshold;
}


// TubeHalfAdder implementation
TubeHalfAdder::TubeHalfAdder() {
    xorGate = std::make_unique<TubeXORGate>();
    andGate = std::make_unique<TubeANDGate>(2);
    
    // Set the same pin connections for internal gates
    xorGate->setInputPins({inputAPin, inputBPin});
    andGate->setInputs(2);  // Sets pins 0,1 as inputs
}

bool TubeHalfAdder::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeHalfAdder::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == inputAPin && data_bytes == sizeof(double)) {
        memcpy(&inputA, data, sizeof(double));
        return true;
    } else if (conn_id == inputBPin && data_bytes == sizeof(double)) {
        memcpy(&inputB, data, sizeof(double));
        return true;
    }
    return false;
}

bool TubeHalfAdder::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == sumPin && data_bytes == sizeof(double)) {
        memcpy(data, &sumOutput, sizeof(double));
        return true;
    } else if (conn_id == carryPin && data_bytes == sizeof(double)) {
        memcpy(data, &carryOutput, sizeof(double));
        return true;
    }
    return false;
}

bool TubeHalfAdder::Tick() {
    process();
    return true;
}

void TubeHalfAdder::process() {
    // Calculate sum using XOR gate
    int a_logic = xorGate->voltageToLogic(inputA);
    int b_logic = xorGate->voltageToLogic(inputB);
    int sum_logic = a_logic ^ b_logic;
    sumOutput = xorGate->logicToVoltage(sum_logic);
    
    // Calculate carry using AND gate
    int carry_logic = a_logic & b_logic;
    carryOutput = andGate->logicToVoltage(carry_logic);
}


// TubeFullAdder implementation
TubeFullAdder::TubeFullAdder() {
    ha1 = std::make_unique<TubeHalfAdder>();
    ha2 = std::make_unique<TubeHalfAdder>();
    orGate = std::make_unique<TubeORGate>(2);
    
    // Connect internal gates appropriately
    // This is a conceptual model; actual pin connections would be more complex
}

bool TubeFullAdder::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeFullAdder::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == inputAPin && data_bytes == sizeof(double)) {
        memcpy(&inputA, data, sizeof(double));
        return true;
    } else if (conn_id == inputBPin && data_bytes == sizeof(double)) {
        memcpy(&inputB, data, sizeof(double));
        return true;
    } else if (conn_id == inputCinPin && data_bytes == sizeof(double)) {
        memcpy(&inputCin, data, sizeof(double));
        return true;
    }
    return false;
}

bool TubeFullAdder::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == sumPin && data_bytes == sizeof(double)) {
        memcpy(data, &sumOutput, sizeof(double));
        return true;
    } else if (conn_id == carryOutPin && data_bytes == sizeof(double)) {
        memcpy(data, &carryOutput, sizeof(double));
        return true;
    }
    return false;
}

bool TubeFullAdder::Tick() {
    process();
    return true;
}

void TubeFullAdder::process() {
    // Full adder using two half adders:
    // First half adder adds A and B
    int a_logic = ha1->xorGate->voltageToLogic(inputA);
    int b_logic = ha1->xorGate->voltageToLogic(inputB);
    int sum_ab = a_logic ^ b_logic;
    int carry_ab = a_logic & b_logic;
    
    // Second half adder adds (A XOR B) and Cin
    int cin_logic = ha2->xorGate->voltageToLogic(inputCin);
    int final_sum = sum_ab ^ cin_logic;
    int carry_ac = sum_ab & cin_logic;
    
    // Final carry is (AB) OR (AC)
    int final_carry = carry_ab | carry_ac;
    
    // Convert back to voltages
    sumOutput = ha1->xorGate->logicToVoltage(final_sum);
    carryOutput = orGate->logicToVoltage(final_carry);
}