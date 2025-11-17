#include "TubeCircuitTopologies.h"
#include "TubeModels.h"
#include <cmath>

// CathodeFollower implementation
CathodeFollower::CathodeFollower(const std::string& tubeType) : tubeType(tubeType) {
    tubeModel = std::make_unique<TriodeModel>();
    updateParams();
}

void CathodeFollower::updateParams() {
    // Update parameters based on tube type
    if (tubeType == "12AX7") {
        tubeModel->setAmplificationFactor(100.0);
        tubeModel->setTransconductance(0.00165);
        // Adjust cathode resistor for proper biasing
        cathodeResistor = 1500.0;
        calculatedGain = 0.95;
        outputImpedance = 1500.0 / 100.0;  // Ra/(mu+1) approximation
    } else if (tubeType == "12AU7") {
        tubeModel->setAmplificationFactor(44.0);
        tubeModel->setTransconductance(0.00175);
        cathodeResistor = 1500.0;
        calculatedGain = 0.92;
        outputImpedance = 1500.0 / 45.0;
    } else if (tubeType == "ECC803S") {
        tubeModel->setAmplificationFactor(95.0);
        tubeModel->setTransconductance(0.0016);
        cathodeResistor = 1500.0;
        calculatedGain = 0.95;
        outputImpedance = 1500.0 / 96.0;
    }
    
    // Update input impedance (very high for cathode follower)
    inputImpedance = gridResistor;
}

bool bool CathodeFollower::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool CathodeFollower::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == inputPin && data_bytes == sizeof(double)) {
        memcpy(&inputSignal, data, sizeof(double));
        return true;
    } else if (conn_id == bPlusPin && data_bytes == sizeof(double)) {
        memcpy(&bPlusVoltage, data, sizeof(double));
        return true;
    }
    return false;
}

bool CathodeFollower::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == outputPin && data_bytes == sizeof(double)) {
        memcpy(data, &outputSignal, sizeof(double));
        return true;
    }
    return false;
}

bool CathodeFollower::Tick() {
    processSignal();
    return true;
}

void CathodeFollower::processSignal() {
    // In a cathode follower, the output is taken from the cathode
    // The grid voltage is approximately the input voltage
    gridVoltage = inputSignal + operatingBias;
    
    // Calculate the plate-cathode voltage based on current and cathode resistor
    // For a cathode follower, we're primarily interested in the cathode voltage
    // which follows the grid voltage but with gain slightly less than 1
    
    // Calculate current based on tube characteristics
    double v_gk = gridVoltage - cathodeVoltage;  // Grid-to-cathode voltage
    
    // For a first approximation in cathode follower
    double target_current = 0.002;  // 2mA target current
    
    // Adjust cathode voltage to achieve target current
    // This is a simplified approach - in reality, this would be solved iteratively
    double expected_current = dynamic_cast<TriodeModel*>(tubeModel.get())->calculateAnodeCurrent(v_gk, bPlusVoltage - cathodeVoltage);
    if (expected_current > 0) {
        double cathode_current = bPlusVoltage / (loadResistor + cathodeResistor);
        cathodeVoltage = cathode_current * cathodeResistor;
    }
    
    // The output voltage follows the input with gain less than 1
    outputSignal = inputSignal * calculatedGain;
    
    // Add some tube non-linearity
    if (outputSignal > 0.7) outputSignal = 0.7 + 0.3 * tanh((outputSignal - 0.7) / 0.3);
    if (outputSignal < -0.7) outputSignal = -0.7 + 0.3 * tanh((outputSignal + 0.7) / 0.3);
}


// GroundedCathodeAmp implementation
GroundedCathodeAmp::GroundedCathodeAmp(const std::string& tubeType) : tubeType(tubeType) {
    tubeModel = std::make_unique<TriodeModel>();
    updateParams();
}

void GroundedCathodeAmp::updateParams() {
    if (tubeType == "12AX7") {
        tubeModel->setAmplificationFactor(100.0);
        tubeModel->setTransconductance(0.00165);
        plateResistor = 100000.0;
        calculatedGain = -66.0;  // Approximation: mu * Rp / (Rp + ra), where ra = 1/gm
    } else if (tubeType == "12AU7") {
        tubeModel->setAmplificationFactor(44.0);
        tubeModel->setTransconductance(0.00175);
        plateResistor = 100000.0;
        calculatedGain = -28.0;
    }
    
    // Calculate output impedance
    outputImpedance = plateResistor;
    inputImpedance = gridResistor;
}

