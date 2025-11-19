#ifndef TUBE_COUNTERS_REGISTERS_H
#define TUBE_COUNTERS_REGISTERS_H

#include "Common.h"
#include "TubeFlipFlops.h"
#include <vector>
#include <memory>

// Base class for tube-based counters
class TubeCounter : public ElectricNodeBase {
public:
    TubeCounter(int width = 4);
    virtual ~TubeCounter() = default;
    
    virtual bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    /*virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;*/
    virtual bool Tick() override;
    
    // Counter operations
    void reset();
    void set(bool load = false, const std::vector<bool>& value = std::vector<bool>());
    
    // Get the current count
    int getCount() const;
    std::vector<bool> getBinaryValue() const;
    
    // Enable/disable counting
    void enableCounting(bool enable) { countingEnabled = enable; }
    bool isCountingEnabled() const { return countingEnabled; }
    
    // Configure counter type
    void setCountDirection(bool up) { countUp = up; }
    bool isCountingUp() const { return countUp; }
    
    // Set max count (for modulo counters)
    void setMaxCount(int max) { maxCount = max; }
    int getMaxCount() const { return maxCount; }
    
    // Clock the counter
    virtual void clock();

protected:
    int width;
    std::vector<bool> currentValue;
    std::vector<std::unique_ptr<TubeDFlipFlop>> flipFlops;
    
    // Counter configuration
    bool countingEnabled = true;
    bool countUp = true;
    int maxCount = -1;  // -1 means no maximum (binary counter)
    
    // Pin assignments
    int clockPin = 0;
    int resetPin = 1;
    int enablePin = 2;
    int loadPin = 3;
    int upDownPin = 4;  // For up/down selection
    
    // Internal state
    bool clockPrev = false;
    bool resetPrev = false;
    bool enablePrev = false;
    bool loadPrev = false;
    
    // Initialize the counter
    void initialize();
    
    // Process the counter operation
    virtual void processOperation();
};

// Binary counter using tubes
class TubeBinaryCounter : public TubeCounter {
public:
    TubeBinaryCounter(int width = 4);
    virtual ~TubeBinaryCounter() = default;
    
    virtual void clock() override;
    
protected:
    virtual void processOperation() override;
};

// BCD Counter (Binary Coded Decimal) using tubes
class TubeBCDCounter : public TubeCounter {
public:
    TubeBCDCounter(int width = 4);  // Width should be multiple of 4 for BCD
    virtual ~TubeBCDCounter() = default;
    
    virtual void clock() override;
    
protected:
    virtual void processOperation() override;
};

// Ring counter using tubes
class TubeRingCounter : public TubeCounter {
public:
    TubeRingCounter(int width = 4);
    virtual ~TubeRingCounter() = default;
    
    virtual void clock() override;
    
    // Initialize to a specific pattern
    void initializePattern(const std::vector<bool>& pattern = std::vector<bool>());
    
protected:
    virtual void processOperation() override;
    std::vector<bool> initialPattern;
};

// Johnson counter (Twisted ring) using tubes
class TubeJohnsonCounter : public TubeCounter {
public:
    TubeJohnsonCounter(int width = 4);
    virtual ~TubeJohnsonCounter() = default;
    
    virtual void clock() override;
    
    void initializePattern(const std::vector<bool>& pattern = std::vector<bool>());
    
protected:
    virtual void processOperation() override;
    std::vector<bool> initialPattern;
};

// Specialized register types
class TubeBufferRegister : public ElectricNodeBase {
public:
    TubeBufferRegister(int width = 8);
    virtual ~TubeBufferRegister() = default;
    
    virtual bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    /*virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;*/
    virtual bool Tick() override;
    
    // Load data into the register
    void load(const std::vector<bool>& data);
    
    // Enable/disable output
    void setOutputEnable(bool enabled) { outputEnabled = enabled; }
    
    // Get the stored value
    std::vector<bool> getValue() const { return currentValue; }
    bool getValue(int bit) const { return currentValue[bit]; }
    
    int getWidth() const { return width; }

private:
    int width;
    std::vector<bool> currentValue;
    std::vector<std::unique_ptr<TubeDFlipFlop>> flipFlops;
    bool outputEnabled = true;
    
    // Pin assignments
    std::vector<int> inputPins;  // Each bit has an input pin
    std::vector<int> outputPins; // Each bit has an output pin
    int clockPin = 0;
    int loadPin = 1;      // Load/enable pin
    int outputEnablePin = 2;  // Output enable
    
    void initialize();
    void updateOutputs();
};

class TubeUniversalShiftRegister : public ElectricNodeBase {
public:
    enum ShiftMode {
        NOP,      // No operation
        RIGHT,    // Shift right
        LEFT,     // Shift left
        LOAD      // Parallel load
    };
    
    TubeUniversalShiftRegister(int width = 4);
    virtual ~TubeUniversalShiftRegister() = default;
    
    virtual bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    /*virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;*/
    virtual bool Tick() override;
    
    // Set shift mode
    void setMode(ShiftMode mode) { shiftMode = mode; }
    ShiftMode getMode() const { return shiftMode; }
    
    // Serial inputs
    void setSerialInputLeft(bool value) { serialInputLeft = value; }
    void setSerialInputRight(bool value) { serialInputRight = value; }
    
    // Parallel data
    void setParallelData(const std::vector<bool>& data);
    
    // Clock the register
    void clock();
    
    // Get values
    std::vector<bool> getValue() const { return currentValue; }
    bool getValue(int bit) const { return currentValue[bit]; }
    bool getSerialOutputLeft() const { return currentValue.empty() ? false : currentValue[0]; }
    bool getSerialOutputRight() const { return currentValue.empty() ? false : currentValue[currentValue.size()-1]; }
    
    int getWidth() const { return width; }

private:
    int width;
    std::vector<bool> currentValue;
    std::vector<std::unique_ptr<TubeDFlipFlop>> flipFlops;
    
    // Control signals
    ShiftMode shiftMode = NOP;
    bool serialInputLeft = false;
    bool serialInputRight = false;
    std::vector<bool> parallelData;
    
    // Pin assignments
    int clockPin = 0;
    int modePin0 = 1;   // Mode select pins
    int modePin1 = 2;
    int dataRightPin = 3;   // Serial input from right
    int dataLeftPin = 4;    // Serial input from left
    std::vector<int> parallelInputPins;  // Parallel data input pins
    
    // Internal state
    bool clockPrev = false;
    
    void initialize();
    void processOperation();
};

// Counter with specific functionality (like for clock generation)
class TubeClockDivider : public TubeCounter {
public:
    TubeClockDivider(int divideBy = 2);
    virtual ~TubeClockDivider() = default;
    
    // Get the divided clock output
    bool getDividedClock() const { return dividedClock; }
    
    // Set the division factor
    void setDivisionFactor(int factor) { 
        if (factor > 0) {
            divideBy = factor;
            maxCount = factor - 1;  // Count from 0 to factor-1
        }
    }
    int getDivisionFactor() const { return divideBy; }

protected:
    int divideBy = 2;
    bool dividedClock = false;
    
    virtual void processOperation() override;
    virtual void clock() override;
};

#endif // TUBE_COUNTERS_REGISTERS_H