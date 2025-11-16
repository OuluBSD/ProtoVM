#include "TubeComputerSystems.h"
#include <algorithm>

// TubeALU implementation
TubeALU::TubeALU(int width) : width(width) {
    if (width < 1) width = 1;
    if (width > 32) width = 32;
    
    initializeALU();
}

void TubeALU::initializeALU() {
    operandA.resize(width, false);
    operandB.resize(width, false);
    result.resize(width, false);
    
    // Create adders for each bit position
    adders.resize(width);
    for (int i = 0; i < width; i++) {
        adders[i] = std::make_unique<TubeFullAdder>();
        inputAPins.push_back(i);
        inputBPins.push_back(width + i);
        resultPins.push_back(2 * width + i);
    }
    
    operationPin = 2 * width;
    carryInPin = 2 * width + 1;
    carryOutPin = 2 * width + 2;
    zeroFlagPin = 2 * width + 3;
    negativeFlagPin = 2 * width + 4;
    clockPin = 2 * width + 5;
}

bool TubeALU::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeALU::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    // Handle operand A inputs
    for (int i = 0; i < width; i++) {
        if (conn_id == static_cast<uint16>(inputAPins[i]) && data_bytes == sizeof(double)) {
            operandA[i] = voltageToLogic(*reinterpret_cast<double*>(data));
            return true;
        }
    }
    
    // Handle operand B inputs
    for (int i = 0; i < width; i++) {
        if (conn_id == static_cast<uint16>(inputBPins[i]) && data_bytes == sizeof(double)) {
            operandB[i] = voltageToLogic(*reinterpret_cast<double*>(data));
            return true;
        }
    }
    
    if (conn_id == operationPin && data_bytes == sizeof(double)) {
        operation = static_cast<int>(*reinterpret_cast<double*>(data));
        return true;
    }
    
    return false;
}

bool TubeALU::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
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
    
    if (conn_id == zeroFlagPin && data_bytes == sizeof(double)) {
        double voltage = logicToVoltage(zeroFlag);
        memcpy(data, &voltage, sizeof(double));
        return true;
    }
    
    if (conn_id == negativeFlagPin && data_bytes == sizeof(double)) {
        double voltage = logicToVoltage(negativeFlag);
        memcpy(data, &voltage, sizeof(double));
        return true;
    }
    
    return false;
}

bool TubeALU::Tick() {
    performOperation();
    updateFlags();
    return true;
}

void TubeALU::setOperandA(const std::vector<bool>& value) {
    int bitsToSet = std::min(static_cast<int>(value.size()), width);
    for (int i = 0; i < bitsToSet; i++) {
        operandA[i] = value[i];
    }
}

void TubeALU::setOperandB(const std::vector<bool>& value) {
    int bitsToSet = std::min(static_cast<int>(value.size()), width);
    for (int i = 0; i < bitsToSet; i++) {
        operandB[i] = value[i];
    }
}

void TubeALU::setOperation(int op) {
    operation = op;
}

void TubeALU::performOperation() {
    switch (operation) {
        case 0: {  // Addition
            bool carry = false;
            for (int i = 0; i < width; i++) {
                int sum = operandA[i] + operandB[i] + carry;
                result[i] = sum & 1;
                carry = sum > 1;
            }
            carryOut = carry;
            break;
        }
        case 1: {  // Subtraction (A - B)
            bool borrow = false;
            for (int i = 0; i < width; i++) {
                int diff = operandA[i] - operandB[i] - borrow;
                result[i] = diff & 1;
                borrow = diff < 0;
            }
            carryOut = !borrow;  // Invert for typical carry/borrow behavior
            break;
        }
        case 2: {  // AND
            for (int i = 0; i < width; i++) {
                result[i] = operandA[i] && operandB[i];
            }
            carryOut = false;
            break;
        }
        case 3: {  // OR
            for (int i = 0; i < width; i++) {
                result[i] = operandA[i] || operandB[i];
            }
            carryOut = false;
            break;
        }
        case 4: {  // XOR
            for (int i = 0; i < width; i++) {
                result[i] = operandA[i] ^ operandB[i];
            }
            carryOut = false;
            break;
        }
        default:
            // NOP - keep current result
            break;
    }
}