bool bool GroundedCathodeAmp::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool GroundedCathodeAmp::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == inputPin && data_bytes == sizeof(double)) {
        memcpy(&inputSignal, data, sizeof(double));
        return true;
    } else if (conn_id == bPlusPin && data_bytes == sizeof(double)) {
        memcpy(&bPlusVoltage, data, sizeof(double));
        return true;
    }
    return false;
}

bool GroundedCathodeAmp::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == outputPin && data_bytes == sizeof(double)) {
        memcpy(data, &outputSignal, sizeof(double));
        return true;
    }
    return false;
}

bool GroundedCathodeAmp::Tick() {
    processSignal();
    return true;
}

void GroundedCathodeAmp::processSignal() {
    // Calculate grid voltage with bias
    gridVoltage = inputSignal + operatingBias;
    
    // Calculate cathode voltage (for common cathode, it's typically grounded)
    cathodeVoltage = 0.0;  // Grounded cathode
    
    // Calculate grid-to-cathode voltage
    double v_gk = gridVoltage - cathodeVoltage;
    
    // Calculate anode-to-cathode voltage (anode voltage will be determined by current and plate resistor)
    double v_ak = bPlusVoltage;  // Initial approximation
    
    // Calculate anode current using the tube model
    double i_a = dynamic_cast<TriodeModel*>(tubeModel.get())->calculateAnodeCurrent(v_gk, v_ak);
    
    // Calculate voltage drop across plate resistor
    double v_drop = i_a * plateResistor;
    
    // Calculate anode voltage (output)
    plateVoltage = bPlusVoltage - v_drop;
    
    // For small signals, the gain is approximately -gm * Rp (in parallel with load)
    double small_signal_gain = -tubeModel->getTransconductance() * plateResistor;
    
    // Apply gain to input signal
    outputSignal = inputSignal * small_signal_gain;
    
    // Add some non-linearity to simulate tube saturation
    if (outputSignal > (bPlusVoltage * 0.9)) outputSignal = bPlusVoltage * 0.9;
    if (outputSignal < 0) outputSignal = 0;
    
    // Apply soft clipping to simulate tube saturation
    if (outputSignal > 5.0) outputSignal = 5.0 + 2.5 * tanh((outputSignal - 5.0) / 2.5);
    if (outputSignal < -5.0) outputSignal = -5.0 + 2.5 * tanh((outputSignal + 5.0) / 2.5);
}


// GroundedGridAmp implementation
GroundedGridAmp::GroundedGridAmp(const std::string& tubeType) : tubeType(tubeType) {
    tubeModel = std::make_unique<TriodeModel>();
    updateParams();
}

void GroundedGridAmp::updateParams() {
    if (tubeType == "6DJ8") {
        tubeModel->setAmplificationFactor(18.0);
        tubeModel->setTransconductance(0.0067);
        plateResistor = 47000.0;
        calculatedGain = 15.0;  // Lower gain than common cathode but higher input current
    } else if (tubeType == "ECC803S") {
        tubeModel->setAmplificationFactor(95.0);
        tubeModel->setTransconductance(0.0016);
        plateResistor = 47000.0;
        calculatedGain = 65.0;
    }
    
    outputImpedance = plateResistor;
    inputImpedance = 1.0 / tubeModel->getTransconductance();  // Low input impedance
}

bool bool GroundedGridAmp::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool GroundedGridAmp::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == inputPin && data_bytes == sizeof(double)) {
        memcpy(&inputSignal, data, sizeof(double));
        return true;
    } else if (conn_id == bPlusPin && data_bytes == sizeof(double)) {
        memcpy(&bPlusVoltage, data, sizeof(double));
        return true;
    }
    return false;
}

bool GroundedGridAmp::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == outputPin && data_bytes == sizeof(double)) {
        memcpy(data, &outputSignal, sizeof(double));
        return true;
    }
    return false;
}

