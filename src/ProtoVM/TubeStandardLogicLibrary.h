#ifndef TUBE_STANDARD_LOGIC_LIBRARY_H
#define TUBE_STANDARD_LOGIC_LIBRARY_H

#include "Common.h"
#include "TubeLogicGates.h"
#include "TubeFlipFlops.h"
#include "TubeCountersRegisters.h"
#include "TubeMuxDemux.h"
#include <vector>
#include <memory>

// Standard library for tube-based logic components
namespace TubeLogicLibrary {

    // Enum for different logic families/types
    enum LogicFamily {
        RTL,        // Resistor-Transistor Logic (adapted for tubes)
        DTL,        // Diode-Transistor Logic (adapted for tubes)
        TTL,        // Transistor-Transistor Logic (adapted for tubes)
        CMOS,       // Complementary MOS (adapted for dual triode tubes)
        TUBE_NPN,   // N-P-N tube equivalent
        TUBE_PNP    // P-N-P tube equivalent (using inverted configurations)
    };

    // Standard logic ICs modeled with tubes
    namespace StandardICs {
        // 7400 series equivalents using tubes
        class IC7400 : public ElectricNodeBase {  // Quad 2-input NAND gates
        public:
            IC7400();
            virtual ~IC7400() = default;
            
            virtual bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
            virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
            /*virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;*/
            virtual bool Tick() override;
            
        private:
            std::vector<std::unique_ptr<TubeNANDGate>> nandGates;  // 4 NAND gates
            
            // Pin assignments for 14-pin DIP package (simplified)
            std::vector<int> inputPinsA;  // Pins 1, 2, 4, 5
            std::vector<int> inputPinsB;  // Pins 13, 12, 10, 9
            std::vector<int> outputPins;  // Pins 3, 6, 8, 11
            int vccPin = 14;              // Vcc (power)
            int gndPin = 7;               // GND (ground)
            
            void initialize();
            void processLogic();
        };

        class IC7402 : public ElectricNodeBase {  // Quad 2-input NOR gates
        public:
            IC7402();
            virtual ~IC7402() = default;
            
            virtual bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
            virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
            /*virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;*/
            virtual bool Tick() override;
            
        private:
            std::vector<std::unique_ptr<TubeNORGate>> norGates;  // 4 NOR gates
            
            // Pin assignments for 14-pin DIP package (simplified)
            std::vector<int> inputPinsA;  // Pins 2, 3, 6, 7
            std::vector<int> inputPinsB;  // Pins 13, 12, 10, 9
            std::vector<int> outputPins;  // Pins 1, 4, 5, 11
            int vccPin = 14;              // Vcc (power)
            int gndPin = 8;               // GND (ground)
            
            void initialize();
            void processLogic();
        };

        class IC7404 : public ElectricNodeBase {  // Hex inverter (NOT gates)
        public:
            IC7404();
            virtual ~IC7404() = default;
            
            virtual bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
            virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
            /*virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;*/
            virtual bool Tick() override;
            
        private:
            std::vector<std::unique_ptr<TubeNOTGate>> notGates;  // 6 NOT gates
            
            // Pin assignments for 14-pin DIP package
            std::vector<int> inputPins;   // Pins 1, 2, 4, 5, 9, 10
            std::vector<int> outputPins;  // Pins 2, 3, 5, 6, 8, 9
            int vccPin = 14;              // Vcc (power)
            int gndPin = 7;               // GND (ground)
            
            void initialize();
            void processLogic();
        };

        class IC7408 : public ElectricNodeBase {  // Quad 2-input AND gates
        public:
            IC7408();
            virtual ~IC7408() = default;
            
            virtual bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
            virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
            /*virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;*/
            virtual bool Tick() override;
            
        private:
            std::vector<std::unique_ptr<TubeANDGate>> andGates;  // 4 AND gates
            
            // Pin assignments for 14-pin DIP package (simplified)
            std::vector<int> inputPinsA;  // Pins 1, 2, 4, 5
            std::vector<int> inputPinsB;  // Pins 13, 12, 10, 9
            std::vector<int> outputPins;  // Pins 3, 6, 8, 11
            int vccPin = 14;              // Vcc (power)
            int gndPin = 7;               // GND (ground)
            
