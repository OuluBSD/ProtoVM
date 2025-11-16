#include "CompleteTubeComputerSystem.h"
#include <fstream>
#include <sstream>

// TubeComputer implementation
TubeComputer::TubeComputer(ComputerArchitecture arch) : architecture(arch) {
    initializeComputer();
}

void TubeComputer::initializeComputer() {
    // Initialize computer based on selected architecture
    switch (architecture) {
        case ENIAC_STYLE:
            initializeENIAC();
            break;
        case COLOSSUS_STYLE:
            initializeColossus();
            break;
        case EDSAC_STYLE:
            initializeEDSAC();
            break;
        case MANCHESTER_STYLE:
            initializeManchester();
            break;
        case IBM_701_STYLE:
            initializeIBM701();
            break;
    }
    
    // Initialize memory
    initializeMemory();
    
    // Create main system components
    system = std::make_unique<TubeComputerSystem>(static_cast<TubeComputerSystem::ComputerType>(architecture));
    alu = std::make_unique<TubeArithmeticProcessingUnit>(dataWidth);
    memory = std::make_unique<TubeMemory>(addressWidth, dataWidth);
    
    // Create registers
    programCounter = std::make_unique<TubeRegister>(addressWidth);
    instructionRegister = std::make_unique<TubeRegister>(dataWidth);
    addressRegister = std::make_unique<TubeRegister>(addressWidth);
    
    // Create clock system
    clockSystem = std::make_unique<TubeClockSystem>();
    clockSystem->setMasterFrequency(clockSpeed);
    
    // Create control components
    controlRegister = std::make_unique<TubeRegister>(8);  // 8-bit control register
    controlDecoder = std::make_unique<TubeDecoder>(3, 8);  // 3-to-8 decoder for control signals
}

bool TubeComputer::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeComputer::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == powerPin && data_bytes == sizeof(double)) {
        bool power = voltageToLogic(*reinterpret_cast<double*>(data));
        if (power) {
            powerOn();
        } else {
            powerOff();
        }
        return true;
    } else if (conn_id == resetPin && data_bytes == sizeof(double)) {
        bool reset = voltageToLogic(*reinterpret_cast<double*>(data));
        if (reset) {
            reset();
        }
        return true;
    } else if (conn_id == startPin && data_bytes == sizeof(double)) {
        bool start = voltageToLogic(*reinterpret_cast<double*>(data));
        if (start) {
            start();
        } else {
            stop();
        }
        return true;
    }
    
    // Handle memory access if powered
    if (powered) {
        // In a real implementation, this would connect to specific memory addresses
        // For this simulation, we'll handle it differently
    }
    
    return false;
}

bool TubeComputer::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    // Handle state requests
    // For this simulation, return a simple status value
    if (conn_id == 0 && data_bytes == sizeof(double)) {  // Status
        double status = powered ? (running ? 2.0 : 1.0) : 0.0;
        memcpy(data, &status, sizeof(double));
        return true;
    }
    return false;
}

bool TubeComputer::Tick() {
    if (powered && running) {
        executeInstructionCycle();
        instructionCount++;
        updateSystemState();
    }
    return true;
}

void TubeComputer::powerOn() {
    powered = true;
    reset();  // Initialize system state on power-on
    clockSystem->setEnable(true);
}

void TubeComputer::powerOff() {
    powered = false;
    running = false;
    clockSystem->setEnable(false);
}

void TubeComputer::reset() {
    if (!powered) return;
    
    // Reset system components
    programCounter->setInput(std::vector<bool>(addressWidth, false));
    instructionRegister->setInput(std::vector<bool>(dataWidth, false));
    
    // Reset memory
    for (auto& row : mainMemory) {
        std::fill(row.begin(), row.end(), false);
    }
    
    running = false;
    instructionCount = 0;
    interruptsEnabled = false;
    std::fill(interruptLines.begin(), interruptLines.end(), false);
}

void TubeComputer::start() {
    if (powered) {
        running = true;
    }
}

void TubeComputer::stop() {
    running = false;
}

void TubeComputer::executeInstructionCycle() {
    fetchInstruction();
    decodeInstruction();
    executeInstruction();
    updateProgramCounter();
}

void TubeComputer::fetchInstruction() {
    // Get current program counter value
    std::vector<bool> pc = programCounter->getValue();
    
    // Convert to integer address
    int addr = 0;
    for (int i = 0; i < addressWidth; i++) {
        if (pc[i]) {
            addr |= (1 << i);
        }
    }
    
    // Read instruction from memory
    std::vector<bool> instruction = readMemory(addr);
    instructionRegister->setInput(instruction);
}

