#ifndef TUBE_COMPUTER_SYSTEMS_H
#define TUBE_COMPUTER_SYSTEMS_H

#include "Common.h"
#include "TubeLogicGates.h"
#include "TubeFlipFlops.h"
#include "TubeCountersRegisters.h"
#include <vector>
#include <memory>

// Class to represent a tube-based arithmetic logic unit (ALU)
class TubeALU : public ElectricNodeBase {
public:
    TubeALU(int width = 8);
    virtual ~TubeALU() = default;
    
    virtual bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    /*virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;*/
    virtual bool Tick() override;
    
    // Set inputs
    void setOperandA(const std::vector<bool>& value);
    void setOperandB(const std::vector<bool>& value);
    void setOperation(int op);  // 0=ADD, 1=SUB, 2=AND, 3=OR, 4=XOR, etc.
    
    // Get results
    std::vector<bool> getResult() const { return result; }
    bool getCarryOut() const { return carryOut; }
    bool getZeroFlag() const { return zeroFlag; }
    bool getNegativeFlag() const { return negativeFlag; }
    
    int getWidth() const { return width; }

private:
    int width;
    std::vector<bool> operandA;
    std::vector<bool> operandB;
    std::vector<bool> result;
    int operation = 0;  // Operation code: 0=ADD, 1=SUB, etc.
    
    bool carryOut = false;
    bool zeroFlag = false;
    bool negativeFlag = false;
    
    // Components needed for ALU operation
    std::vector<std::unique_ptr<TubeFullAdder>> adders;
    std::vector<std::unique_ptr<TubeXORGate>> xorGates;
    std::vector<std::unique_ptr<TubeANDGate>> andGates;
    std::vector<std::unique_ptr<TubeORGate>> orGates;
    
    // Pin assignments
    std::vector<int> inputAPins;
    std::vector<int> inputBPins;
    int operationPin = 0;
    int carryInPin = 1;
    std::vector<int> resultPins;
    int carryOutPin = 2;
    int zeroFlagPin = 3;
    int negativeFlagPin = 4;
    int clockPin = 5;
    
    void initializeALU();
    void performOperation();
    void updateFlags();
};

// Class to represent a tube-based memory unit
class TubeMemory : public ElectricNodeBase {
public:
    TubeMemory(int addrWidth = 8, int dataWidth = 8); // 256 bytes by default
    virtual ~TubeMemory() = default;
    
    virtual bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    /*virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;*/
    virtual bool Tick() override;
    
    // Memory operations
    void setAddress(const std::vector<bool>& addr);
    void setWriteData(const std::vector<bool>& data);
    void setWriteEnable(bool enable) { writeEnable = enable; }
    void setReadEnable(bool enable) { readEnable = enable; }
    
    // Get read data
    std::vector<bool> getReadData() const { return readData; }
    
    int getAddressWidth() const { return addrWidth; }
    int getDataWidth() const { return dataWidth; }
    int getMemorySize() const { return 1 << addrWidth; }

private:
    int addrWidth;
    int dataWidth;
    int memSize;
    
    std::vector<std::vector<bool>> memoryArray;
    std::vector<bool> address;
    std::vector<bool> writeData;
    std::vector<bool> readData;
    
    bool writeEnable = false;
    bool readEnable = true;
    
    // Address decoder
    std::vector<std::unique_ptr<TubeANDGate>> decoderGates;
    std::vector<std::unique_ptr<TubeRegister>> storageRegisters;
    
    // Pin assignments
    std::vector<int> addrPins;
    std::vector<int> writeDataPins;
    std::vector<int> readDataPins;
    int writeEnablePin = 0;
    int readEnablePin = 1;
    int clockPin = 2;
    int chipEnablePin = 3;
    
    void initializeMemory();
    void accessMemory();
};

// Class to represent a tube-based accumulator register (common in early computers)
class TubeAccumulator : public ElectricNodeBase {
public:
    TubeAccumulator(int width = 8);
    virtual ~TubeAccumulator() = default;
    
    virtual bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    /*virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;*/
    virtual bool Tick() override;
    