            void initialize();
            void processLogic();
        };

        class IC7432 : public ElectricNodeBase {  // Quad 2-input OR gates
        public:
            IC7432();
            virtual ~IC7432() = default;
            
            virtual bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
            virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
            /*virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;*/
            virtual bool Tick() override;
            
        private:
            std::vector<std::unique_ptr<TubeORGate>> orGates;  // 4 OR gates
            
            // Pin assignments for 14-pin DIP package (simplified)
            std::vector<int> inputPinsA;  // Pins 1, 2, 4, 5
            std::vector<int> inputPinsB;  // Pins 13, 12, 10, 9
            std::vector<int> outputPins;  // Pins 3, 6, 8, 11
            int vccPin = 14;              // Vcc (power)
            int gndPin = 7;               // GND (ground)
            
            void initialize();
            void processLogic();
        };

        // Register ICs
        class IC7474 : public ElectricNodeBase {  // Dual D-type flip-flop with clear and preset
        public:
            IC7474();
            virtual ~IC7474() = default;
            
            virtual bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
            virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
            /*virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;*/
            virtual bool Tick() override;
            
        private:
            std::vector<std::unique_ptr<TubeDFlipFlop>> flipFlops;  // 2 D flip-flops
            std::unique_ptr<TubeClockDivider> clockScaler;  // For clock conditioning
            
            // Pin assignments for 14-pin DIP package (simplified)
            std::vector<int> dataPins;        // Pins 2, 12 (D inputs)
            std::vector<int> clockPins;       // Pins 3, 11 (clock inputs)
            std::vector<int> presetPins;      // Pins 4, 10 (preset inputs)
            std::vector<int> clearPins;       // Pins 1, 13 (clear inputs)
            std::vector<int> outputPins;      // Pins 5, 9 (Q outputs)
            std::vector<int> outputInvertedPins; // Pins 6, 8 (Qbar outputs)
            int vccPin = 14;                  // Vcc (power)
            int gndPin = 7;                   // GND (ground)
            
            void initialize();
            void processLogic();
        };

        // Counter ICs
        class IC7490 : public ElectricNodeBase {  // Decade counter
        public:
            IC7490();
            virtual ~IC7490() = default;
            
            virtual bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
            virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
            /*virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;*/
            virtual bool Tick() override;
            
        private:
            std::unique_ptr<TubeBCDCounter> bcdCounter;
            
            // Pin assignments for 14-pin DIP package (simplified)
            int inputA = 14;           // Input A
            int inputB = 1;            // Input B  
            int resetPins[2] = {2, 3}; // Reset inputs
            int setPins[2] = {6, 7};   // Set inputs
            std::vector<int> outputPins; // Outputs QA, QB, QC, QD
            int vccPin = 5;            // Vcc (power)
            int gndPin = 10;           // GND (ground)
            
            void initialize();
            void processLogic();
        };

        class IC74161 : public ElectricNodeBase {  // 4-bit synchronous binary counter
        public:
            IC74161();
            virtual ~IC74161() = default;
            
            virtual bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
            virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
            /*virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;*/
            virtual bool Tick() override;
            
        private:
            std::unique_ptr<TubeBinaryCounter> counter;
            
            // Pin assignments for 16-pin DIP package
            int clockPin = 2;           // Clock
            int resetPin = 1;           // Master Reset
            int enablePinP = 7;         // Enable Parallel
            int enablePinT = 10;        // Enable Trickle
            int loadPin = 9;            // Load
            std::vector<int> dataPins;  // A, B, C, D - pins 3, 4, 5, 6
            std::vector<int> outputPins; // QA, QB, QC, QD - pins 15, 1, 2, 6
            int carryOutPin = 15;       // Ripple Carry Output
            int vccPin = 16;            // Vcc (power)
            int gndPin = 8;             // GND (ground)
            
            void initialize();
            void processLogic();
        };

        // Multiplexer ICs
        class IC74151 : public ElectricNodeBase {  // 8-to-1 multiplexer
        public:
            IC74151();
            virtual ~IC74151() = default;
            
            virtual bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
            virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
            /*virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;*/
            virtual bool Tick() override;
            
        private:
            std::unique_ptr<TubeMux8To1> mux;
            
