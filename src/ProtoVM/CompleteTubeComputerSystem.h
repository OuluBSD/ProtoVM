#ifndef COMPLETE_TUBE_COMPUTER_SYSTEM_H
#define COMPLETE_TUBE_COMPUTER_SYSTEM_H

#include "Common.h"
#include "TubeComputerSystems.h"
#include "TubeStandardLogicLibrary.h"
#include "TubeArithmeticUnits.h"
#include "TubeMuxDemux.h"
#include <vector>
#include <memory>

// Class representing a complete tube-based computer system
class TubeComputer : public ElectricNodeBase {
public:
    enum ComputerArchitecture {
        ENIAC_STYLE,      // Early general purpose computer
        COLOSSUS_STYLE,   // Codebreaking machine
        EDSAC_STYLE,      // Stored program computer
        MANCHESTER_STYLE, // Early stored program computer
        IBM_701_STYLE     // Commercial scientific computer
    };
    
    TubeComputer(ComputerArchitecture arch = EDSAC_STYLE);
    virtual ~TubeComputer() = default;
    
    virtual bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    /*virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;*/
    virtual bool Tick() override;
    
    // Power control
    void powerOn();
    void powerOff();
    bool isPowered() const { return powered; }
    
    // Reset and control
    void reset();
    void start();
    void stop();
    bool isRunning() const { return running; }
    
    // Program loading
    void loadProgram(const std::vector<std::vector<bool>>& program);
    void loadProgram(const std::string& filename); // Load from file
    
    // Memory access
    std::vector<bool> readMemory(int address) const;
    void writeMemory(int address, const std::vector<bool>& data);
    
    // CPU state access
    int getProgramCounter() const;
    std::vector<bool> getAccumulator() const;
    std::vector<bool> getInstructionRegister() const;
    
    // Computer architecture
    ComputerArchitecture getArchitecture() const { return architecture; }
    
    // Interrupt handling
    void triggerInterrupt(int interruptNum);
    void acknowledgeInterrupt();
    
    // I/O operations
    void setInput(int port, const std::vector<bool>& data);
    std::vector<bool> getOutput(int port) const;
    
    // Performance monitoring
    int getInstructionCount() const { return instructionCount; }
    double getClockSpeed() const { return clockSpeed; }
    void setClockSpeed(double speed) { clockSpeed = speed; }
    
    // Access to internal components
    TubeComputerSystem* getSystem() { return system.get(); }
    TubeArithmeticProcessingUnit* getArithmeticUnit() { return alu.get(); }
    TubeMemory* getMemory() { return memory.get(); }

private:
    ComputerArchitecture architecture;
    bool powered = false;
    bool running = false;
    int instructionCount = 0;
    double clockSpeed = 1000.0;  // Default 1kHz
    
    // Main system components
    std::unique_ptr<TubeComputerSystem> system;
    std::unique_ptr<TubeArithmeticProcessingUnit> alu;
    std::unique_ptr<TubeMemory> memory;
    std::unique_ptr<TubeRegister> programCounter;
    std::unique_ptr<TubeRegister> instructionRegister;
    std::unique_ptr<TubeRegister> addressRegister;
    std::unique_ptr<TubeClockSystem> clockSystem;
    
    // Control unit
    std::unique_ptr<TubeRegister> controlRegister;
    std::unique_ptr<TubeDecoder> controlDecoder;
    
    // Memory
    std::vector<std::vector<bool>> mainMemory;
    int memorySize;
    int addressWidth;
    int dataWidth;
    
    // I/O ports
    std::vector<std::vector<bool>> inputPorts;
    std::vector<std::vector<bool>> outputPorts;
    
    // Interrupt system
    std::vector<bool> interruptLines;
    bool interruptsEnabled = false;
    
    // Pin assignments
    int powerPin = 0;
    int resetPin = 1;
    int startPin = 2;
    int clockPin = 3;
    int interruptPin = 4;
    
    // Initialize the computer based on architecture
    void initializeComputer();
    
    // Execute a single instruction cycle
    void executeInstructionCycle();
    
    // Fetch-decode-execute cycle
    void fetchInstruction();
    void decodeInstruction();
    void executeInstruction();
    void updateProgramCounter();
    
    // Handle specific architectures
    void initializeENIAC();
    void initializeColossus();
    void initializeEDSAC();
    void initializeManchester();
    void initializeIBM701();
    
    // Instruction set
    struct Instruction {
        int opCode;
        int address;
        int operand;
    };
    
    Instruction parseInstruction(const std::vector<bool>& instruction);
    
    // Execute specific instructions
    void executeLOAD(const Instruction& instr);
    void executeSTORE(const Instruction& instr);
    void executeADD(const Instruction& instr);
    void executeSUB(const Instruction& instr);
    void executeJUMP(const Instruction& instr);
    void executeJZ(const Instruction& instr);  // Jump if zero
    void executeJNZ(const Instruction& instr); // Jump if not zero
    void executeHALT(const Instruction& instr);
    void executeNOP(const Instruction& instr);
    void executeINPUT(const Instruction& instr);
    void executeOUTPUT(const Instruction& instr);
    
    // Memory management
    void initializeMemory();
    void accessMemory(int address, const std::vector<bool>* writeData = nullptr, std::vector<bool>* readData = nullptr);
    
    // System state
    void updateSystemState();
    
    // Clock and timing
    void updateClock();
};