void TubeALU::updateFlags() {
    // Zero flag: set if result is all zeros
    zeroFlag = true;
    for (int i = 0; i < width; i++) {
        if (result[i]) {
            zeroFlag = false;
            break;
        }
    }
    
    // Negative flag: set based on most significant bit
    negativeFlag = result[width - 1];
}


// TubeMemory implementation
TubeMemory::TubeMemory(int addrWidth, int dataWidth) 
    : addrWidth(addrWidth), dataWidth(dataWidth) {
    if (addrWidth < 1) addrWidth = 1;
    if (addrWidth > 10) addrWidth = 10;  // Limit to 1024 locations
    if (dataWidth < 1) dataWidth = 1;
    if (dataWidth > 32) dataWidth = 32;
    
    memSize = 1 << addrWidth;
    initializeMemory();
}

void TubeMemory::initializeMemory() {
    // Initialize memory array
    memoryArray.resize(memSize, std::vector<bool>(dataWidth, false));
    
    // Initialize with default values
    address.resize(addrWidth, false);
    writeData.resize(dataWidth, false);
    readData.resize(dataWidth, false);
    
    // Create storage registers for each memory location
    storageRegisters.resize(memSize);
    for (int i = 0; i < memSize; i++) {
        storageRegisters[i] = std::make_unique<TubeRegister>(dataWidth);
    }
    
    // Set up pin assignments
    for (int i = 0; i < addrWidth; i++) {
        addrPins.push_back(i);
    }
    for (int i = 0; i < dataWidth; i++) {
        writeDataPins.push_back(addrWidth + i);
        readDataPins.push_back(addrWidth + dataWidth + i);
    }
    
    writeEnablePin = addrWidth + 2 * dataWidth;
    readEnablePin = addrWidth + 2 * dataWidth + 1;
    clockPin = addrWidth + 2 * dataWidth + 2;
    chipEnablePin = addrWidth + 2 * dataWidth + 3;
}

bool TubeMemory::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeMemory::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    // Handle address inputs
    for (int i = 0; i < addrWidth; i++) {
        if (conn_id == static_cast<uint16>(addrPins[i]) && data_bytes == sizeof(double)) {
            address[i] = voltageToLogic(*reinterpret_cast<double*>(data));
            return true;
        }
    }
    
    // Handle write data inputs
    for (int i = 0; i < dataWidth; i++) {
        if (conn_id == static_cast<uint16>(writeDataPins[i]) && data_bytes == sizeof(double)) {
            writeData[i] = voltageToLogic(*reinterpret_cast<double*>(data));
            return true;
        }
    }
    
    if (conn_id == writeEnablePin && data_bytes == sizeof(double)) {
        writeEnable = voltageToLogic(*reinterpret_cast<double*>(data));
        if (writeEnable) {
            accessMemory();
        }
        return true;
    }
    
    if (conn_id == readEnablePin && data_bytes == sizeof(double)) {
        readEnable = voltageToLogic(*reinterpret_cast<double*>(data));
        if (readEnable) {
            accessMemory();
        }
        return true;
    }
    
    return false;
}

bool TubeMemory::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    // Handle read data outputs
    for (int i = 0; i < dataWidth; i++) {
        if (conn_id == static_cast<uint16>(readDataPins[i]) && data_bytes == sizeof(double)) {
            double voltage = logicToVoltage(readData[i]);
            memcpy(data, &voltage, sizeof(double));
            return true;
        }
    }
    return false;
}

bool TubeMemory::Tick() {
    accessMemory();
    return true;
}

void TubeMemory::setAddress(const std::vector<bool>& addr) {
    int bitsToSet = std::min(static_cast<int>(addr.size()), addrWidth);
    for (int i = 0; i < bitsToSet; i++) {
        address[i] = addr[i];
    }
}

void TubeMemory::setWriteData(const std::vector<bool>& data) {
    int bitsToSet = std::min(static_cast<int>(data.size()), dataWidth);
    for (int i = 0; i < bitsToSet; i++) {
        writeData[i] = data[i];
    }
}

void TubeMemory::accessMemory() {
    // Calculate memory address
    int addr = 0;
    for (int i = 0; i < addrWidth; i++) {
        if (address[i]) addr |= (1 << i);
    }
    
    if (addr >= memSize) return;  // Out of bounds
    
    if (writeEnable) {
        // Write operation
        memoryArray[addr] = writeData;
        storageRegisters[addr]->setInput(writeData);
        storageRegisters[addr]->clock(true);  // Clock the register
    }
    
    if (readEnable) {
        // Read operation
        readData = memoryArray[addr];
    }
}