bool GroundedGridAmp::Tick() {
    processSignal();
    return true;
}

void GroundedGridAmp::processSignal() {
    // In grounded grid, input is applied to cathode
    cathodeVoltage = inputSignal + operatingBias;
    
    // Grid is at ground potential
    gridVoltage = gridBias;  // Usually 0V or slightly negative
    
    // Calculate grid-to-cathode voltage
    double v_gk = gridVoltage - cathodeVoltage;
    
    // Calculate anode voltage based on current through plate resistor
    double i_a = dynamic_cast<TriodeModel*>(tubeModel.get())->calculateAnodeCurrent(v_gk, bPlusVoltage);
    
    // Calculate anode voltage (output)
    plateVoltage = bPlusVoltage - (i_a * plateResistor);
    
    // Grounded grid has the same gain as common cathode but non-inverting
    // In reality, it's slightly less due to the cathode resistor
    outputSignal = inputSignal * calculatedGain;
    
    // Apply soft limiting
    if (outputSignal > 0.9 * bPlusVoltage) outputSignal = 0.9 * bPlusVoltage;
    if (outputSignal < 0) outputSignal = 0;
}


// LongTailedPair implementation
LongTailedPair::LongTailedPair(const std::string& tubeType) : tubeType(tubeType) {
    tubeModel1 = std::make_unique<TriodeModel>();
    tubeModel2 = std::make_unique<TriodeModel>();
    updateParams();
}

void LongTailedPair::updateParams() {
    if (tubeType == "12AX7") {
        tubeModel1->setAmplificationFactor(100.0);
        tubeModel1->setTransconductance(0.00165);
        tubeModel2->setAmplificationFactor(100.0);
        tubeModel2->setTransconductance(0.00165);
        plateResistor = 100000.0;
        diffGain = -66.0;
    }
    
    // Common mode rejection ratio is typically around 30-60 dB
    cmrr = 40.0;  // dB
}

bool bool LongTailedPair::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool LongTailedPair::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == input1Pin && data_bytes == sizeof(double)) {
        memcpy(&input1Signal, data, sizeof(double));
        return true;
    } else if (conn_id == input2Pin && data_bytes == sizeof(double)) {
        memcpy(&input2Signal, data, sizeof(double));
        return true;
    } else if (conn_id == bPlusPin && data_bytes == sizeof(double)) {
        memcpy(&bPlusVoltage, data, sizeof(double));
        return true;
    }
    return false;
}

bool LongTailedPair::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == output1Pin && data_bytes == sizeof(double)) {
        memcpy(data, &output1Signal, sizeof(double));
        return true;
    } else if (conn_id == output2Pin && data_bytes == sizeof(double)) {
        memcpy(data, &output2Signal, sizeof(double));
        return true;
    }
    return false;
}

bool LongTailedPair::Tick() {
    processSignal();
    return true;
}

void LongTailedPair::processSignal() {
    // Calculate voltages for each tube
    double grid1 = input1Signal + operatingBias;
    double grid2 = input2Signal + operatingBias;
    
    // The tail resistor sets the total current, which is shared between both tubes
    double totalCurrent = bPlusVoltage / tailResistor;
    
    // Calculate individual currents based on input difference
    // For a simplified model, assume current splits based on input differential
    double diffInput = input1Signal - input2Signal;
    double current1 = totalCurrent/2 + diffInput * 0.001;  // Rough approximation
    double current2 = totalCurrent/2 - diffInput * 0.001;
    
    // Limit currents to reasonable values
    current1 = fmax(0.0, fmin(totalCurrent, current1));
    current2 = fmax(0.0, fmin(totalCurrent, current2));
    
    // Calculate output voltages
    output1Signal = bPlusVoltage - current1 * plateResistor;
    output2Signal = bPlusVoltage - current2 * plateResistor;
}


// PhaseInverter implementation
PhaseInverter::PhaseInverter(InverterType type, const std::string& tubeType) 
    : inverterType(type), tubeType(tubeType) {
    tubeModel1 = std::make_unique<TriodeModel>();
    tubeModel2 = std::make_unique<TriodeModel>();
    updateParams();
}

