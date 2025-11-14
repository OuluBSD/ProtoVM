#include "Fuse.h"

// Fuse Implementation
Fuse::Fuse(double current_rating, bool is_blown_initially) 
    : current_rating(current_rating < MIN_CURRENT_RATING ? MIN_CURRENT_RATING : current_rating),
      blown(is_blown_initially),
      last_current(0.0),
      blow_time_constant(0.1),  // 100ms time constant by default
      heat_accumulation(0.0),
      cooling_factor(0.9) {
    // A fuse has two terminals
    AddBidirectional("A");
    AddBidirectional("B");
    analog_values.resize(2, 0.0);  // Initialize both terminals to 0V
}

bool Fuse::Tick() {
    double voltage_a = GetAnalogValue(0);  // Voltage at terminal A
    double voltage_b = GetAnalogValue(1);  // Voltage at terminal B
    double voltage_diff = voltage_a - voltage_b;  // Voltage across the fuse
    
    // Calculate current through the fuse using Ohm's law (when not blown)
    double current = 0.0;
    
    if (!blown) {
        // In a real circuit, the current would be determined by the external circuit
        // For simulation, we'll make an approximate calculation based on voltage difference
        // and assume some resistance when the fuse is intact
        double fuse_resistance = 0.1; // Low resistance when normal (100mΩ)
        current = voltage_diff / fuse_resistance;
        
        // Calculate heat generation based on I^2*R (approximately)
        double heat_generation = (current * current) * HEAT_BUILD_RATE * SIMULATION_TIMESTEP;
        heat_accumulation += heat_generation;
        
        // Apply cooling effect
        heat_accumulation *= (1.0 - HEAT_DISSIPATION_RATE * SIMULATION_TIMESTEP);
        
        // Check if the accumulated heat is enough to blow the fuse
        // This is simplified: a real fuse would have more complex thermal characteristics
        if (std::abs(current) > current_rating * BLOW_CURRENT_MULTIPLIER) {
            // If current is significantly above rating, blow immediately
            blown = true;
        } else if (std::abs(current) > current_rating && heat_accumulation >= BLOW_THRESHOLD) {
            // If current is above rating and enough heat has accumulated, blow the fuse
            blown = true;
        }
        
        // Prevent heat from going negative
        if (heat_accumulation < 0) heat_accumulation = 0;
    }
    
    // Update output based on fuse state
    if (blown) {
        // When blown, the fuse becomes an open circuit
        // High resistance means almost no current flows
        // The terminals become isolated
        UpdateAnalogValue(0, voltage_a);
        UpdateAnalogValue(1, voltage_b);
    } else {
        // When normal, the fuse acts as a low-resistance connection
        // Pass through voltage with minimal drop
        UpdateAnalogValue(0, voltage_a);
        UpdateAnalogValue(1, voltage_b);
    }
    
    last_current = current;
    
    AnalogNodeBase::Tick();  // Call parent tick
    return true;
}

void Fuse::SetCurrentRating(double rating) {
    current_rating = rating < MIN_CURRENT_RATING ? MIN_CURRENT_RATING : rating;
}

void Fuse::Blow() {
    blown = true;
    heat_accumulation = BLOW_THRESHOLD;  // Set to blown state
}

void Fuse::Reset() {
    blown = false;
    heat_accumulation = 0.0;  // Reset heat accumulation
}