    // Operations
    void load(const std::vector<bool>& data);
    void clear();
    std::vector<bool> getValue() const { return currentValue; }
    void increment();
    void shiftLeft();
    void shiftRight();
    
    // Flag outputs
    bool getZeroFlag() const { return zeroFlag; }
    bool getCarryFlag() const { return carryFlag; }
    
    int getWidth() const { return width; }

private:
    int width;
    std::vector<bool> currentValue;
    std::unique_ptr<TubeRegister> registerBank;
    std::unique_ptr<TubeALU> alu;
    
    bool zeroFlag = true;
    bool carryFlag = false;
    
    // Pin assignments
    std::vector<int> inputPins;
    std::vector<int> outputPins;
    int loadPin = 0;
    int clearPin = 1;
    int clockPin = 2;
    int incrementPin = 3;
    int shiftLeftPin = 4;
    int shiftRightPin = 5;
    
    void initializeAccumulator();
    void updateFlags();
};

// Class representing a complete early computer system (like a simplified ENIAC or Colossus)
class TubeComputerSystem : public ElectricNodeBase {
public:
    enum ComputerType {
        ENIAC,
        COLOSSUS,
        MANCHESTER_SLOW,
        EDSAC
    };
    
    TubeComputerSystem(ComputerType type = ENIAC);
    virtual ~TubeComputerSystem() = default;
    
    virtual bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    /*virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;*/
    virtual bool Tick() override;
    
    // System operations
    void reset();
    void start();
    void stop();
    void loadProgram(const std::vector<std::vector<bool>>& program);
    
    // Get system components
    TubeALU* getALU() { return alu.get(); }
    TubeMemory* getMemory() { return memory.get(); }
    TubeAccumulator* getAccumulator() { return accumulator.get(); }
    
    // Get system state
    int getProgramCounter() const { return currentProgramCounter; }
    bool isRunning() const { return running; }

private:
    ComputerType computerType;
    
    // Main system components
    std::unique_ptr<TubeALU> alu;
    std::unique_ptr<TubeMemory> memory;
    std::unique_ptr<TubeAccumulator> accumulator;
    
    // Control unit components
    std::unique_ptr<TubeCounter> pcCounter;
    std::unique_ptr<TubeRegister> instructionRegister;
    std::unique_ptr<TubeCounter> stepCounter;
    
    // System state
    int currentProgramCounter = 0;
    std::vector<std::vector<bool>> programMemory;
    bool running = false;
    bool stopped = true;
    
    // Control signals
    bool clockSignal = false;
    bool resetSignal = false;
    
    // Pin assignments
    int clockPin = 0;
    int resetPin = 1;
    int startPin = 2;
    int stopPin = 3;
    int interruptPin = 4;
    
    void initializeSystem(ComputerType type);
    void fetchInstruction();
    void decodeInstruction();
    void executeInstruction();
    void updateSystem();
    
    // Specific computer implementations
    void initializeENIAC();
    void initializeColossus();
    void initializeManchesterSlow();
    void initializeEDSAC();
    
    // Instruction format for different computers
    struct Instruction {
        int opCode;
        int operand1;
        int operand2;
        int address;
    };
    
    Instruction parseInstruction(const std::vector<bool>& instruction);
};

// Class representing a tube-based sequencer for early computers
class TubeSequencer : public ElectricNodeBase {
public:
    TubeSequencer(int numSteps = 16);
    virtual ~TubeSequencer() = default;
    
    virtual bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    /*virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;*/
    virtual bool Tick() override;
    
    void setStep(int step);
    void setStepCount(int count) { stepCount = count; }
    int getCurrentStep() const { return currentStep; }
    
    // Control
    void start();
    void stop();
    void reset();
    void setAuto(bool autoStep) { autoSequence = autoStep; }
    
private:
    int numSteps;
    int currentStep = 0;
    int stepCount = 16;
    bool running = false;
    bool autoSequence = false;
    
    std::unique_ptr<TubeCounter> counter;
    
    // Pin assignments
    int clockPin = 0;
    int startPin = 1;
    int stopPin = 2;
    int resetPin = 3;
    int stepPins[16];  // Output pins for each step
    
    void initializeSequencer();
};

#endif // TUBE_COMPUTER_SYSTEMS_H