void PhaseInverter::updateParams() {
    if (tubeType == "12AX7") {
        tubeModel1->setAmplificationFactor(100.0);
        tubeModel1->setTransconductance(0.00165);
        tubeModel2->setAmplificationFactor(100.0);
        tubeModel2->setTransconductance(0.00165);
    }
    
    switch (inverterType) {
        case SPLIT_LOAD:
            plateResistor = 100000.0;
            cathodeResistor = 8200.0;
            phaseBalance = 180.0;
            gainBalance = 1.0;
            break;
        case CATHODE_COUPLED:
            plateResistor = 100000.0;
            cathodeResistor = 8200.0;
            phaseBalance = 180.0;
            gainBalance = 1.0;
            break;
        case DIFFERENTIAL:
            plateResistor = 100000.0;
            phaseBalance = 180.0;
            gainBalance = 1.0;
            break;
    }
}

bool bool PhaseInverter::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool PhaseInverter::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == inputPin && data_bytes == sizeof(double)) {
        memcpy(&inputSignal, data, sizeof(double));
        return true;
    } else if (conn_id == bPlusPin && data_bytes == sizeof(double)) {
        memcpy(&bPlusVoltage, data, sizeof(double));
        return true;
    }
    return false;
}

bool PhaseInverter::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == outputInPhasePin && data_bytes == sizeof(double)) {
        memcpy(data, &outputInPhase, sizeof(double));
        return true;
    } else if (conn_id == outputOutOfPhasePin && data_bytes == sizeof(double)) {
        memcpy(data, &outputOutOfPhase, sizeof(double));
        return true;
    }
    return false;
}

bool PhaseInverter::Tick() {
    processSignal();
    return true;
}

void PhaseInverter::processSignal() {
    switch (inverterType) {
        case SPLIT_LOAD:
            processSplitLoad();
            break;
        case CATHODE_COUPLED:
            processCathodeCoupled();
            break;
        case DIFFERENTIAL:
            processDifferential();
            break;
    }
}

void PhaseInverter::processSplitLoad() {
    // In split-load phase inverter, one output is taken from the plate (inverted)
    // and the other from the cathode (non-inverted)
    
    // Input drives the grid
    double gridVoltage = inputSignal - 1.5;  // Apply bias
    
    // Calculate cathode voltage (voltage across cathode resistor)
    double cathodeCurrent = 0.002;  // 2mA typical
    double cathodeVoltage = cathodeCurrent * cathodeResistor;
    
    // Calculate anode voltage (output from plate)
    double plateDrop = cathodeCurrent * plateResistor;
    double plateVoltage = bPlusVoltage - plateDrop;
    
    // Outputs
    outputInPhase = cathodeVoltage * 0.1;  // Scaled appropriately
    outputOutOfPhase = (bPlusVoltage - plateVoltage) * 0.1 - 0.5;  // Inverted and scaled
}

void PhaseInverter::processCathodeCoupled() {
    // In cathode-coupled (concertina) phase inverter, 
    // input drives one grid, the other is connected to the cathode
    // Outputs are taken from both plates
    
    // For a simplified model, we'll treat this as a differential pair with specific resistor values
    double input1 = inputSignal;
    double input2 = 0.0;  // Connected to cathode of first triode
    
    // Differential amplification
    outputInPhase = bPlusVoltage * 0.7 + input1 * 10.0;      // One output
    outputOutOfPhase = bPlusVoltage * 0.7 - input1 * 10.0;   // Inverted output
    
    // Apply limiting to keep within power supply rails
    outputInPhase = fmin(bPlusVoltage * 0.95, fmax(0.05 * bPlusVoltage, outputInPhase));
    outputOutOfPhase = fmin(bPlusVoltage * 0.95, fmax(0.05 * bPlusVoltage, outputOutOfPhase));
}

void PhaseInverter::processDifferential() {
    // Differential amplifier: input goes to one grid, the other grid is referenced
    // to some bias point; outputs are taken from both plates
    
    // For a simplified approach:
    double diffOutput1 = bPlusVoltage - (0.001 * plateResistor);  // Quiescent
    double diffOutput2 = bPlusVoltage - (0.001 * plateResistor);  // Quiescent
    
    // Apply differential gain to inputs
    diffOutput1 += inputSignal * 15.0;   // Positive gain applied
    diffOutput2 -= inputSignal * 15.0;   // Negative gain applied (inverted)
    
    outputInPhase = diffOutput1 * 0.2;      // Scale to manageable levels
    outputOutOfPhase = diffOutput2 * 0.2;   // Scale to manageable levels
}


