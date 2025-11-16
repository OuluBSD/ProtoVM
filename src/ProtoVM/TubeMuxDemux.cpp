#include "TubeMuxDemux.h"
#include <cmath>

// TubeMultiplexer implementation
TubeMultiplexer::TubeMultiplexer(int dataBits, int selectBits) 
    : dataBits(dataBits), selectBits(selectBits) {
    if (dataBits < 1) dataBits = 1;
    if (dataBits > 32) dataBits = 32;
    if (selectBits < 1) selectBits = 1;
    if (selectBits > 5) selectBits = 5;  // Max 32 channels (2^5)
    
    channelCount = 1 << selectBits;  // 2^selectBits channels
    
    initializeMultiplexer();
}

void TubeMultiplexer::initializeMultiplexer() {
    // Initialize data inputs
    dataInputs.resize(channelCount, std::vector<bool>(dataBits, false));
    
    // Initialize select inputs
    selectInputs.resize(selectBits, false);
    
    // Initialize pin assignments
    dataInputPins.resize(channelCount);
    for (int i = 0; i < channelCount; i++) {
        dataInputPins[i].resize(dataBits);
        for (int j = 0; j < dataBits; j++) {
            dataInputPins[i][j] = i * dataBits + j;
        }
    }
    
    selectPins.resize(selectBits);
    for (int i = 0; i < selectBits; i++) {
        selectPins[i] = channelCount * dataBits + i;
    }
    
    outputPin = channelCount * dataBits + selectBits;
    enablePin = channelCount * dataBits + selectBits + 1;
    
    // Create internal components for implementation
    // A real multiplexer would be implemented with AND/OR gates
    andGates.resize(channelCount);
    for (int i = 0; i < channelCount; i++) {
        andGates[i] = std::make_unique<TubeANDGate>(dataBits + selectBits);
    }
    orGate = std::make_unique<TubeORGate>(channelCount);
}

bool TubeMultiplexer::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeMultiplexer::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    // Handle data inputs
    for (int i = 0; i < channelCount; i++) {
        for (int j = 0; j < dataBits; j++) {
            if (conn_id == static_cast<uint16>(dataInputPins[i][j]) && data_bytes == sizeof(double)) {
                dataInputs[i][j] = voltageToLogic(*reinterpret_cast<double*>(data));
                return true;
            }
        }
    }
    
    // Handle select inputs
    for (int i = 0; i < selectBits; i++) {
        if (conn_id == static_cast<uint16>(selectPins[i]) && data_bytes == sizeof(double)) {
            selectInputs[i] = voltageToLogic(*reinterpret_cast<double*>(data));
            return true;
        }
    }
    
    if (conn_id == enablePin && data_bytes == sizeof(double)) {
        enabled = voltageToLogic(*reinterpret_cast<double*>(data));
        return true;
    }
    
    return false;
}

bool TubeMultiplexer::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == static_cast<uint16>(outputPin) && data_bytes == sizeof(double)) {
        double outputVolts = logicToVoltage(output);
        memcpy(data, &outputVolts, sizeof(double));
        return true;
    }
    return false;
}

bool TubeMultiplexer::Tick() {
    if (enabled) {
        evaluateOutput();
    } else {
        output = false;  // Disabled mux outputs 0
    }
    return true;
}

void TubeMultiplexer::setDataInput(int channel, bool value) {
    if (channel >= 0 && channel < channelCount) {
        dataInputs[channel][0] = value;
    }
}

void TubeMultiplexer::setDataInputs(const std::vector<bool>& values) {
    int bitsToSet = std::min(static_cast<int>(values.size()), channelCount);
    for (int i = 0; i < bitsToSet; i++) {
        setDataInput(i, values[i]);
    }
}

void TubeMultiplexer::setSelectInput(int bit, bool value) {
    if (bit >= 0 && bit < selectBits) {
        selectInputs[bit] = value;
    }
}

void TubeMultiplexer::setSelectInputs(const std::vector<bool>& values) {
    int bitsToSet = std::min(static_cast<int>(values.size()), selectBits);
    for (int i = 0; i < bitsToSet; i++) {
        setSelectInput(i, values[i]);
    }
}

void TubeMultiplexer::evaluateOutput() {
    // Determine which channel is selected
    int selectedChannel = 0;
    for (int i = 0; i < selectBits; i++) {
        if (selectInputs[i]) {
            selectedChannel |= (1 << i);
        }
    }
    
    // If selected channel is valid, output its value
    if (selectedChannel >= 0 && selectedChannel < channelCount) {
        output = dataInputs[selectedChannel][0];  // For 1-bit data
    } else {
        output = false;  // Invalid selection
    }
}