void TubeComputer::decodeInstruction() {
    std::vector<bool> instr = instructionRegister->getValue();
    // Decode happens in executeInstruction
}

void TubeComputer::executeInstruction() {
    std::vector<bool> instr = instructionRegister->getValue();
    Instruction decoded = parseInstruction(instr);
    
    switch (decoded.opCode) {
        case 0:  // HALT
            executeHALT(decoded);
            break;
        case 1:  // LOAD
            executeLOAD(decoded);
            break;
        case 2:  // STORE
            executeSTORE(decoded);
            break;
        case 3:  // ADD
            executeADD(decoded);
            break;
        case 4:  // SUB
            executeSUB(decoded);
            break;
        case 5:  // JUMP
            executeJUMP(decoded);
            break;
        case 6:  // JZ
            executeJZ(decoded);
            break;
        case 7:  // JNZ
            executeJNZ(decoded);
            break;
        case 8:  // INPUT
            executeINPUT(decoded);
            break;
        case 9:  // OUTPUT
            executeOUTPUT(decoded);
            break;
        default:
            executeNOP(decoded);  // No operation for unknown opcodes
            break;
    }
}

void TubeComputer::updateProgramCounter() {
    // Increment program counter for next instruction
    std::vector<bool> pc = programCounter->getValue();
    
    // Increment
    bool carry = true;
    for (int i = 0; i < addressWidth && carry; i++) {
        bool oldBit = pc[i];
        pc[i] = !oldBit;  // Flip bit
        carry = oldBit && pc[i];  // Carry if flipped from 1 to 0
    }
    
    programCounter->setInput(pc);
}

void TubeComputer::loadProgram(const std::vector<std::vector<bool>>& program) {
    int maxAddr = std::min(static_cast<int>(program.size()), memorySize);
    for (int i = 0; i < maxAddr; i++) {
        writeMemory(i, program[i]);
    }
}

std::vector<bool> TubeComputer::readMemory(int address) const {
    if (address >= 0 && address < memorySize) {
        return mainMemory[address];
    }
    return std::vector<bool>(dataWidth, false);
}

void TubeComputer::writeMemory(int address, const std::vector<bool>& data) {
    if (address >= 0 && address < memorySize) {
        int bitsToWrite = std::min(static_cast<int>(data.size()), dataWidth);
        for (int i = 0; i < dataWidth; i++) {
            if (i < bitsToWrite) {
                mainMemory[address][i] = data[i];
            } else {
                mainMemory[address][i] = false;
            }
        }
    }
}

int TubeComputer::getProgramCounter() const {
    std::vector<bool> pc = programCounter->getValue();
    int addr = 0;
    for (int i = 0; i < addressWidth; i++) {
        if (pc[i]) addr |= (1 << i);
    }
    return addr;
}

std::vector<bool> TubeComputer::getAccumulator() const {
    // In our system, the ALU accumulator is not directly accessible
    // but we return a default value
    return std::vector<bool>(dataWidth, false);
}

std::vector<bool> TubeComputer::getInstructionRegister() const {
    return instructionRegister->getValue();
}

void TubeComputer::triggerInterrupt(int interruptNum) {
    if (interruptNum >= 0 && interruptNum < static_cast<int>(interruptLines.size())) {
        interruptLines[interruptNum] = true;
    }
}

TubeComputer::Instruction TubeComputer::parseInstruction(const std::vector<bool>& instruction) {
    Instruction instr;
    
    // Simple encoding: upper bits are opcode, lower bits are address/operand
    int opBits = 4;  // Use 4 bits for opcode
    instr.opCode = 0;
    for (int i = 0; i < opBits && i < dataWidth; i++) {
        if (instruction[dataWidth - 1 - i]) {
            instr.opCode |= (1 << (opBits - 1 - i));
        }
    }
    
    // Remaining bits for address/operand
    instr.address = 0;
    int addrBits = dataWidth - opBits;
    for (int i = 0; i < addrBits && i < dataWidth - opBits; i++) {
        int bitPos = addrBits - 1 - i;
        if (instruction[bitPos]) {
            instr.address |= (1 << i);
        }
    }
    
    instr.operand = instr.address;
    return instr;
}