// TubeStage implementation
TubeStage::TubeStage(StageType type, const std::string& tubeType) : stageType(type), tubeType(tubeType) {
    initializeStage(type);
}

void TubeStage::initializeStage(StageType type) {
    switch (type) {
        case VOLTAGE_AMPLIFIER:
            groundedCathodeAmp = std::make_unique<GroundedCathodeAmp>(tubeType);
            calculatedGain = groundedCathodeAmp->getGain();
            outputImpedance = groundedCathodeAmp->getOutputImpedance();
            inputImpedance = groundedCathodeAmp->getInputImpedance();
            break;
        case CURRENT_BUFFER:
            cathodeFollower = std::make_unique<CathodeFollower>(tubeType);
            calculatedGain = cathodeFollower->getGain();
            outputImpedance = cathodeFollower->getOutputImpedance();
            inputImpedance = cathodeFollower->getInputImpedance();
            break;
        case VOLTAGE_BUFFER:
            cathodeFollower = std::make_unique<CathodeFollower>(tubeType);
            calculatedGain = cathodeFollower->getGain();
            outputImpedance = cathodeFollower->getOutputImpedance();
            inputImpedance = cathodeFollower->getInputImpedance();
            break;
        case PHASE_SPLITTER:
            phaseInverter = std::make_unique<PhaseInverter>(PhaseInverter::DIFFERENTIAL, tubeType);
            calculatedGain = 10.0;  // Typical gain for phase inverter
            outputImpedance = 100000.0;
            inputImpedance = 1000000.0;
            break;
    }
}

bool bool TubeStage::Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) {::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (op == OP_READ) {
        return GetRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_WRITE) {
        return PutRaw(conn_id, data, data_bytes, data_bits);
    } else if (op == OP_TICK) {
        return Tick();
    }
    return false;
}

bool TubeStage::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == inputPin && data_bytes == sizeof(double)) {
        memcpy(&inputSignal, data, sizeof(double));
        
        // Pass the input to the appropriate internal component
        switch (stageType) {
            case VOLTAGE_AMPLIFIER:
                if (groundedCathodeAmp) {
                    // In a real system, we'd properly connect the components
                    // For this simulation, we'll just process through the appropriate function
                }
                break;
            case CURRENT_BUFFER:
            case VOLTAGE_BUFFER:
                if (cathodeFollower) {
                    // Process through cathode follower internally
                }
                break;
            case PHASE_SPLITTER:
                if (phaseInverter) {
                    // Process through phase inverter internally
                }
                break;
        }
        return true;
    } else if (conn_id == bPlusPin && data_bytes == sizeof(double)) {
        memcpy(&inputSignal, data, sizeof(double));
        return true;
    }
    return false;
}

bool TubeStage::GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == outputPin && data_bytes == sizeof(double)) {
        memcpy(data, &outputSignal, sizeof(double));
        return true;
    }
    return false;
}

bool TubeStage::Tick() {
    processSignal();
    return true;
}

void TubeStage::processSignal() {
    switch (stageType) {
        case VOLTAGE_AMPLIFIER:
            outputSignal = inputSignal * calculatedGain * 0.1;  // Scaled gain
            break;
        case CURRENT_BUFFER:
        case VOLTAGE_BUFFER:
            outputSignal = inputSignal * calculatedGain;
            // Apply soft clipping for tube-like behavior
            if (outputSignal > 0.8) outputSignal = 0.8 + 0.2 * tanh((outputSignal - 0.8) / 0.2);
            if (outputSignal < -0.8) outputSignal = -0.8 + 0.2 * tanh((outputSignal + 0.8) / 0.2);
            break;
        case PHASE_SPLITTER:
            // This would produce two outputs in a real implementation
            outputSignal = inputSignal * 0.5;  // Scaled for single output representation
            break;
    }
}