// TubeDemultiplexer implementation
TubeDemultiplexer::TubeDemultiplexer(int dataBits, int selectBits) 
    : dataBits(dataBits), selectBits(selectBits) {
    if (dataBits < 1) dataBits = 1;
    if (dataBits > 32) dataBits = 32;
    if (selectBits < 1) selectBits = 1;
    if (selectBits > 5) selectBits = 5;  // Max 32 channels
    
    channelCount = 1 << selectBits;  // 2^selectBits channels
    
    initializeDemultiplexer();
}

void TubeDemultiplexer::initializeDemultiplexer() {
    // Initialize data input
    inputData.resize(dataBits, false);
    
    // Initialize select inputs
    selectInputs.resize(selectBits, false);
    
    // Initialize outputs
    outputs.resize(channelCount, std::vector<bool>(dataBits, false));
    
    // Pin assignments
    dataInputPins.resize(dataBits);
    for (int i = 0; i < dataBits; i++) {
        dataInputPins[i] = i;
    }
    
    selectPins.resize(selectBits);
    for (int i = 0; i < selectBits; i++) {
        selectPins[i] = dataBits + i;
    }
    
    outputPins.resize(channelCount);
    for (int i = 0; i < channelCount; i++) {
        outputPins[i].resize(dataBits);
        for (int j = 0; j < dataBits; j++) {
            outputPins[i][j] = dataBits + selectBits + (i * dataBits) + j;
        }
    }
    
    enablePin = dataBits + selectBits + channelCount * dataBits;
    
    // Create components for implementation
    andGates.resize(channelCount);
    for (int i = 0; i < channelCount; i++) {
        // Each AND gate gets the input data + decoded select lines
        andGates[i] = std::make_unique<TubeANDGate>(dataBits + selectBits + 1);  // +1 for select decode
    }
}

bool TubeDemultiplexer::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeDemultiplexer::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    // Handle data input
    for (int i = 0; i < dataBits; i++) {
        if (conn_id == static_cast<uint16>(dataInputPins[i]) && data_bytes == sizeof(double)) {
            inputData[i] = voltageToLogic(*reinterpret_cast<double*>(data));
            return true;
        }
    }
    
    // Handle select inputs
    for (int i = 0; i < selectBits; i++) {
        if (conn_id == static_cast<uint16>(selectPins[i]) && data_bytes == sizeof(double)) {
            selectInputs[i] = voltageToLogic(*reinterpret_cast<double*>(data));
            return true;
        }
    }
    
    if (conn_id == enablePin && data_bytes == sizeof(double)) {
        enabled = voltageToLogic(*reinterpret_cast<double*>(data));
        return true;
    }
    
    return false;
}

bool TubeDemultiplexer::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    // Handle output pins
    for (int i = 0; i < channelCount; i++) {
        for (int j = 0; j < dataBits; j++) {
            if (conn_id == static_cast<uint16>(outputPins[i][j]) && data_bytes == sizeof(double)) {
                double voltage = logicToVoltage(outputs[i][j]);
                memcpy(data, &voltage, sizeof(double));
                return true;
            }
        }
    }
    return false;
}

bool TubeDemultiplexer::Tick() {
    if (enabled) {
        evaluateOutputs();
    } else {
        // When disabled, all outputs go to 0
        for (int i = 0; i < channelCount; i++) {
            for (int j = 0; j < dataBits; j++) {
                outputs[i][j] = false;
            }
        }
    }
    return true;
}

bool TubeDemultiplexer::getOutput(int channel) const {
    if (channel >= 0 && channel < channelCount) {
        return outputs[channel][0];  // For 1-bit data
    }
    return false;
}

void TubeDemultiplexer::setDataInput(bool value) {
    inputData[0] = value;
}

void TubeDemultiplexer::setDataInput(const std::vector<bool>& value) {
    int bitsToSet = std::min(static_cast<int>(value.size()), dataBits);
    for (int i = 0; i < bitsToSet; i++) {
        inputData[i] = value[i];
    }
}

void TubeDemultiplexer::setSelectInput(int bit, bool value) {
    if (bit >= 0 && bit < selectBits) {
        selectInputs[bit] = value;
    }
}

void TubeDemultiplexer::setSelectInputs(const std::vector<bool>& values) {
    int bitsToSet = std::min(static_cast<int>(values.size()), selectBits);
    for (int i = 0; i < bitsToSet; i++) {
        setSelectInput(i, values[i]);
    }
}

