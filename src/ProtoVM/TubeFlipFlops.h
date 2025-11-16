#ifndef TUBE_FLIP_FLOPS_H
#define TUBE_FLIP_FLOPS_H

#include "ElectricNodeBase.h"
#include "TubeLogicGates.h"
#include <memory>

// Base class for tube-based latches and flip-flops
class TubeLatchFlipFlop : public ElectricNodeBase {
public:
    TubeLatchFlipFlop();
    virtual ~TubeLatchFlipFlop() = default;
    
    virtual bool Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool Tick() override;
    
    // Set logic voltage levels
    void setHighLevel(double volts) { highLevel = volts; }
    void setLowLevel(double volts) { lowLevel = volts; }
    double getHighLevel() const { return highLevel; }
    double getLowLevel() const { return lowLevel; }
    
    // Get state
    bool getQ() const { return qState; }
    bool getQBar() const { return !qState; }

protected:
    // Logic voltage levels
    double highLevel = 5.0;      // Voltage representing logic HIGH
    double lowLevel = 0.0;       // Voltage representing logic LOW
    
    // Internal state
    bool qState = false;
    bool qBarState = true;
    
    // Pin configuration
    int clockPin = 0;            // Clock/enable pin
    int dataPin = 1;             // Data input
    int setPin = 2;              // Set input
    int resetPin = 3;            // Reset input
    int qPin = 4;                // Q output
    int qBarPin = 5;             // Q Bar output
    int bPlusPin = 6;            // High voltage supply
    int groundPin = 7;           // Ground reference
    
    // Convert analog voltage to logic value (0 or 1)
    bool voltageToLogic(double voltage) const {
        return voltage > (highLevel + lowLevel) / 2.0;
    }
    
    // Convert logic value to analog voltage
    double logicToVoltage(bool logic) const {
        return logic ? highLevel : lowLevel;
    }
    
    // Update outputs based on internal state
    void updateOutputs();
    
    // Process the latch/flip-flop operation
    virtual void processOperation() = 0;
};

// SR Latch using tubes
class TubeSRLatch : public TubeLatchFlipFlop {
public:
    TubeSRLatch();
    virtual ~TubeSRLatch() = default;

protected:
    virtual void processOperation() override;
    
    // Inputs
    bool sInput = false;
    bool rInput = false;
};

// D Latch using tubes
class TubeDLatch : public TubeLatchFlipFlop {
public:
    TubeDLatch();
    virtual ~TubeDLatch() = default;

protected:
    virtual void processOperation() override;
    
    // Inputs
    bool dInput = false;
    bool enableInput = false;
};

// D Flip-Flop using tubes (master-slave configuration)
class TubeDFlipFlop : public TubeLatchFlipFlop {
public:
    TubeDFlipFlop();
    virtual ~TubeDFlipFlop() = default;
    
    // Set clock edge sensitivity
    void setRisingEdgeTriggered(bool rising) { risingEdgeTriggered = rising; }
    bool isRisingEdgeTriggered() const { return risingEdgeTriggered; }

protected:
    virtual void processOperation() override;
    
    // Internal state for master and slave
    bool masterState = false;
    bool slaveState = false;
    
    // Previous clock state for edge detection
    bool previousClock = false;
    bool risingEdgeTriggered = true;
    
    // Inputs
    bool dInput = false;
    bool clockInput = false;
};

// JK Flip-Flop using tubes
class TubeJKFlipFlop : public TubeLatchFlipFlop {
public:
    TubeJKFlipFlop();
    virtual ~TubeJKFlipFlop() = default;

protected:
    virtual void processOperation() override;
    
    // Inputs
    bool jInput = false;
    bool kInput = false;
    bool clockInput = false;
    
    // Internal state
    bool previousClock = false;
    bool risingEdgeTriggered = true;
};

// T Flip-Flop using tubes
class TubeTFlipFlop : public TubeLatchFlipFlop {
public:
    TubeTFlipFlop();
    virtual ~TubeTFlipFlop() = default;

protected:
    virtual void processOperation() override;
    
    // Inputs
    bool tInput = false;
    bool clockInput = false;
    
    // Internal state
    bool previousClock = false;
    bool risingEdgeTriggered = true;
};

// Class for creating registers using flip-flops
class TubeRegister {
public:
    TubeRegister(int width = 8);
    ~TubeRegister() = default;
    
    // Set data input
    void setInput(const std::vector<bool>& data);
    void setInput(int bit, bool value);
    
    // Clock the register
    void clock(bool clockSignal);
    
    // Set/Reset all flip-flops
    void setAll();
    void resetAll();
    
    // Get the current value
    std::vector<bool> getValue() const;
    bool getValue(int bit) const;
    
    // Get access to individual flip-flops
    TubeDFlipFlop* getFlipFlop(int index) { return flipFlops[index].get(); }

private:
    int width;
    std::vector<std::unique_ptr<TubeDFlipFlop>> flipFlops;
    std::vector<bool> currentValue;
    
    void updateValue();
};

// Shift register using tube flip-flops
class TubeShiftRegister {
public:
    TubeShiftRegister(int width = 8);
    ~TubeShiftRegister() = default;
    
    // Shift operations
    void shiftLeft(bool serialInput = false);
    void shiftRight(bool serialInput = false);
    void clock(bool clockSignal);
    
    // Parallel load
    void load(const std::vector<bool>& data);
    void setSerialInput(bool input) { serialInputBit = input; }
    
    // Get values
    std::vector<bool> getValue() const { return currentValue; }
    bool getSerialOutput() const { return currentValue.empty() ? false : currentValue[currentValue.size()-1]; }
    bool getValue(int bit) const { return currentValue[bit]; }
    
    int getWidth() const { return width; }

private:
    int width;
    std::vector<std::unique_ptr<TubeDFlipFlop>> flipFlops;
    std::vector<bool> currentValue;
    bool serialInputBit = false;
    
    void updateValue();
};

#endif // TUBE_FLIP_FLOPS_H