void TubeComputer::initializeMemory() {
    // Set up memory configuration
    switch (architecture) {
        case ENIAC_STYLE:
            addressWidth = 10;  // 1024 addresses
            dataWidth = 10;     // 10 bits per word
            break;
        case COLOSSUS_STYLE:
            addressWidth = 8;   // 256 addresses
            dataWidth = 5;      // 5 bits for character processing
            break;
        case EDSAC_STYLE:
            addressWidth = 11;  // 2048 addresses
            dataWidth = 35;     // 35 bits per word
            break;
        case MANCHESTER_STYLE:
            addressWidth = 12;  // 4096 addresses
            dataWidth = 40;     // 40 bits per word
            break;
        case IBM_701_STYLE:
            addressWidth = 10;  // 1024 addresses
            dataWidth = 36;     // 36 bits per word
            break;
    }
    
    memorySize = 1 << addressWidth;
    mainMemory.resize(memorySize, std::vector<bool>(dataWidth, false));
    interruptLines.resize(16, false);  // 16 interrupt lines
    inputPorts.resize(8, std::vector<bool>(dataWidth, false));  // 8 input ports
    outputPorts.resize(8, std::vector<bool>(dataWidth, false)); // 8 output ports
}

void TubeComputer::executeLOAD(const Instruction& instr) {
    // Load from memory address to accumulator
    std::vector<bool> data = readMemory(instr.address);
    if (alu) {
        alu->setOperandA(data);
    }
}

void TubeComputer::executeSTORE(const Instruction& instr) {
    // Store accumulator to memory address
    std::vector<bool> data;
    if (alu) {
        data = alu->getResult();
    } else {
        // Default data
        data = std::vector<bool>(dataWidth, false);
    }
    writeMemory(instr.address, data);
}

void TubeComputer::executeADD(const Instruction& instr) {
    // Add memory address contents to accumulator
    std::vector<bool> data = readMemory(instr.address);
    if (alu) {
        alu->setOperandB(data);
        alu->setOperation(TubeALUExtended::ADD);
        alu->execute();
    }
}

void TubeComputer::executeSUB(const Instruction& instr) {
    // Subtract memory address contents from accumulator
    std::vector<bool> data = readMemory(instr.address);
    if (alu) {
        alu->setOperandB(data);
        alu->setOperation(TubeALUExtended::SUB);
        alu->execute();
    }
}

void TubeComputer::executeJUMP(const Instruction& instr) {
    // Jump to address
    std::vector<bool> newPC(dataWidth, false);
    for (int i = 0; i < addressWidth && i < dataWidth; i++) {
        if (instr.address & (1 << i)) {
            newPC[i] = true;
        }
    }
    programCounter->setInput(newPC);
}

void TubeComputer::executeJZ(const Instruction& instr) {
    // Jump if accumulator is zero
    if (alu) {
        if (alu->getZero()) {
            executeJUMP(instr);
        }
    }
}

void TubeComputer::executeJNZ(const Instruction& instr) {
    // Jump if accumulator is not zero
    if (alu) {
        if (!alu->getZero()) {
            executeJUMP(instr);
        }
    }
}

void TubeComputer::executeHALT(const Instruction& instr) {
    // Halt execution
    stop();
}

void TubeComputer::executeNOP(const Instruction& instr) {
    // No operation
    // Just increment program counter in updateProgramCounter()
}

void TubeComputer::executeINPUT(const Instruction& instr) {
    // Input from specified port to accumulator
    int port = instr.address % 8;
    if (alu) {
        alu->setOperandA(inputPorts[port]);
    }
}

void TubeComputer::executeOUTPUT(const Instruction& instr) {
    // Output accumulator to specified port
    int port = instr.address % 8;
    if (alu) {
        outputPorts[port] = alu->getResult();
    }
}

void TubeComputer::initializeENIAC() { }

void TubeComputer::initializeColossus() { }

void TubeComputer::initializeEDSAC() { }

void TubeComputer::initializeManchester() { }

void TubeComputer::initializeIBM701() { }

void TubeComputer::updateSystemState() {
    // Update the state of all components based on current clock
    if (system) {
        system->Tick();
    }
    if (alu) {
        alu->Tick();
    }
    if (memory) {
        memory->Tick();
    }
    if (clockSystem) {
        clockSystem->Tick();
    }
}


// ENIACSimulator implementation
ENIACSimulator::ENIACSimulator() : TubeComputer(ENIAC_STYLE) {
    initializeENIACSpecifics();
}