void TubeDemultiplexer::evaluateOutputs() {
    // Determine which output channel is selected
    int selectedChannel = 0;
    for (int i = 0; i < selectBits; i++) {
        if (selectInputs[i]) {
            selectedChannel |= (1 << i);
        }
    }
    
    // Set up outputs: only the selected channel gets the input value
    for (int i = 0; i < channelCount; i++) {
        if (i == selectedChannel) {
            for (int j = 0; j < dataBits; j++) {
                outputs[i][j] = inputData[j];
            }
        } else {
            for (int j = 0; j < dataBits; j++) {
                outputs[i][j] = false;  // Non-selected channels get 0
            }
        }
    }
}


// Specific multiplexer implementations
TubeMux2To1::TubeMux2To1() : TubeMultiplexer(1, 1) { }  // 2 channels, 1 select bit

TubeMux4To1::TubeMux4To1() : TubeMultiplexer(1, 2) { }  // 4 channels, 2 select bits

TubeMux8To1::TubeMux8To1() : TubeMultiplexer(1, 3) { }  // 8 channels, 3 select bits


// Specific demultiplexer implementations
TubeDemux1To2::TubeDemux1To2() : TubeDemultiplexer(1, 1) { }  // 1 input, 2 outputs, 1 select bit

TubeDemux1To4::TubeDemux1To4() : TubeDemultiplexer(1, 2) { }  // 1 input, 4 outputs, 2 select bits

TubeDemux1To8::TubeDemux1To8() : TubeDemultiplexer(1, 3) { }  // 1 input, 8 outputs, 3 select bits


// TubeDecoder implementation
TubeDecoder::TubeDecoder(int inputBits, int outputBits) 
    : inputBits(inputBits), outputBits(outputBits) {
    if (inputBits < 1) inputBits = 1;
    if (inputBits > 5) inputBits = 5;  // Max 32 outputs (2^5)
    
    // Output bits should be 2^inputBits, but allow flexibility for special decoders
    if (outputBits == 0) {
        outputBits = 1 << inputBits;
    }
    
    initializeDecoder();
}

void TubeDecoder::initializeDecoder() {
    // Initialize inputs and outputs
    inputs.resize(inputBits, false);
    outputs.resize(outputBits, false);
    
    // Pin assignments
    inputPins.resize(inputBits);
    outputPins.resize(outputBits);
    
    for (int i = 0; i < inputBits; i++) {
        inputPins[i] = i;
    }
    
    for (int i = 0; i < outputBits; i++) {
        outputPins[i] = inputBits + i;
    }
    
    enablePin = inputBits + outputBits;
    
    // Create gates for implementation
    andGates.resize(outputBits);
    for (int i = 0; i < outputBits; i++) {
        andGates[i] = std::make_unique<TubeANDGate>(inputBits + 1);  // +1 for enable
    }
    
    // Create NOT gates for input inversion when needed
    notGates.resize(inputBits);
    for (int i = 0; i < inputBits; i++) {
        notGates[i] = std::make_unique<TubeNOTGate>();
    }
}

bool TubeDecoder::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeDecoder::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    // Handle input pins
    for (int i = 0; i < inputBits; i++) {
        if (conn_id == static_cast<uint16>(inputPins[i]) && data_bytes == sizeof(double)) {
            inputs[i] = voltageToLogic(*reinterpret_cast<double*>(data));
            return true;
        }
    }
    
    if (conn_id == enablePin && data_bytes == sizeof(double)) {
        enabled = voltageToLogic(*reinterpret_cast<double*>(data));
        return true;
    }
    
    return false;
}

bool TubeDecoder::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    // Handle output pins
    for (int i = 0; i < outputBits; i++) {
        if (conn_id == static_cast<uint16>(outputPins[i]) && data_bytes == sizeof(double)) {
            double voltage = logicToVoltage(outputs[i]);
            memcpy(data, &voltage, sizeof(double));
            return true;
        }
    }
    return false;
}

bool TubeDecoder::Tick() {
    if (enabled) {
        evaluateOutputs();
    } else {
        // When disabled, all outputs are 0
        for (int i = 0; i < outputBits; i++) {
            outputs[i] = false;
        }
    }
    return true;
}

void TubeDecoder::setInput(int bit, bool value) {
    if (bit >= 0 && bit < inputBits) {
        inputs[bit] = value;
    }
}