// Specialized class for simulating the ENIAC computer
class ENIACSimulator : public TubeComputer {
public:
    ENIACSimulator();
    virtual ~ENIACSimulator() = default;
    
    // ENIAC-specific features
    void configurePanel(int panelId, const std::vector<int>& functions);
    void connectTray(int trayA, int trayB, int connectionType);
    
    // Panel operations
    void setPanelSwitch(int panelId, int switchId, bool state);
    bool getPanelIndicator(int panelId, int indicatorId) const;
    
private:
    // ENIAC had 40 panels with different functions
    std::vector<std::vector<bool>> panelSwitches;
    std::vector<std::vector<bool>> panelIndicators;
    std::vector<std::vector<int>> trayConnections;
    
    void initializeENIACSpecifics();
    void updateENIACPanels();
};

// Specialized class for simulating the Colossus computer
class ColossusSimulator : public TubeComputer {
public:
    ColossusSimulator();
    virtual ~ColossusSimulator() = default;
    
    // Colossus-specific features
    void setPaperTape(const std::string& tapeData);
    void setDictionary(const std::vector<std::string>& dict);
    void startBreaking();
    
    // Get results of codebreaking
    std::vector<std::string> getBreakResults() const { return breakResults; }
    int getConfidenceScore() const { return confidenceScore; }

private:
    std::string paperTapeData;
    std::vector<std::string> dictionary;
    std::vector<std::string> breakResults;
    int confidenceScore = 0;
    
    // Colossus had optical readers and shift registers
    std::unique_ptr<TubeShiftRegister> opticalReader;
    std::unique_ptr<TubeShiftRegister> comparisonRegisters[5];
    
    void initializeColossusSpecifics();
    void runColossusAlgorithm();
};

// Specialized class for simulating the EDSAC computer
class EDSACSimulator : public TubeComputer {
public:
    EDSACSimulator();
    virtual ~EDSACSimulator() = default;
    
    // EDSAC-specific features
    void loadInitialOrders(const std::vector<std::vector<bool>>& orders);
    void punchTape(const std::vector<std::vector<bool>>& data);
    
    // Get tape reader state
    bool isTapeReading() const { return tapeReading; }
    bool isTapeReady() const { return tapeReady; }

private:
    bool tapeReading = false;
    bool tapeReady = false;
    
    // EDSAC had mercury delay lines for memory
    std::vector<std::vector<bool>> delayLineMemory;
    std::unique_ptr<TubeRegister> delayLineAddressRegister;
    
    void initializeEDSACSpecifics();
    void handleTapeOperations();
};

// Class for managing tube failures and maintenance
class TubeFailureManager {
public:
    TubeFailureManager(TubeComputer* comp);
    
    // Simulate tube aging and failure
    void simulateTubeAging(double timeElapsed);
    void forceTubeFailure(int tubeId);
    void repairTube(int tubeId);
    void performMaintenance();
    
    // Get statistics
    int getFailedTubesCount() const { return failedTubes.size(); }
    std::vector<int> getFailedTubes() const { return failedTubes; }
    double getSystemReliability() const { return reliability; }

private:
    TubeComputer* computer;
    std::vector<int> failedTubes;
    std::vector<double> tubeAgeHours;  // Hours of operation
    std::vector<bool> tubeHealty;      // Health status
    double reliability = 1.0;
    
    // Mean time to failure parameters
    double lambda = 0.001;  // Failure rate (failures per hour)
    
    // Calculate the probability of failure based on age
    bool calculateFailure(double ageHours, double failureRate);
};

// Class for simulating the complete computing installation
class TubeComputingInstallation {
public:
    enum InstallationType {
        UNIV_LAB,        // University lab setup
        COMM_CENTER,     // Commercial computing center
        MIL_INSTALLATION // Military installation
    };
    
    TubeComputingInstallation(InstallationType type = UNIV_LAB);
    ~TubeComputingInstallation();
    
    // Add computers to installation
    void addComputer(std::unique_ptr<TubeComputer> computer);
    
    // Operator interactions
    void operatorLogin(const std::string& operatorName);
    void operatorLogout();
    void submitJob(const std::vector<std::vector<bool>>& job);
    void viewJobQueue();
    
    // Environmental controls (temperature, power, etc.)
    void setTemperature(double temp) { roomTemperature = temp; }
    void setHumidity(double humidity) { roomHumidity = humidity; }
    void setPowerStability(double stability) { powerStability = stability; }
    
    // Get environmental readings
    double getTemperature() const { return roomTemperature; }
    double getHumidity() const { return roomHumidity; }
    double getPowerStability() const { return powerStability; }
    
    // Perform maintenance on all systems
    void performDailyMaintenance();
    void generateStatusReport();
    
private:
    InstallationType installationType;
    std::vector<std::unique_ptr<TubeComputer>> computers;
    std::vector<std::unique_ptr<TubeFailureManager>> failureManagers;
    
    // Operator and job management
    std::string currentOperator;
    std::vector<std::vector<std::vector<bool>>> jobQueue;
    
    // Environmental factors
    double roomTemperature = 22.0;  // Celsius
    double roomHumidity = 45.0;    // Percent
    double powerStability = 1.0;   // 1.0 = perfect, 0.0 = no power
    
    // Staff and maintenance scheduling
    std::vector<std::string> scheduledMaintenances;
    
    void initializeInstallation();
    void updateEnvironmentalFactors();
    void handleOperatorRequests();
    void manageJobs();
};

#endif // COMPLETE_TUBE_COMPUTER_SYSTEM_H