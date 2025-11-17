#ifndef TUBE_LOGIC_GATES_H
#define TUBE_LOGIC_GATES_H

#include "Common.h"
#include "TubeComponents.h"
#include <vector>
#include <memory>

// Base class for tube-based logic gates
class TubeLogicGate : public ElectricNodeBase {
public:
    TubeLogicGate();
    virtual ~TubeLogicGate() = default;
    
    virtual bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    /*virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;*/
    virtual bool Tick() override;
    
    // Set logic voltage levels
    void setHighLevel(double volts) { highLevel = volts; }
    void setLowLevel(double volts) { lowLevel = volts; }
    double getHighLevel() const { return highLevel; }
    double getLowLevel() const { return lowLevel; }
    
    // Set tube parameters
    void setTubeType(const std::string& type) { tubeType = type; }
    std::string getTubeType() const { return tubeType; }
    
    // Set logic threshold (voltage level that determines 0/1)
    void setThreshold(double threshold) { logicThreshold = threshold; }
    double getThreshold() const { return logicThreshold; }

protected:
    // Logic voltage levels
    double highLevel = 5.0;      // Voltage representing logic HIGH
    double lowLevel = 0.0;       // Voltage representing logic LOW
    double logicThreshold = 2.5; // Threshold between HIGH and LOW
    
    // Tube parameters
    std::string tubeType = "6SN7";  // Common choice for digital applications
    
    // Internal state
    std::vector<double> inputStates;
    double outputState = 0.0;
    
    // Pin configuration - to be implemented by derived classes
    std::vector<int> inputPins;
    int outputPin = 0;
    int bPlusPin = 1;            // High voltage supply
    int groundPin = 2;           // Ground reference
    
    // Convert analog voltage to logic value (0 or 1)
    int voltageToLogic(double voltage) const {
        return voltage > logicThreshold ? 1 : 0;
    }
    
    // Convert logic value to analog voltage
    double logicToVoltage(int logic) const {
        return logic ? highLevel : lowLevel;
    }
    
    // Process the logical operation - to be implemented by derived classes
    virtual int performLogicOperation() = 0;
    
    // Update output based on inputs
    void updateOutput();
};

// NOT gate using tube
class TubeNOTGate : public TubeLogicGate {
public:
    TubeNOTGate();
    virtual ~TubeNOTGate() = default;
    
    // Pin configuration: single input
    void setInput(int pin) { inputPins = {pin}; }

protected:
    virtual int performLogicOperation() override;
};

// OR gate using tubes
class TubeORGate : public TubeLogicGate {
public:
    TubeORGate(int inputs = 2);
    virtual ~TubeORGate() = default;
    
    // Configure number of inputs
    void setInputs(int numInputs);

protected:
    virtual int performLogicOperation() override;
};

// AND gate using tubes
class TubeANDGate : public TubeLogicGate {
public:
    TubeANDGate(int inputs = 2);
    virtual ~TubeANDGate() = default;
    
    // Configure number of inputs
    void setInputs(int numInputs);

protected:
    virtual int performLogicOperation() override;
};

// NAND gate using tubes
class TubeNANDGate : public TubeLogicGate {
public:
    TubeNANDGate(int inputs = 2);
    virtual ~TubeNANDGate() = default;
    
    // Configure number of inputs
    void setInputs(int numInputs);

protected:
    virtual int performLogicOperation() override;
};

// NOR gate using tubes
class TubeNORGate : public TubeLogicGate {
public:
    TubeNORGate(int inputs = 2);
    virtual ~TubeNORGate() = default;
    
    // Configure number of inputs
    void setInputs(int numInputs);

protected:
    virtual int performLogicOperation() override;
};

// Exclusive OR (XOR) gate using tubes
class TubeXORGate : public TubeLogicGate {
public:
    TubeXORGate();
    virtual ~TubeXORGate() = default;

protected:
    virtual int performLogicOperation() override;
};

// Class to represent a complete tube-based logic family (like DTL or RTL equivalents)
class TubeLogicFamily {
public:
    // Initialize basic gates for the family
    static std::unique_ptr<TubeNOTGate> createNOT();
    static std::unique_ptr<TubeORGate> createOR(int inputs = 2);
    static std::unique_ptr<TubeANDGate> createAND(int inputs = 2);
    static std::unique_ptr<TubeNANDGate> createNAND(int inputs = 2);
    static std::unique_ptr<TubeNORGate> createNOR(int inputs = 2);
    static std::unique_ptr<TubeXORGate> createXOR();
    
    // Set standard voltage levels for the family
    static void setStandardVoltageLevels(double low = 0.0, double high = 5.0, double threshold = 2.5);
    
private:
    static double standardLowLevel;
    static double standardHighLevel;
    static double standardThreshold;
};

// Complex gate combinations
class TubeHalfAdder : public ElectricNodeBase {
public:
    TubeHalfAdder();
    virtual ~TubeHalfAdder() = default;
    
    virtual bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    /*virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;*/
    virtual bool Tick() override;
    
    // Set the inputs
    void setInputA(double value) { inputA = value; }
    void setInputB(double value) { inputB = value; }
    
    // Get the outputs
    double getSum() const { return sumOutput; }
    double getCarry() const { return carryOutput; }

private:
    // Internal components
    std::unique_ptr<TubeXORGate> xorGate;
    std::unique_ptr<TubeANDGate> andGate;
    
    // Inputs and outputs
    double inputA = 0.0;
    double inputB = 0.0;
    double sumOutput = 0.0;
    double carryOutput = 0.0;
    
    // Pin assignments
    int inputAPin = 0;
    int inputBPin = 1;
    int sumPin = 2;
    int carryPin = 3;
    int bPlusPin = 4;
    int groundPin = 5;
    
    void process();
};

class TubeFullAdder : public ElectricNodeBase {
public:
    TubeFullAdder();
    virtual ~TubeFullAdder() = default;
    
    virtual bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    /*virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;*/
    virtual bool Tick() override;
    
    // Set the inputs
    void setInputA(double value) { inputA = value; }
    void setInputB(double value) { inputB = value; }
    void setInputCin(double value) { inputCin = value; }
    
    // Get the outputs
    double getSum() const { return sumOutput; }
    double getCarryOut() const { return carryOutput; }

private:
    // Internal components (using half adders)
    std::unique_ptr<TubeHalfAdder> ha1;
    std::unique_ptr<TubeHalfAdder> ha2;
    std::unique_ptr<TubeORGate> orGate;
    
    // Inputs and outputs
    double inputA = 0.0;
    double inputB = 0.0;
    double inputCin = 0.0;
    double sumOutput = 0.0;
    double carryOutput = 0.0;
    
    // Pin assignments
    int inputAPin = 0;
    int inputBPin = 1;
    int inputCinPin = 2;
    int sumPin = 3;
    int carryOutPin = 4;
    int bPlusPin = 5;
    int groundPin = 6;
    
    void process();
};

#endif // TUBE_LOGIC_GATES_H