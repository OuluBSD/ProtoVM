#include "CommonTubeTopologies.h"
#include <algorithm>
#include <cmath>

// TubeTopologyBase implementation
TubeTopologyBase::TubeTopologyBase(TubeTopology topology) 
    : topology_type(topology)
    , input_signal(0.0)
    , input_signal2(0.0)
    , output_signal(0.0)
    , topology_gain(1.0)
    , output_impedance(62000.0)  // Default to triode plate resistance
    , input_impedance(1000000.0) // High input impedance
    , is_enabled(true)
{
    // Initialize with basic tube
    tubes.push_back(std::make_unique<Triode>());
    ConfigureTubes();
}

TubeTopologyBase::~TubeTopologyBase() {
    // Cleanup handled by destructors
}

bool TubeTopologyBase::Tick() {
    if (!is_enabled) {
        output_signal = input_signal;
        return true;
    }
    
    // Process the signal through the topology
    ProcessSignal();
    
    // Tick all tubes in the topology
    for (auto& tube : tubes) {
        tube->Tick();
    }
    
    return true;
}


// CathodeFollower implementation
CathodeFollower::CathodeFollower(double mu, double rp, double gm) 
    : TubeTopologyBase(TubeTopology::CATHODE_FOLLOWER)
    , cathode_resistor(2200.0)  // 2.2kΩ typical
    , plate_resistor(100000.0)  // 100kΩ for biasing
{
    // Clear and configure for cathode follower
    tubes.clear();
    tubes.push_back(std::make_unique<Triode>(mu, rp, gm));
    ConfigureTubes();
}

void CathodeFollower::ConfigureTubes() {
    // Configure the tube for cathode follower operation
    if (tubes.empty()) return;
    
    auto& tube = tubes[0];
    if (auto triode = dynamic_cast<Triode*>(tube.get())) {
        triode->SetAmplificationFactor(0.5);  // Very low mu for cathode follower
        triode->SetPlateResistance(1000.0);   // Low effective output impedance
        triode->SetTransconductance(5.0e-3);  // Higher gm
    }
    
    // Set topology characteristics
    topology_gain = 0.95;  // Close to unity gain
    output_impedance = 1000.0;  // Lower output impedance than triode
    input_impedance = 1000000.0;  // Very high input impedance
}

void CathodeFollower::ProcessSignal() {
    if (tubes.empty()) {
        output_signal = input_signal;
        return;
    }
    
    auto& tube = tubes[0];
    
    // In cathode follower, input goes to grid, output is taken from cathode
    // The cathode voltage follows the grid voltage (1:1 gain, in-phase)
    
    // Apply input signal to grid with proper bias
    double grid_voltage = -1.5 + input_signal * 0.5;  // Bias at -1.5V, input scaled
    tube->SetGridVoltage(grid_voltage);
    tube->SetPlateVoltage(250.0);  // Typical plate voltage
    tube->SetCathodeVoltage(0.0);  // Cathode initially at 0V
    
    // Calculate tube behavior
    tube->CalculateTubeBehavior();
    
    // For cathode follower, the output is the cathode voltage
    // This is approximated by the current through the cathode resistor
    double plate_current = tube->GetPlateCurrent();
    double cathode_voltage = plate_current * cathode_resistor;
    
    // The cathode voltage follows the grid voltage - apply gain factor
    output_signal = (grid_voltage + 1.5) * 0.98;  // Slightly less than unity
    
    // Ensure output is within reasonable bounds
    output_signal = std::max(-5.0, std::min(5.0, output_signal));
}

double CathodeFollower::GetTheoreticalOutputImpedance() const {
    // Zout ≈ Rp / (mu + 1) for cathode follower
    if (tubes.empty()) return output_impedance;
    
    auto triode = dynamic_cast<Triode*>(tubes[0].get());
    if (!triode) return output_impedance;
    
    double mu = triode->GetAmplificationFactor();
    double rp = triode->GetPlateResistance();
    return rp / (mu + 1.0);
}

