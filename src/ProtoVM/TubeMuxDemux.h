#ifndef TUBE_MUX_DEMUX_H
#define TUBE_MUX_DEMUX_H

#include "ElectricNodeBase.h"
#include "TubeLogicGates.h"
#include "TubeFlipFlops.h"
#include <vector>
#include <memory>

// Base class for tube-based multiplexers
class TubeMultiplexer : public ElectricNodeBase {
public:
    TubeMultiplexer(int dataBits, int selectBits);
    virtual ~TubeMultiplexer() = default;
    
    virtual bool Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool Tick() override;
    
    // Set data inputs
    void setDataInput(int channel, bool value);
    void setDataInputs(const std::vector<bool>& values);
    
    // Set select inputs
    void setSelectInput(int bit, bool value);
    void setSelectInputs(const std::vector<bool>& values);
    
    // Get outputs
    bool getOutput() const { return output; }
    std::vector<bool> getOutputs() const { return {output}; }  // Mux typically has single output
    
    int getDataBits() const { return dataBits; }
    int getSelectBits() const { return selectBits; }
    int getChannelCount() const { return channelCount; }

protected:
    int dataBits;        // Width of each data input
    int selectBits;      // Width of selector input
    int channelCount;    // 2^selectBits channels
    
    // Data inputs (one per channel)
    std::vector<std::vector<bool>> dataInputs;
    
    // Select inputs
    std::vector<bool> selectInputs;
    
    // Output
    bool output = false;
    
    // Components for mux implementation
    std::vector<std::unique_ptr<TubeANDGate>> andGates;
    std::unique_ptr<TubeORGate> orGate;
    
    // Pin assignments
    std::vector<std::vector<int>> dataInputPins;  // [channel][bit]
    std::vector<int> selectPins;
    int outputPin = 0;
    int enablePin = 1;
    
    bool enabled = true;
    
    void initializeMultiplexer();
    void evaluateOutput();
};

// Class for tube-based demultiplexer
class TubeDemultiplexer : public ElectricNodeBase {
public:
    TubeDemultiplexer(int dataBits, int selectBits);
    virtual ~TubeDemultiplexer() = default;
    
    virtual bool Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool Tick() override;
    
    // Set data input (single input to be routed to outputs)
    void setDataInput(bool value);
    void setDataInput(const std::vector<bool>& value);
    
    // Set select inputs
    void setSelectInput(int bit, bool value);
    void setSelectInputs(const std::vector<bool>& values);
    
    // Set enable signal
    void setEnable(bool en) { enabled = en; }
    
    // Get output for specific channel
    bool getOutput(int channel) const;
    std::vector<bool> getOutputs() const { return outputs[0]; }  // Simplified for single bit data
    
    int getDataBits() const { return dataBits; }
    int getSelectBits() const { return selectBits; }
    int getChannelCount() const { return channelCount; }

protected:
    int dataBits;        // Width of data input/output
    int selectBits;      // Width of selector input
    int channelCount;    // 2^selectBits channels
    
    // Data input
    std::vector<bool> inputData;
    
    // Select inputs
    std::vector<bool> selectInputs;
    
    // Outputs (one per channel)
    std::vector<std::vector<bool>> outputs;
    
    // Components for demux implementation
    std::vector<std::unique_ptr<TubeANDGate>> andGates;  // Decoder + data routing
    
    // Pin assignments
    std::vector<int> dataInputPins;
    std::vector<int> selectPins;
    std::vector<std::vector<int>> outputPins;  // [channel][bit]
    int enablePin = 0;
    
    bool enabled = true;
    
    void initializeDemultiplexer();
    void evaluateOutputs();
};

// 2-to-1 Multiplexer using tubes (most basic implementation)
class TubeMux2To1 : public TubeMultiplexer {
public:
    TubeMux2To1();
    virtual ~TubeMux2To1() = default;
    
    void setA(bool value) { setDataInput(0, value); }
    void setB(bool value) { setDataInput(1, value); }
    void setSelect(bool sel) { setSelectInput(0, sel); }
};

// 4-to-1 Multiplexer using tubes
class TubeMux4To1 : public TubeMultiplexer {
public:
    TubeMux4To1();
    virtual ~TubeMux4To1() = default;
    
    void setA(bool value) { setDataInput(0, value); }
    void setB(bool value) { setDataInput(1, value); }
    void setC(bool value) { setDataInput(2, value); }
    void setD(bool value) { setDataInput(3, value); }
    void setSelect(const std::vector<bool>& sel) { setSelectInputs(sel); }
};

// 8-to-1 Multiplexer using tubes
class TubeMux8To1 : public TubeMultiplexer {
public:
    TubeMux8To1();
    virtual ~TubeMux8To1() = default;
    
    void setA(bool value) { setDataInput(0, value); }
    void setB(bool value) { setDataInput(1, value); }
    void setC(bool value) { setDataInput(2, value); }
    void setD(bool value) { setDataInput(3, value); }
    void setE(bool value) { setDataInput(4, value); }
    void setF(bool value) { setDataInput(5, value); }
    void setG(bool value) { setDataInput(6, value); }
    void setH(bool value) { setDataInput(7, value); }
    void setSelect(const std::vector<bool>& sel) { setSelectInputs(sel); }
};

