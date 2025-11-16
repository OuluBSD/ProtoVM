#ifndef TUBE_ARITHMETIC_UNITS_H
#define TUBE_ARITHMETIC_UNITS_H

#include "ElectricNodeBase.h"
#include "TubeLogicGates.h"
#include "TubeFlipFlops.h"
#include <vector>
#include <memory>

// Base class for tube-based arithmetic units
class TubeArithmeticUnit : public ElectricNodeBase {
public:
    TubeArithmeticUnit(int width = 8);
    virtual ~TubeArithmeticUnit() = default;
    
    virtual bool Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool Tick() override;
    
    // Set inputs
    void setInputA(const std::vector<bool>& value);
    void setInputB(const std::vector<bool>& value);
    void setCarryIn(bool carry) { carryIn = carry; }
    void setOperation(int op) { operation = op; }  // 0=ADD, 1=SUB, 2=MUL, 3=DIV, etc.
    
    // Get results
    std::vector<bool> getResult() const { return result; }
    bool getCarryOut() const { return carryOut; }
    bool getOverflow() const { return overflow; }
    bool getZero() const { return zero; }
    bool getNegative() const { return negative; }
    
    int getWidth() const { return width; }
    int getOperation() const { return operation; }

protected:
    int width;
    std::vector<bool> inputA;
    std::vector<bool> inputB;
    std::vector<bool> result;
    
    bool carryIn = false;
    bool carryOut = false;
    bool overflow = false;
    bool zero = true;
    bool negative = false;
    int operation = 0;  // 0=ADD, 1=SUB, etc.
    
    // Pin assignments
    std::vector<int> inputAPins;
    std::vector<int> inputBPins;
    int carryInPin = 0;
    int operationPin = 1;
    std::vector<int> resultPins;
    int carryOutPin = 2;
    int overflowPin = 3;
    int zeroPin = 4;
    int negativePin = 5;
    int clockPin = 6;
    
    // Internal components
    std::vector<std::unique_ptr<TubeFullAdder>> adders;
    
    void initialize();
    void performOperation();
    void updateFlags();
};

// Tube-based adder (can function as subtractor with 2's complement)
class TubeAdder : public TubeArithmeticUnit {
public:
    TubeAdder(int width = 8);
    virtual ~TubeAdder() = default;
    
    // Perform addition operation
    void performAddition();
    
    // Perform subtraction using 2's complement
    void performSubtraction();
    
    // Perform increment operation
    void performIncrement();

protected:
    virtual void performOperation() override;
};

// Tube-based multiplier
class TubeMultiplier : public TubeArithmeticUnit {
public:
    TubeMultiplier(int width = 8);
    virtual ~TubeMultiplier() = default;
    
    // Perform multiplication
    void performMultiplication();

protected:
    virtual void performOperation() override;
    
    // Internal implementation of multiplication algorithm
    std::vector<bool> multiply(const std::vector<bool>& a, const std::vector<bool>& b);
};

// Tube-based divider
class TubeDivider : public TubeArithmeticUnit {
public:
    TubeDivider(int width = 8);
    virtual ~TubeDivider() = default;
    
    // Perform division
    void performDivision();

protected:
    virtual void performOperation() override;
    
    // Internal implementation of division algorithm
    std::pair<std::vector<bool>, std::vector<bool>> divide(const std::vector<bool>& dividend, const std::vector<bool>& divisor);
};

// Tube-based ALU (Arithmetic Logic Unit) 
class TubeALUExtended : public TubeArithmeticUnit {
public:
    enum Operation {
        ADD = 0,
        SUB = 1,
        AND = 2,
        OR = 3,
        XOR = 4,
        NOT = 5,
        SHIFT_LEFT = 6,
        SHIFT_RIGHT = 7,
        INC = 8,
        DEC = 9,
        COMPARE = 10
    };
    
    TubeALUExtended(int width = 8);
    virtual ~TubeALUExtended() = default;
    
    virtual void setOperation(int op) override;
    
    // Comparison result (for COMPARE operation)
    enum CompareResult { EQUAL = 0, GREATER = 1, LESS = 2 };
    CompareResult getCompareResult() const { return compareResult; }

protected:
    virtual void performOperation() override;
    
    // Additional operations beyond basic arithmetic
    void performAND();
    void performOR();
    void performXOR();
    void performNOT();
    void performShiftLeft();
    void performShiftRight();
    void performCompare();
    
    CompareResult compareResult = EQUAL;
};

// Class for a complete arithmetic processing unit
class TubeArithmeticProcessingUnit {
public:
    TubeArithmeticProcessingUnit(int width = 8);
    ~TubeArithmeticProcessingUnit() = default;
    
    // Set inputs
    void setOperandA(const std::vector<bool>& value);
    void setOperandB(const std::vector<bool>& value);
    void setOperation(TubeALUExtended::Operation op);
    
    // Execute operation
    void execute();
    
    // Get results
    std::vector<bool> getResult() const { return result; }
    bool getCarry() const { return carry; }
    bool getOverflow() const { return overflow; }
    bool getZero() const { return zero; }
    bool getNegative() const { return negative; }
    
    // Get the arithmetic unit
    TubeALUExtended* getALU() { return alu.get(); }
    
private:
    int width;
    std::unique_ptr<TubeALUExtended> alu;
    std::unique_ptr<TubeAdder> adder;
    std::unique_ptr<TubeMultiplier> multiplier;
    std::unique_ptr<TubeDivider> divider;
    
    std::vector<bool> result;
    bool carry = false;
    bool overflow = false;
    bool zero = true;
    bool negative = false;
    
    // Store inputs for different operations
    std::vector<bool> operandA;
    std::vector<bool> operandB;
    
    void performAddSub();
    void performLogic();
    void performMultiplication();
    void performDivision();
};

// Specialized class for BCD (Binary Coded Decimal) arithmetic
class TubeBCDArithmeticUnit : public ElectricNodeBase {
public:
    TubeBCDArithmeticUnit(int digits = 4);  // Number of BCD digits
    virtual ~TubeBCDArithmeticUnit() = default;
    
    virtual bool Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool Tick() override;
    
    // Set BCD inputs (each 4-bit group represents a decimal digit)
    void setInputA(const std::vector<bool>& bcdValue);
    void setInputB(const std::vector<bool>& bcdValue);
    
    // Get BCD result
    std::vector<bool> getResult() const { return result; }
    
    // Operations
    void setOperation(int op) { operation = op; }  // 0=ADD, 1=SUB
    void performOperation();
    
    int getDigits() const { return digits; }

private:
    int digits;
    int bits;  // 4 * digits
    std::vector<bool> inputA;  // BCD encoded
    std::vector<bool> inputB;  // BCD encoded
    std::vector<bool> result;  // BCD encoded
    
    bool carryIn = false;
    bool carryOut = false;
    int operation = 0;  // 0=ADD BCD, 1=SUB BCD
    
    // Components for BCD arithmetic
    std::vector<std::unique_ptr<TubeAdder>> binaryAdders;
    std::vector<std::unique_ptr<TubeAdder>> correctionAdders;
    
    // Pin assignments
    std::vector<int> inputAPins;
    std::vector<int> inputBPins;
    int carryInPin = 0;
    int operationPin = 1;
    std::vector<int> resultPins;
    int carryOutPin = 2;
    int clockPin = 3;
    
    void initialize();
    void performBCDAddition();
    void performBCDSubtraction();
    std::vector<bool> bcdCorrection(const std::vector<bool>& binaryResult);
};

#endif // TUBE_ARITHMETIC_UNITS_H