double CathodeFollower::GetTheoreticalInputImpedance() const {
    // Very high input impedance for cathode follower (basically grid resistance)
    return 1000000.0;  // 1M Ohm or higher
}


// CommonCathodeAmp implementation
CommonCathodeAmp::CommonCathodeAmp(double mu, double rp, double gm) 
    : TubeTopologyBase(TubeTopology::COMMON_CATHODE)
    , plate_resistor(100000.0)  // 100kΩ typical
    , cathode_resistor(1500.0)  // 1.5kΩ typical
{
    tubes.clear();
    tubes.push_back(std::make_unique<Triode>(mu, rp, gm));
    ConfigureTubes();
}

void CommonCathodeAmp::ConfigureTubes() {
    // The tube is already configured with passed parameters
    // Set topology characteristics
    topology_gain = -20.0;  // Inverting gain
    output_impedance = 62000.0;  // Close to plate resistance
    input_impedance = 1000000.0;  // High input impedance
}

void CommonCathodeAmp::ProcessSignal() {
    if (tubes.empty()) {
        output_signal = input_signal * topology_gain;
        return;
    }
    
    auto& tube = tubes[0];
    
    // Input to grid, output from plate, cathode grounded (or via bypass cap)
    double grid_voltage = -1.5 + input_signal * 0.1;  // Bias at -1.5V, input scaled
    tube->SetGridVoltage(grid_voltage);
    tube->SetPlateVoltage(250.0);  // Plate voltage
    
    // Calculate cathode voltage based on current and cathode resistor
    // For simplicity, we'll assume cathode is effectively grounded (bypassed)
    tube->SetCathodeVoltage(0.0);
    
    // Calculate tube behavior
    tube->CalculateTubeBehavior();
    
    // Calculate plate voltage based on plate current and load resistor
    double plate_current = tube->GetPlateCurrent();
    double plate_voltage_drop = plate_current * plate_resistor;
    double plate_voltage = 250.0 - plate_voltage_drop;
    
    // Output is the plate voltage swing relative to DC bias
    output_signal = (plate_voltage - (250.0 - plate_current * plate_resistor)) * 0.5;
    
    // Apply cathode degeneration effect if cathode resistor is used
    if (cathode_resistor > 0.0) {
        // This reduces gain but increases linearity
        double degeneration_factor = 1.0 / (1.0 + tube->GetTransconductance() * cathode_resistor);
        output_signal *= degeneration_factor;
    }
    
    // Ensure output is within reasonable bounds
    output_signal = std::max(-5.0, std::min(5.0, output_signal));
}

double CommonCathodeAmp::GetTheoreticalGain() const {
    if (tubes.empty()) return topology_gain;
    
    auto triode = dynamic_cast<Triode*>(tubes[0].get());
    if (!triode) return topology_gain;
    
    // For common cathode: A = -gm * Rp (without cathode degeneration)
    double gm = triode->GetTransconductance();
    if (cathode_resistor > 0.0) {
        // With cathode degeneration: A = -gm * Rp / (1 + gm * Rk)
        return -gm * plate_resistor / (1.0 + gm * cathode_resistor);
    } else {
        return -gm * plate_resistor;
    }
}


// DifferentialAmp implementation
DifferentialAmp::DifferentialAmp(double mu, double rp, double gm) 
    : TubeTopologyBase(TubeTopology::DIFF_AMP)
    , load_resistor(100000.0)  // 100kΩ typical load
    , tail_resistor(200000.0)  // 200kΩ typical tail resistor
    , differential_gain(30.0)   // Typical differential gain
    , cmrr(80.0)               // 80dB CMRR
{
    tubes.clear();
    tubes.push_back(std::make_unique<Triode>(mu, rp, gm));  // Left triode
    tubes.push_back(std::make_unique<Triode>(mu, rp, gm));  // Right triode
    ConfigureTubes();
}