// TubeAccumulator implementation
TubeAccumulator::TubeAccumulator(int width) : width(width) {
    if (width < 1) width = 1;
    if (width > 32) width = 32;
    
    initializeAccumulator();
}

void TubeAccumulator::initializeAccumulator() {
    currentValue.resize(width, false);
    registerBank = std::make_unique<TubeRegister>(width);
    alu = std::make_unique<TubeALU>(width);
    
    // Set up pin assignments
    for (int i = 0; i < width; i++) {
        inputPins.push_back(i);
        outputPins.push_back(width + i);
    }
    
    loadPin = 2 * width;
    clearPin = 2 * width + 1;
    clockPin = 2 * width + 2;
    incrementPin = 2 * width + 3;
    shiftLeftPin = 2 * width + 4;
    shiftRightPin = 2 * width + 5;
}

bool TubeAccumulator::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeAccumulator::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    // Handle data inputs
    for (int i = 0; i < width; i++) {
        if (conn_id == static_cast<uint16>(inputPins[i]) && data_bytes == sizeof(double)) {
            // Set data but don't load until load signal
            return true;
        }
    }
    
    if (conn_id == loadPin && data_bytes == sizeof(double)) {
        bool loadSignal = voltageToLogic(*reinterpret_cast<double*>(data));
        if (loadSignal) {
            std::vector<bool> dataVec(width);
            for (int i = 0; i < width; i++) {
                dataVec[i] = voltageToLogic(*reinterpret_cast<double*>(data));
            }
            load(dataVec);
        }
        return true;
    } else if (conn_id == clearPin && data_bytes == sizeof(double)) {
        bool clearSignal = voltageToLogic(*reinterpret_cast<double*>(data));
        if (clearSignal) {
            clear();
        }
        return true;
    } else if (conn_id == incrementPin && data_bytes == sizeof(double)) {
        bool incSignal = voltageToLogic(*reinterpret_cast<double*>(data));
        if (incSignal) {
            increment();
        }
        return true;
    } else if (conn_id == shiftLeftPin && data_bytes == sizeof(double)) {
        bool shiftSignal = voltageToLogic(*reinterpret_cast<double*>(data));
        if (shiftSignal) {
            shiftLeft();
        }
        return true;
    } else if (conn_id == shiftRightPin && data_bytes == sizeof(double)) {
        bool shiftSignal = voltageToLogic(*reinterpret_cast<double*>(data));
        if (shiftSignal) {
            shiftRight();
        }
        return true;
    }
    return false;
}

bool TubeAccumulator::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    // Handle data outputs
    for (int i = 0; i < width; i++) {
        if (conn_id == static_cast<uint16>(outputPins[i]) && data_bytes == sizeof(double)) {
            double voltage = logicToVoltage(currentValue[i]);
            memcpy(data, &voltage, sizeof(double));
            return true;
        }
    }
    return false;
}

bool TubeAccumulator::Tick() {
    updateFlags();
    return true;
}

void TubeAccumulator::load(const std::vector<bool>& data) {
    int bitsToSet = std::min(static_cast<int>(data.size()), width);
    for (int i = 0; i < bitsToSet; i++) {
        currentValue[i] = data[i];
    }
    registerBank->setInput(currentValue);
    registerBank->clock(true);
    updateFlags();
}

void TubeAccumulator::clear() {
    for (int i = 0; i < width; i++) {
        currentValue[i] = false;
    }
    registerBank->setInput(currentValue);
    registerBank->clock(true);
    updateFlags();
}

void TubeAccumulator::increment() {
    // Add 1 to current value
    bool carry = true;
    for (int i = 0; i < width && carry; i++) {
        bool oldBit = currentValue[i];
        currentValue[i] = !oldBit;
        carry = oldBit && currentValue[i];  // Carry if we went from 1 to 0
    }
    
    registerBank->setInput(currentValue);
    registerBank->clock(true);
    updateFlags();
}

void TubeAccumulator::shiftLeft() {
    for (int i = width - 1; i > 0; i--) {
        currentValue[i] = currentValue[i-1];
    }
    currentValue[0] = false;  // Shift in 0
    
    registerBank->setInput(currentValue);
    registerBank->clock(true);
    updateFlags();
}