void ENIACSimulator::initializeENIACSpecifics() {
    // ENIAC had 40 programmable panels
    panelSwitches.resize(40, std::vector<bool>(20, false));  // 20 switches per panel
    panelIndicators.resize(40, std::vector<bool>(10, false));  // 10 indicators per panel
    trayConnections.resize(20);  // 20 trays with various connections
}

void ENIACSimulator::setPanelSwitch(int panelId, int switchId, bool state) {
    if (panelId >= 0 && panelId < 40 && switchId >= 0 && switchId < 20) {
        panelSwitches[panelId][switchId] = state;
    }
}

bool ENIACSimulator::getPanelIndicator(int panelId, int indicatorId) const {
    if (panelId >= 0 && panelId < 40 && indicatorId >= 0 && indicatorId < 10) {
        return panelIndicators[panelId][indicatorId];
    }
    return false;
}

void ENIACSimulator::updateENIACPanels() {
    // Update logic for ENIAC-style panels
    for (int i = 0; i < 40; i++) {
        // Update indicators based on switch positions and internal state
        for (int j = 0; j < 10; j++) {
            // This would update indicators based on real connections
            panelIndicators[i][j] = panelSwitches[i][j * 2];  // Simplified
        }
    }
}


// ColossusSimulator implementation
ColossusSimulator::ColossusSimulator() : TubeComputer(COLOSSUS_STYLE) {
    initializeColossusSpecifics();
}

void ColossusSimulator::initializeColossusSpecifics() {
    // Create shift registers for the optical reader and comparison
    opticalReader = std::make_unique<TubeShiftRegister>(2048);  // Paper tape could be long
    for (int i = 0; i < 5; i++) {
        comparisonRegisters[i] = std::make_unique<TubeShiftRegister>(2048);
    }
}

void ColossusSimulator::setPaperTape(const std::string& tapeData) {
    paperTapeData = tapeData;
    
    // Convert string to binary and load into shift register
    std::vector<bool> binaryTape;
    for (char c : tapeData) {
        for (int i = 0; i < 8; i++) {
            binaryTape.push_back((c >> i) & 1);
        }
    }
    
    opticalReader->load(binaryTape);
}

void ColossusSimulator::setDictionary(const std::vector<std::string>& dict) {
    dictionary = dict;
}

void ColossusSimulator::startBreaking() {
    runColossusAlgorithm();
}

void ColossusSimulator::runColossusAlgorithm() {
    // Simplified version of the Colossus algorithm
    // In reality, this would involve complex boolean logic and statistical analysis
    
    // Shift through the paper tape
    // Compare with dictionary entries using statistical methods
    // This is a placeholder for the actual cryptanalytic algorithms
    
    breakResults.push_back("PLACEHOLDER RESULT - IMPLEMENT ACTUAL CRYPTOGRAPHIC ALGORITHM");
    confidenceScore = 42; // Placeholder confidence score
}


// EDSACSimulator implementation
EDSACSimulator::EDSACSimulator() : TubeComputer(EDSAC_STYLE) {
    initializeEDSACSpecifics();
}

void EDSACSimulator::initializeEDSACSpecifics() {
    // EDSAC used mercury delay lines for memory
    delayLineMemory.resize(1024, std::vector<bool>(35, false));  // 1024 words of 35 bits
    delayLineAddressRegister = std::make_unique<TubeRegister>(10);  // 10-bit address
}

void EDSACSimulator::loadInitialOrders(const std::vector<std::vector<bool>>& orders) {
    // Load the initial orders (bootstrap program) into memory
    for (int i = 0; i < std::min(static_cast<int>(orders.size()), 32); i++) {  // First 32 locations
        writeMemory(i, orders[i]);
    }
}

void EDSACSimulator::punchTape(const std::vector<std::vector<bool>>& data) {
    // Simulate punch tape operation
    // This would create a punched tape output in the real system
}

bool EDSACSimulator::isTapeReading() const {
    return tapeReading;
}

bool EDSACSimulator::isTapeReady() const {
    return tapeReady;
}

void EDSACSimulator::handleTapeOperations() {
    // Handle tape reading and writing operations
    // This is a simplified representation
    tapeReady = true;
    tapeReading = false;
}


// TubeFailureManager implementation
TubeFailureManager::TubeFailureManager(TubeComputer* comp) : computer(comp) {
    if (computer) {
        // Initialize tube arrays based on the computer's size
        tubeAgeHours.resize(2000, 0.0);  // Estimate 2000 tubes in a large computer
        tubeHealty.resize(2000, true);
    }
}