void DifferentialAmp::ConfigureTubes() {
    // Configure both tubes for differential operation
    topology_gain = 30.0;
    output_impedance = 62000.0;
    input_impedance = 1000000.0;
}

void DifferentialAmp::ProcessSignal() {
    if (tubes.size() < 2) {
        output_signal = (input_signal - input_signal2) * topology_gain;
        return;
    }
    
    auto& tube1 = tubes[0];
    auto& tube2 = tubes[1];
    
    // Input signals to each grid
    double grid1_voltage = -1.5 + input_signal * 0.1;   // Left input
    double grid2_voltage = -1.5 + input_signal2 * 0.1;  // Right input
    
    // Set the same plate voltage for both tubes
    tube1->SetGridVoltage(grid1_voltage);
    tube1->SetPlateVoltage(250.0);
    tube1->SetCathodeVoltage(0.0);
    
    tube2->SetGridVoltage(grid2_voltage);
    tube2->SetPlateVoltage(250.0);
    tube2->SetCathodeVoltage(0.0);
    
    // Calculate tube behaviors
    tube1->CalculateTubeBehavior();
    tube2->CalculateTubeBehavior();
    
    // Calculate differential output
    output_signal = CalculateDifferentialOutput();
}

double DifferentialAmp::CalculateDifferentialOutput() const {
    if (tubes.size() < 2) return 0.0;
    
    auto& tube1 = tubes[0];
    auto& tube2 = tubes[1];
    
    // Get plate currents from both tubes
    double i1 = tube1->GetPlateCurrent();
    double i2 = tube2->GetPlateCurrent();
    
    // Calculate plate voltages based on currents and load resistors
    double v1 = 250.0 - i1 * load_resistor;
    double v2 = 250.0 - i2 * load_resistor;
    
    // Differential output is the difference between plate voltages
    double diff_output = v1 - v2;
    
    // Scale appropriately - for simulation purposes
    return diff_output * 0.1;
}

void DifferentialAmp::ConfigureTubes() {
    topology_gain = 30.0;
    output_impedance = 62000.0;
    input_impedance = 1000000.0;
    
    // Calculate approximate differential gain and CMRR
    if (!tubes.empty()) {
        auto triode = dynamic_cast<Triode*>(tubes[0].get());
        if (triode) {
            double gm = triode->GetTransconductance();
            differential_gain = gm * load_resistor;  // Approximation
            cmrr = 20 * log10(differential_gain / 0.01);  // Simplified CMRR
        }
    }
}


// CascodeAmp implementation
CascodeAmp::CascodeAmp(double mu1, double rp1, double gm1, double mu2, double rp2, double gm2) 
    : TubeTopologyBase(TubeTopology::CASCODE)
    , plate_resistor(100000.0)  // 100kΩ typical
    , mu1(mu1), rp1(rp1), gm1(gm1)
    , mu2(mu2), rp2(rp2), gm2(gm2)
{
    tubes.clear();
    tubes.push_back(std::make_unique<Triode>(mu1, rp1, gm1));  // Lower triode
    tubes.push_back(std::make_unique<Triode>(mu2, rp2, gm2));  // Upper triode (shield)
    ConfigureTubes();
}

void CascodeAmp::ConfigureTubes() {
    // In cascode, the upper tube acts as a current buffer
    topology_gain = -mu1 * gm2 * plate_resistor;  // Approximate gain
    output_impedance = rp1 * rp2 / (rp1 + rp2);   // Very high output impedance
    input_impedance = 1000000.0;                  // High input impedance
}

