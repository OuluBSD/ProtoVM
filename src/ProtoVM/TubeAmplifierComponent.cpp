#include "TubeAmplifierComponent.h"
#include "TubeDistortion.h"
#include <cstring>

// TubeAmplifierComponent implementation
TubeAmplifierComponent::TubeAmplifierComponent() {
    // Add a basic preamp stage by default
    addPreamplifierStage("12AX7", 35.0, 1.0);
    
    // Add power amp stage by default
    addPowerAmplifierStage("EL34", 2);  // Push-pull configuration
}

bool bool TubeAmplifierComponent::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeAmplifierComponent::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == inputPin && data_bytes == sizeof(double)) {
        // Input signal
        memcpy(&inputSignal, data, sizeof(double));
        return true;
    } else if (conn_id == bPlusPin && data_bytes == sizeof(double)) {
        // B+ supply voltage
        memcpy(&bPlusVoltage, data, sizeof(double));
        return true;
    }
    return false;
}

bool TubeAmplifierComponent::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == outputPin && data_bytes == sizeof(double)) {
        // Output signal
        memcpy(data, &currentOutput, sizeof(double));
        return true;
    }
    return false;
}

bool TubeAmplifierComponent::Tick() {
    processSignal();
    return true;
}

void TubeAmplifierComponent::processSignal() {
    // Process the input signal through the amplifier simulation
    currentOutput = simulator.processSample(inputSignal);
}

void TubeAmplifierComponent::addPreamplifierStage(const std::string& tubeType, double gain, double drive) {
    simulator.addPreamplifierStage(tubeType, gain, drive);
}

void TubeAmplifierComponent::addPhaseSplitterStage(const std::string& tubeType) {
    simulator.addPhaseSplitterStage(tubeType);
}

void TubeAmplifierComponent::addPowerAmplifierStage(const std::string& tubeType, int numTubes) {
    simulator.addPowerAmplifierStage(tubeType, numTubes);
}

void TubeAmplifierComponent::setToneControls(double bass, double mid, double treble) {
    simulator.setToneControls(bass, mid, treble);
}


// TubeCircuitComponent implementation
TubeCircuitComponent::TubeCircuitComponent(CircuitType type) : circuitType(type) {
    // Configure pins based on circuit type
    switch (circuitType) {
        case CATHODE_FOLLOWER:
            inputPins = {0};  // Single input
            outputPin = 1;
            supplyPin = 2;
            groundPin = 3;
            tubeConfig.setConfiguration(TubeConfigurationModel::CATHODE_FOLLOWER);
            break;
            
        case DIFFERENTIAL_PAIR:
        case LONG_TAILED_PAIR:
            inputPins = {0, 1};  // Two inputs
            outputPin = 2;
            supplyPin = 3;
            groundPin = 4;
            tubeConfig.setConfiguration(TubeConfigurationModel::DIFFERENTIAL_PAIR);
            break;
            
        case CLASS_A_SINGLE_ENDED:
            inputPins = {0};  // Single input
            outputPin = 1;
            supplyPin = 2;
            groundPin = 3;
            tubeConfig.setConfiguration(TubeConfigurationModel::SINGLE_ENDED_TRIODE);
            break;
            
        case CLASS_A_PUSH_PULL:
        case CLASS_AB_PUSH_PULL:
            inputPins = {0};  // Single input (phase splitter handles the rest)
            outputPin = 1;
            supplyPin = 2;
            groundPin = 3;
            tubeConfig.setConfiguration(TubeConfigurationModel::PUSH_PULL_CLASS_AB);
            break;
    }
    
    // Set default tube type based on circuit type
    switch (circuitType) {
        case CATHODE_FOLLOWER:
        case CLASS_A_SINGLE_ENDED:
            setTubeType("12AX7");  // Common triode for these circuits
            break;
        case DIFFERENTIAL_PAIR:
        case LONG_TAILED_PAIR:
            setTubeType("12AX7");  // Common for phase splitters
            break;
        case CLASS_A_PUSH_PULL:
        case CLASS_AB_PUSH_PULL:
            setTubeType("EL34");  // Common power tube
            break;
    }
}

bool bool TubeCircuitComponent::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeCircuitComponent::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    // Check if this connection matches any input pin
    for (size_t i = 0; i < inputPins.size(); ++i) {
        if (conn_id == static_cast<uint16>(inputPins[i]) && data_bytes == sizeof(double)) {
            // For differential inputs, we might need to store multiple values
            double input;
            memcpy(&input, data, sizeof(double));
            
            if (i == 0) {
                inputSignal = input;  // For single input circuits, or first input of differential
            } else if (i == 1 && circuitType == DIFFERENTIAL_PAIR) {
                // For differential pair, we'd need to handle both inputs
                // In this simplified model, we'll just process the current inputSignal with the new input
            }
            
            return true;
        }
    }
    
    // Check for supply voltage
    if (conn_id == supplyPin && data_bytes == sizeof(double)) {
        double supply;
        memcpy(&supply, data, sizeof(double));
        // Use this for internal calculations if needed
        return true;
    }
    
    return false;
}

bool TubeCircuitComponent::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == outputPin && data_bytes == sizeof(double)) {
        memcpy(data, &currentOutput, sizeof(double));
        return true;
    }
    return false;
}

bool TubeCircuitComponent::Tick() {
    processSignal();
    return true;
}

void TubeCircuitComponent::processSignal() {
    // Process the input through the appropriate tube configuration
    currentOutput = tubeConfig.processSample(inputSignal);
}