void TubeFailureManager::simulateTubeAging(double timeElapsed) {
    for (int i = 0; i < tubeAgeHours.size(); i++) {
        if (tubeHealty[i] && !failedTubes.empty()) {
            tubeAgeHours[i] += timeElapsed;
            
            // Calculate failure probability based on age (Weibull distribution model)
            // In reality, this would be more complex
            if (calculateFailure(tubeAgeHours[i], lambda)) {
                tubeHealty[i] = false;
                failedTubes.push_back(i);
            }
        }
    }
    
    // Recalculate reliability
    int healthyCount = 0;
    for (bool healthy : tubeHealty) {
        if (healthy) healthyCount++;
    }
    reliability = static_cast<double>(healthyCount) / tubeHealty.size();
}

void TubeFailureManager::forceTubeFailure(int tubeId) {
    if (tubeId >= 0 && tubeId < tubeHealty.size()) {
        tubeHealty[tubeId] = false;
        failedTubes.push_back(tubeId);
    }
}

void TubeFailureManager::repairTube(int tubeId) {
    if (tubeId >= 0 && tubeId < tubeHealty.size()) {
        tubeHealty[tubeId] = true;
        // Remove from failed list
        failedTubes.erase(std::remove(failedTubes.begin(), failedTubes.end(), tubeId), failedTubes.end());
        tubeAgeHours[tubeId] = 0;  // Reset age when repaired
    }
}

void TubeFailureManager::performMaintenance() {
    // Cycle through all tubes and perform preventive maintenance
    for (int i = 0; i < tubeHealty.size(); i++) {
        if (!tubeHealty[i]) {
            // Attempt to repair failed tubes
            repairTube(i);
        }
    }
}

bool TubeFailureManager::calculateFailure(double ageHours, double failureRate) {
    // Simplified failure calculation using exponential model
    // In reality, this would use more complex aging models
    double prob = failureRate * ageHours;
    return (rand() / static_cast<double>(RAND_MAX)) < prob;
}

double TubeFailureManager::getSystemReliability() const {
    return reliability;
}


// TubeComputingInstallation implementation
TubeComputingInstallation::TubeComputingInstallation(InstallationType type) : installationType(type) {
    initializeInstallation();
}

void TubeComputingInstallation::initializeInstallation() {
    // Initialize based on installation type
    switch (installationType) {
        case UNIV_LAB:
            roomTemperature = 20.0;
            roomHumidity = 50.0;
            powerStability = 0.95;
            break;
        case COMM_CENTER:
            roomTemperature = 18.0;
            roomHumidity = 45.0;
            powerStability = 0.98;
            break;
        case MIL_INSTALLATION:
            roomTemperature = 25.0;
            roomHumidity = 40.0;
            powerStability = 0.92;
            break;
    }
}

void TubeComputingInstallation::addComputer(std::unique_ptr<TubeComputer> computer) {
    computers.push_back(std::move(computer));
    failureManagers.push_back(std::make_unique<TubeFailureManager>(computers.back().get()));
}

void TubeComputingInstallation::operatorLogin(const std::string& operatorName) {
    currentOperator = operatorName;
}

void TubeComputingInstallation::operatorLogout() {
    currentOperator = "";
}

void TubeComputingInstallation::submitJob(const std::vector<std::vector<bool>>& job) {
    jobQueue.push_back(job);
}

void TubeComputingInstallation::performDailyMaintenance() {
    // Perform maintenance on all computers
    for (auto& fm : failureManagers) {
        fm->performMaintenance();
    }
}

void TubeComputingInstallation::generateStatusReport() {
    std::cout << "=== Tube Computing Installation Status Report ===" << std::endl;
    std::cout << "Environment: Temp=" << roomTemperature << "C, Hum=" << roomHumidity << "%, Power=" << powerStability << std::endl;
    
    for (size_t i = 0; i < computers.size(); i++) {
        std::cout << "Computer " << i << ": ";
        if (computers[i]->isPowered()) {
            std::cout << "ON, ";
            if (computers[i]->isRunning()) {
                std::cout << "RUNNING (" << computers[i]->getInstructionCount() << " instructions)";
            } else {
                std::cout << "STOPPED";
            }
            
            if (failureManagers[i]) {
                std::cout << ", Reliability: " << failureManagers[i]->getSystemReliability();
            }
        } else {
            std::cout << "OFF";
        }
        std::cout << std::endl;
    }
    std::cout << "===============================================" << std::endl;
}