void TubeAccumulator::shiftRight() {
    for (int i = 0; i < width - 1; i++) {
        currentValue[i] = currentValue[i+1];
    }
    currentValue[width-1] = false;  // Shift in 0
    
    registerBank->setInput(currentValue);
    registerBank->clock(true);
    updateFlags();
}

void TubeAccumulator::updateFlags() {
    // Zero flag: set if accumulator is all zeros
    zeroFlag = true;
    for (int i = 0; i < width; i++) {
        if (currentValue[i]) {
            zeroFlag = false;
            break;
        }
    }
    
    // Carry flag: depends on operation, for now just set to false
    carryFlag = false;
}


// TubeComputerSystem implementation
TubeComputerSystem::TubeComputerSystem(ComputerType type) : computerType(type) {
    initializeSystem(type);
}

void TubeComputerSystem::initializeSystem(ComputerType type) {
    // Initialize main components based on computer type
    switch (type) {
        case ENIAC:
            initializeENIAC();
            break;
        case COLOSSUS:
            initializeColossus();
            break;
        case MANCHESTER_SLOW:
            initializeManchesterSlow();
            break;
        case EDSAC:
            initializeEDSAC();
            break;
    }
    
    // Set up common components
    alu = std::make_unique<TubeALU>(32);  // 32-bit ALU
    memory = std::make_unique<TubeMemory>(10, 32);  // 1KB of 32-bit memory
    accumulator = std::make_unique<TubeAccumulator>(32);  // 32-bit accumulator
    
    programCounter = std::make_unique<TubeCounter>(10);  // 10-bit PC -> 1024 instructions
    instructionRegister = std::make_unique<TubeRegister>(32);  // 32-bit instruction register
    stepCounter = std::make_unique<TubeCounter>(4);  // 4-bit microcode step counter
    
    // Set initial state
    reset();
}

void TubeComputerSystem::initializeENIAC() {
    // ENIAC was primarily a calculator with plugboard programming
    // For simulation, we'll implement a basic stored-program version
    // with accumulator architecture and simple instruction set
}

void TubeComputerSystem::initializeColossus() {
    // Colossus was specialized for code-breaking
    // For simulation, focus on Boolean logic operations and counting
}

void TubeComputerSystem::initializeManchesterSlow() {
    // Manchester "Baby" was the first stored-program computer
}

void TubeComputerSystem::initializeEDSAC() {
    // EDSAC had a simple accumulator-based architecture
}

bool TubeComputerSystem::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeComputerSystem::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == clockPin && data_bytes == sizeof(double)) {
        clockSignal = voltageToLogic(*reinterpret_cast<double*>(data));
        if (running && clockSignal) {
            updateSystem();
        }
        return true;
    } else if (conn_id == resetPin && data_bytes == sizeof(double)) {
        resetSignal = voltageToLogic(*reinterpret_cast<double*>(data));
        if (resetSignal) {
            reset();
        }
        return true;
    } else if (conn_id == startPin && data_bytes == sizeof(double)) {
        bool startSignal = voltageToLogic(*reinterpret_cast<double*>(data));
        if (startSignal) {
            start();
        }
        return true;
    } else if (conn_id == stopPin && data_bytes == sizeof(double)) {
        bool stopSignal = voltageToLogic(*reinterpret_cast<double*>(data));
        if (stopSignal) {
            stop();
        }
        return true;
    }
    return false;
}

bool TubeComputerSystem::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    // For now, just return the program counter
    if (conn_id == 0 && data_bytes == sizeof(double)) {
        double pc = static_cast<double>(programCounter->getCount());
        memcpy(data, &pc, sizeof(double));
        return true;
    }
    return false;
}

bool TubeComputerSystem::Tick() {
    if (running) {
        updateSystem();
    }
    return true;
}

void TubeComputerSystem::reset() {
    running = false;
    stopped = true;
    programCounter->reset();
    programCounter->setCount(0);
    accumulator->clear();
    resetSignal = false;
}

void TubeComputerSystem::start() {
    running = true;
    stopped = false;
}

void TubeComputerSystem::stop() {
    running = false;
    stopped = true;
}

void TubeComputerSystem::loadProgram(const std::vector<std::vector<bool>>& program) {
    programMemory = program;
}