// 1-to-2 Demultiplexer using tubes
class TubeDemux1To2 : public TubeDemultiplexer {
public:
    TubeDemux1To2();
    virtual ~TubeDemux1To2() = default;
    
    void setDataInput(bool value);
    void setSelect(bool sel);
    
    bool getOutputA() const { return getOutput(0); }
    bool getOutputB() const { return getOutput(1); }
};

// 1-to-4 Demultiplexer using tubes
class TubeDemux1To4 : public TubeDemultiplexer {
public:
    TubeDemux1To4();
    virtual ~TubeDemux1To4() = default;
    
    void setDataInput(bool value);
    void setSelect(const std::vector<bool>& sel);
    
    bool getOutputA() const { return getOutput(0); }
    bool getOutputB() const { return getOutput(1); }
    bool getOutputC() const { return getOutput(2); }
    bool getOutputD() const { return getOutput(3); }
};

// 1-to-8 Demultiplexer using tubes
class TubeDemux1To8 : public TubeDemultiplexer {
public:
    TubeDemux1To8();
    virtual ~TubeDemux1To8() = default;
    
    void setDataInput(bool value);
    void setSelect(const std::vector<bool>& sel);
    
    bool getOutputA() const { return getOutput(0); }
    bool getOutputB() const { return getOutput(1); }
    bool getOutputC() const { return getOutput(2); }
    bool getOutputD() const { return getOutput(3); }
    bool getOutputE() const { return getOutput(4); }
    bool getOutputF() const { return getOutput(5); }
    bool getOutputG() const { return getOutput(6); }
    bool getOutputH() const { return getOutput(7); }
};

// Specialized class for tube-based decoder (like 3-to-8 decoder)
class TubeDecoder : public ElectricNodeBase {
public:
    TubeDecoder(int inputBits, int outputBits);  // e.g., 3-to-8, 2-to-4
    virtual ~TubeDecoder() = default;
    
    virtual bool Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool Tick() override;
    
    // Set input
    void setInput(int bit, bool value);
    void setInputs(const std::vector<bool>& values);
    
    // Get outputs
    bool getOutput(int outputNum) const { return outputs[outputNum]; }
    std::vector<bool> getOutputs() const { return outputs; }
    
    // Enable/disable decoder
    void setEnable(bool en) { enabled = en; }
    bool isEnabled() const { return enabled; }
    
    int getInputBits() const { return inputBits; }
    int getOutputBits() const { return outputBits; }

private:
    int inputBits;
    int outputBits;  // Usually 2^inputBits
    bool enabled = true;
    
    std::vector<bool> inputs;
    std::vector<bool> outputs;
    
    // Component for implementation
    std::vector<std::unique_ptr<TubeANDGate>> andGates;
    std::vector<std::unique_ptr<TubeNOTGate>> notGates;
    
    // Pin assignments
    std::vector<int> inputPins;
    std::vector<int> outputPins;
    int enablePin = 0;
    
    void initializeDecoder();
    void evaluateOutputs();
};

// Example: 3-to-8 decoder (used in many demux circuits)
class TubeDecoder3To8 : public TubeDecoder {
public:
    TubeDecoder3To8();
    virtual ~TubeDecoder3To8() = default;
    
    void setInputs(bool a, bool b, bool c);
    bool getOutput0() const { return getOutput(0); }
    bool getOutput1() const { return getOutput(1); }
    bool getOutput2() const { return getOutput(2); }
    bool getOutput3() const { return getOutput(3); }
    bool getOutput4() const { return getOutput(4); }
    bool getOutput5() const { return getOutput(5); }
    bool getOutput6() const { return getOutput(6); }
    bool getOutput7() const { return getOutput(7); }
};

// Class for creating a tube-based analog multiplexer using tube characteristics
class TubeAnalogMultiplexer : public ElectricNodeBase {
public:
    TubeAnalogMultiplexer(int channelCount, int sampleRate = 44100);
    virtual ~TubeAnalogMultiplexer() = default;
    
    virtual bool Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool Tick() override;
    
    // Set analog inputs (continuous values)
    void setAnalogInput(int channel, double value);
    
    // Set select inputs (digital)
    void setSelectInput(int bit, bool value);
    void setSelectInputs(const std::vector<bool>& values);
    
    // Get output
    double getOutput() const { return output; }
    
    int getChannelCount() const { return channelCount; }
    int getSelectBits() const { return selectBits; }

private:
    int channelCount;
    int selectBits;  // Number of select bits = ceil(log2(channelCount))
    int sampleRate;
    
    // Analog inputs
    std::vector<double> analogInputs;
    
    // Digital select inputs
    std::vector<bool> selectInputs;
    
    // Output
    double output = 0.0;
    
    // Selected channel
    int selectedChannel = 0;
    
    // Pin assignments
    std::vector<int> analogInputPins;
    std::vector<int> selectPins;
    int outputPin = 0;
    int enablePin = 1;
    
    bool enabled = true;
    
    void initialize();
    void updateSelection();
    void updateOutput();
};

#endif // TUBE_MUX_DEMUX_H