void TubeDecoder::setInputs(const std::vector<bool>& values) {
    int bitsToSet = std::min(static_cast<int>(values.size()), inputBits);
    for (int i = 0; i < bitsToSet; i++) {
        setInput(i, values[i]);
    }
}

void TubeDecoder::evaluateOutputs() {
    // Calculate which output should be activated
    int outputIndex = 0;
    for (int i = 0; i < inputBits; i++) {
        if (inputs[i]) {
            outputIndex |= (1 << i);
        }
    }
    
    // Set outputs: only the selected output is high
    for (int i = 0; i < outputBits; i++) {
        outputs[i] = (i == outputIndex && enabled);
    }
}


// TubeDecoder3To8 implementation
TubeDecoder3To8::TubeDecoder3To8() : TubeDecoder(3, 8) { }

void TubeDecoder3To8::setInputs(bool a, bool b, bool c) {
    setInput(0, c);  // LSB
    setInput(1, b);
    setInput(2, a);  // MSB
}


// TubeAnalogMultiplexer implementation
TubeAnalogMultiplexer::TubeAnalogMultiplexer(int channelCount, int sampleRate) 
    : channelCount(channelCount), sampleRate(sampleRate) {
    if (channelCount < 2) channelCount = 2;
    if (channelCount > 16) channelCount = 16;  // Reasonable limit
    
    // Calculate select bits needed
    selectBits = 1;
    int temp = channelCount - 1;
    while (temp >>= 1) selectBits++;
    
    initialize();
}

void TubeAnalogMultiplexer::initialize() {
    // Initialize analog inputs
    analogInputs.resize(channelCount, 0.0);
    
    // Initialize select inputs
    selectInputs.resize(selectBits, false);
    
    // Pin assignments
    analogInputPins.resize(channelCount);
    selectPins.resize(selectBits);
    
    for (int i = 0; i < channelCount; i++) {
        analogInputPins[i] = i;
    }
    
    for (int i = 0; i < selectBits; i++) {
        selectPins[i] = channelCount + i;
    }
    
    outputPin = channelCount + selectBits;
    enablePin = channelCount + selectBits + 1;
}

bool TubeAnalogMultiplexer::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeAnalogMultiplexer::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    // Handle analog input pins
    for (int i = 0; i < channelCount; i++) {
        if (conn_id == static_cast<uint16>(analogInputPins[i]) && data_bytes == sizeof(double)) {
            memcpy(&analogInputs[i], data, sizeof(double));
            return true;
        }
    }
    
    // Handle select pins
    for (int i = 0; i < selectBits; i++) {
        if (conn_id == static_cast<uint16>(selectPins[i]) && data_bytes == sizeof(double)) {
            selectInputs[i] = voltageToLogic(*reinterpret_cast<double*>(data));
            return true;
        }
    }
    
    if (conn_id == enablePin && data_bytes == sizeof(double)) {
        enabled = voltageToLogic(*reinterpret_cast<double*>(data));
        return true;
    }
    
    return false;
}

bool TubeAnalogMultiplexer::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == static_cast<uint16>(outputPin) && data_bytes == sizeof(double)) {
        memcpy(data, &output, sizeof(double));
        return true;
    }
    return false;
}

bool TubeAnalogMultiplexer::Tick() {
    if (enabled) {
        updateSelection();
        updateOutput();
    } else {
        output = 0.0;  // Disabled output is 0
    }
    return true;
}

void TubeAnalogMultiplexer::setAnalogInput(int channel, double value) {
    if (channel >= 0 && channel < channelCount) {
        analogInputs[channel] = value;
    }
}

void TubeAnalogMultiplexer::setSelectInput(int bit, bool value) {
    if (bit >= 0 && bit < selectBits) {
        selectInputs[bit] = value;
    }
}

void TubeAnalogMultiplexer::setSelectInputs(const std::vector<bool>& values) {
    int bitsToSet = std::min(static_cast<int>(values.size()), selectBits);
    for (int i = 0; i < bitsToSet; i++) {
        setSelectInput(i, values[i]);
    }
}

void TubeAnalogMultiplexer::updateSelection() {
    // Convert select bits to channel index
    selectedChannel = 0;
    for (int i = 0; i < selectBits; i++) {
        if (selectInputs[i]) {
            selectedChannel |= (1 << i);
        }
    }
    
    // Ensure selected channel is valid
    if (selectedChannel >= channelCount) {
        selectedChannel = 0;  // Default to first channel if out of range
    }
}

void TubeAnalogMultiplexer::updateOutput() {
    // Output the selected channel's analog value
    output = analogInputs[selectedChannel];
}