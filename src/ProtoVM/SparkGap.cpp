#include "SparkGap.h"

// SparkGap Implementation
SparkGap::SparkGap(double breakdown_voltage, bool is_arced_initially) 
    : breakdown_voltage(breakdown_voltage < MIN_BREAKDOWN_VOLTAGE ? MIN_BREAKDOWN_VOLTAGE : breakdown_voltage),
      arced(is_arced_initially),
      arc_duration_ticks(DEFAULT_ARC_DURATION),
      current_arc_ticks(0),
      triggered(false) {
    // A spark gap has two terminals
    AddBidirectional("A");
    AddBidirectional("B");
    analog_values.resize(2, 0.0);  // Initialize both terminals to 0V
}

bool SparkGap::Tick() {
    double voltage_a = GetAnalogValue(0);  // Voltage at terminal A
    double voltage_b = GetAnalogValue(1);  // Voltage at terminal B
    double voltage_diff = std::abs(voltage_a - voltage_b);  // Voltage across the gap
    
    // Check if voltage across the gap exceeds breakdown voltage
    if (voltage_diff >= breakdown_voltage || triggered) {
        if (!arced) {
            // Start the arc
            arced = true;
            current_arc_ticks = 0;
            triggered = false;  // Reset the trigger flag
        }
    }
    
    // If arced, continue for the specified duration
    if (arced) {
        current_arc_ticks++;
        if (current_arc_ticks >= arc_duration_ticks) {
            arced = false;  // End the arc after duration
        }
    }
    
    // Update output based on arcing state
    if (arced) {
        // When arced, the two terminals are connected with low resistance
        // Average the voltages at both ends to simulate connection
        double avg_voltage = (voltage_a + voltage_b) / 2.0;
        UpdateAnalogValue(0, avg_voltage);
        UpdateAnalogValue(1, avg_voltage);
    } else {
        // When not arced, terminals are isolated (high resistance)
        // Pass through the voltages unchanged, but with some leakage
        UpdateAnalogValue(0, voltage_a * 0.999); // Small leakage
        UpdateAnalogValue(1, voltage_b * 0.999); // Small leakage
    }
    
    AnalogNodeBase::Tick();  // Call parent tick
    return true;
}

void SparkGap::SetBreakdownVoltage(double voltage) {
    breakdown_voltage = voltage < MIN_BREAKDOWN_VOLTAGE ? MIN_BREAKDOWN_VOLTAGE : voltage;
}

void SparkGap::TriggerArc() {
    triggered = true;  // Manually trigger the arc
}

void SparkGap::Reset() {
    arced = false;  // Manually reset the arc state
    current_arc_ticks = 0;  // Reset the duration counter
    triggered = false;  // Reset trigger flag
}