            // Pin assignments for 16-pin DIP package
            std::vector<int> inputPins;   // I0-I7 pins 4, 3, 2, 1, 15, 14, 13, 12
            std::vector<int> selectPins;  // A, B, C pins 11, 10, 9
            int strobePin = 7;            // Strobe (active low)
            int outputPin = 5;            // Y output
            int invertedOutputPin = 6;    // W output (inverted)
            int vccPin = 16;              // Vcc (power)
            int gndPin = 8;               // GND (ground)
            
            void initialize();
            void processLogic();
        };
    }

    // Utility functions for the library
    namespace Utils {
        // Function to create a specific logic gate using the optimal tube configuration
        std::unique_ptr<TubeLogicGate> createOptimizedGate(
            TubeLogicGate::GateType type, 
            LogicFamily family = TUBE_NPN, 
            int inputs = 2
        );
        
        // Function to create a register with specific characteristics
        std::unique_ptr<TubeRegister> createOptimizedRegister(
            int width, 
            LogicFamily family = TUBE_NPN
        );
        
        // Function to create a counter with specific characteristics
        std::unique_ptr<TubeCounter> createOptimizedCounter(
            int width, 
            LogicFamily family = TUBE_NPN
        );
        
        // Function to create a multiplexer with specific characteristics
        std::unique_ptr<TubeMultiplexer> createOptimizedMux(
            int dataBits, 
            int selectBits, 
            LogicFamily family = TUBE_NPN
        );
    }

    // System-level components
    namespace System {
        // Arithmetic Logic Unit using tube components
        class TubALU : public ElectricNodeBase {
        public:
            TubALU(int width = 8);
            virtual ~TubALU() = default;
            
            virtual bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
            virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
            /*virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;*/
            virtual bool Tick() override;
            
            enum Operation {
                ADD, SUB, AND, OR, XOR, NOT, SHIFT_LEFT, SHIFT_RIGHT
            };
            
            void setOperation(Operation op) { operation = op; }
            void setOperandA(const std::vector<bool>& operand) { operandA = operand; }
            void setOperandB(const std::vector<bool>& operand) { operandB = operand; }
            
            std::vector<bool> getResult() const { return result; }
            bool getCarryOut() const { return carryOut; }
            
        private:
            int width;
            Operation operation = ADD;
            std::vector<bool> operandA;
            std::vector<bool> operandB;
            std::vector<bool> result;
            bool carryOut = false;
            
            // Internal components
            std::vector<std::unique_ptr<TubeFullAdder>> adders;
            std::vector<std::unique_ptr<TubeANDGate>> andGates;
            std::vector<std::unique_ptr<TubeORGate>> orGates;
            std::vector<std::unique_ptr<TubeXORGate>> xorGates;
            
            // Pin assignments
            std::vector<int> inputAPins;
            std::vector<int> inputBPins;
            int operationPin = 0;
            std::vector<int> resultPins;
            int carryOutPin = 1;
            int clockPin = 2;
            
            void initialize();
            void performOperation();
        };

        // Memory system using tube components
        class TubeMemorySystem : public ElectricNodeBase {
        public:
            TubeMemorySystem(int addrBits = 8, int dataBits = 8, int banks = 1);
            virtual ~TubeMemorySystem() = default;
            
            virtual bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
            virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
            /*virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;*/
            virtual bool Tick() override;
            
            void setAddress(const std::vector<bool>& addr) { address = addr; }
            void setWriteData(const std::vector<bool>& data) { writeData = data; }
            void setWriteEnable(bool en) { writeEnable = en; }
            void setReadEnable(bool en) { readEnable = en; }
            
            std::vector<bool> getReadData() const { return readData; }
            
        private:
            int addrBits;
            int dataBits;
            int banks;
            
            std::vector<bool> address;
            std::vector<bool> writeData;
            std::vector<bool> readData;
            bool writeEnable = false;
            bool readEnable = true;
            
            // Internal components
            std::vector<std::unique_ptr<TubeDecoder>> addressDecoders;
            std::vector<std::unique_ptr<TubeMemory>> memoryBanks;
            
            void initialize();
            void performAccess();
        };
    }
}

#endif // TUBE_STANDARD_LOGIC_LIBRARY_H