void TubeComputerSystem::fetchInstruction() {
    // Get current PC value
    int pc = programCounter->getCount();
    
    // Get instruction from memory
    std::vector<bool> instruction(32, false);  // 32-bit instruction
    if (pc < static_cast<int>(programMemory.size())) {
        instruction = programMemory[pc];
    }
    
    // Load instruction into instruction register
    instructionRegister->load(instruction);
    
    // Increment program counter
    programCounter->clock();
}

void TubeComputerSystem::decodeInstruction() {
    std::vector<bool> instr = instructionRegister->getValue();
    
    // Simple decoding for demonstration
    // In a real system, this would be more complex
    Instruction decoded = parseInstruction(instr);
}

void TubeComputerSystem::executeInstruction() {
    // Execute based on decoded instruction
    // This would implement the actual instruction execution
}

void TubeComputerSystem::updateSystem() {
    fetchInstruction();
    decodeInstruction();
    executeInstruction();
}

TubeComputerSystem::Instruction TubeComputerSystem::parseInstruction(const std::vector<bool>& instruction) {
    Instruction instr;
    
    // Simple instruction format: [OPCODE][OPERAND]
    // 8-bit opcode, 24-bit operand for 32-bit instructions
    instr.opCode = 0;
    for (int i = 0; i < 8; i++) {
        if (instruction[i]) instr.opCode |= (1 << i);
    }
    
    instr.operand1 = 0;
    for (int i = 8; i < 16; i++) {
        if (instruction[i]) instr.operand1 |= (1 << (i-8));
    }
    
    instr.operand2 = 0;
    for (int i = 16; i < 24; i++) {
        if (instruction[i]) instr.operand2 |= (1 << (i-16));
    }
    
    instr.address = 0;
    for (int i = 24; i < 32; i++) {
        if (instruction[i]) instr.address |= (1 << (i-24));
    }
    
    return instr;
}


// TubeSequencer implementation
TubeSequencer::TubeSequencer(int numSteps) : numSteps(numSteps) {
    if (numSteps < 1) numSteps = 1;
    if (numSteps > 32) numSteps = 32;
    
    initializeSequencer();
}

void TubeSequencer::initializeSequencer() {
    counter = std::make_unique<TubeCounter>(5);  // 5-bit counter for up to 32 steps
    
    // Set up step output pins
    for (int i = 0; i < std::min(numSteps, 16); i++) {  // Limit to 16 for now
        stepPins[i] = i;
    }
    
    clockPin = 16;
    startPin = 17;
    stopPin = 18;
    resetPin = 19;
}

bool TubeSequencer::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeSequencer::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == clockPin && data_bytes == sizeof(double)) {
        bool clockSignal = voltageToLogic(*reinterpret_cast<double*>(data));
        if (autoSequence && running && clockSignal) {
            counter->clock();
            currentStep = counter->getCount();
        }
        return true;
    } else if (conn_id == startPin && data_bytes == sizeof(double)) {
        bool startSignal = voltageToLogic(*reinterpret_cast<double*>(data));
        if (startSignal) {
            start();
        }
        return true;
    } else if (conn_id == stopPin && data_bytes == sizeof(double)) {
        bool stopSignal = voltageToLogic(*reinterpret_cast<double*>(data));
        if (stopSignal) {
            stop();
        }
        return true;
    } else if (conn_id == resetPin && data_bytes == sizeof(double)) {
        bool resetSignal = voltageToLogic(*reinterpret_cast<double*>(data));
        if (resetSignal) {
            reset();
        }
        return true;
    }
    return false;
}

bool TubeSequencer::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    // Return current step
    if (conn_id == 0 && data_bytes == sizeof(double)) {
        double step = static_cast<double>(currentStep);
        memcpy(data, &step, sizeof(double));
        return true;
    }
    return false;
}

bool TubeSequencer::Tick() {
    // Update outputs based on current step
    return true;
}

void TubeSequencer::setStep(int step) {
    if (step >= 0 && step < stepCount) {
        currentStep = step;
        // In a real implementation, this would set the counter to the specific step
    }
}

void TubeSequencer::start() {
    running = true;
}

void TubeSequencer::stop() {
    running = false;
}

void TubeSequencer::reset() {
    counter->reset();
    currentStep = 0;
}