void CascodeAmp::ProcessSignal() {
    if (tubes.size() < 2) {
        output_signal = input_signal * topology_gain;
        return;
    }
    
    auto& lower_tube = tubes[0];
    auto& upper_tube = tubes[1];
    
    // Input to lower tube's grid
    double lower_grid_voltage = -1.5 + input_signal * 0.1;
    lower_tube->SetGridVoltage(lower_grid_voltage);
    lower_tube->SetPlateVoltage(250.0);
    lower_tube->SetCathodeVoltage(0.0);
    
    // Upper tube's grid is biased at a fixed voltage (shield voltage)
    upper_tube->SetGridVoltage(50.0);  // Typical shield voltage
    upper_tube->SetPlateVoltage(250.0);
    upper_tube->SetCathodeVoltage(0.0);  // Connected to lower tube's plate
    
    // Calculate tube behaviors
    lower_tube->CalculateTubeBehavior();
    upper_tube->CalculateTubeBehavior();
    
    // For cascode, the output is taken from the upper tube's plate
    // The upper tube acts as a current buffer
    double lower_plate_current = lower_tube->GetPlateCurrent();
    double upper_plate_current = upper_tube->GetPlateCurrent();
    
    // The output is determined by the interaction of both tubes
    // Simplified calculation for simulation purposes
    output_signal = (lower_plate_current + upper_plate_current) * 0.5 * plate_resistor * 0.001;
    
    // Apply approximate cascode gain
    output_signal *= 0.8;  // Adjust gain to reasonable level
    
    // Ensure output is in range
    output_signal = std::max(-5.0, std::min(5.0, output_signal));
}

double CascodeAmp::GetTheoreticalGain() const {
    // For cascode: A ≈ -gm1 * Rp (much higher than common cathode)
    return -gm1 * plate_resistor;
}

double CascodeAmp::GetImprovedBandwidthFactor() const {
    // Cascode has higher bandwidth due to reduced Miller effect
    return 2.0;  // Approximate factor
}

double CascodeAmp::GetReducedNoiseFactor() const {
    // Cascode has reduced noise, particularly at low frequencies
    return 0.7;  // Approximate factor
}


// CurrentMirror implementation
CurrentMirror::CurrentMirror(double mu, double rp, double gm) 
    : TubeTopologyBase(TubeTopology::CURRENT_MIRROR)
    , reference_current(2.0e-3)  // 2mA typical reference
    , output_current(2.0e-3)     // Initial output current
{
    tubes.clear();
    tubes.push_back(std::make_unique<Triode>(mu, rp, gm));  // Reference triode
    tubes.push_back(std::make_unique<Triode>(mu, rp, gm));  // Output triode
    ConfigureTubes();
}

void CurrentMirror::ConfigureTubes() {
    topology_gain = 1.0;  // Current gain of 1:1
    output_impedance = 100000.0;  // High output impedance
    input_impedance = 1000000.0;  // High input impedance
}

void CurrentMirror::ProcessSignal() {
    if (tubes.size() < 2) {
        output_current = reference_current;
        output_signal = output_current * 1000.0;  // Convert current to voltage for output
        return;
    }
    
    auto& reference_tube = tubes[0];
    auto& output_tube = tubes[1];
    
    // For reference tube, we set conditions to generate the reference current
    reference_tube->SetGridVoltage(-3.0);  // More negative to reduce current
    reference_tube->SetPlateVoltage(250.0);
    reference_tube->SetCathodeVoltage(0.0);
    reference_tube->CalculateTubeBehavior();
    
    // Output tube should mirror the reference tube's current
    // The grid of the output tube is tied to the reference tube's grid
    output_tube->SetGridVoltage(-3.0);  // Same grid voltage as reference
    output_tube->SetPlateVoltage(250.0);
    output_tube->SetCathodeVoltage(0.0);
    output_tube->CalculateTubeBehavior();
    
    // The output current should match the reference current
    output_current = output_tube->GetPlateCurrent();
    
    // For demonstration, we'll also show the current as a voltage output
    output_signal = output_current * 1000.0;  // Scale for reasonable output voltage
}

void CurrentMirror::ConfigureTubes() {
    topology_gain = 1.0;
    output_impedance = 100000.0;
    input_impedance = 1000000.0;
}