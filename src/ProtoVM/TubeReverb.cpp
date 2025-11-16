#include "TubeReverb.h"
#include <algorithm>
#include <cmath>

// TubeReverb implementation
TubeReverb::TubeReverb(ReverbType type, SpringReverbConfig config) 
    : reverb_type(type)
    , spring_config(config)
    , driver_type(DriverType::SINGLE_ENDED)
    , input_signal(0.0)
    , dry_signal(0.0)
    , output_signal(0.0)
    , decay_time(2.0)  // 2 seconds default
    , pre_delay(0.0)   // No pre-delay by default
    , mix_level(0.5)   // 50% wet/dry mix
    , reverb_damping(0.3)    // Medium damping
    , reverb_diffusion(0.7)  // Medium diffusion
    , input_gain(1.0)
    , output_gain(1.0)
    , early_reflections_level(0.6)
    , late_reverb_level(0.4)
    , tube_driver_gain(30.0)  // Typical tube reverb driver gain
    , spring_tension(1.0)     // Normal tension
    , spring_length(1.0)      // Normal length
    , is_enabled(true)
    , write_index(0)
    , read_index(0)
    , damping_coefficient(0.7)
{
    delay_line.resize(DELAY_LINE_SIZE, 0.0);
    InitializeReverb();
}

TubeReverb::~TubeReverb() {
    // Cleanup handled by destructors
}

void TubeReverb::InitializeReverb() {
    // Initialize based on reverb type
    switch (reverb_type) {
        case ReverbType::SPRING:
            // Add tubes for spring reverb driver
            driver_tubes.push_back(std::make_unique<Triode>(100.0, 62000.0, 1.6e-3)); // Driver triode
            driver_tubes.push_back(std::make_unique<Triode>(100.0, 62000.0, 1.6e-3)); // Additional gain stage
            
            // Setup for spring reverb simulation
            decay_time = 2.5;  // Springs typically have shorter decay
            reverb_damping = 0.4;  // More damping for spring
            break;
            
        case ReverbType::PLATE:
            // Add tubes for plate reverb driver
            driver_tubes.push_back(std::make_unique<Triode>(100.0, 62000.0, 1.6e-3));
            
            // Setup for plate reverb simulation
            decay_time = 3.0;  // Plates typically have longer decay
            reverb_damping = 0.2;  // Less damping for plate
            break;
            
        case ReverbType::HALL:
            // Setup for hall reverb simulation
            decay_time = 4.0;  // Halls have longer decay
            break;
            
        case ReverbType::ROOM:
            // Setup for room reverb simulation
            decay_time = 1.5;  // Rooms have shorter decay
            break;
    }
    
    // Calculate damping coefficient based on damping parameter
    damping_coefficient = 0.5 + 0.4 * reverb_damping;
}

bool TubeReverb::Tick() {
    if (!is_enabled) {
        output_signal = input_signal;
        return true;
    }
    
    // Store dry signal for mixing later
    dry_signal = input_signal;
    
    // Apply input gain
    double processed_input = input_signal * input_gain;
    
    // Update the delay line with the processed input
    UpdateDelayLine(processed_input);
    
    // Process the reverb signal based on type
    ProcessReverbSignal();
    
    // Apply output gain
    output_signal *= output_gain;
    
    // Limit output to prevent clipping
    output_signal = std::max(-5.0, std::min(5.0, output_signal));
    
    // Tick all tubes used in the driver
    for (auto& tube : driver_tubes) {
        tube->Tick();
    }
    
    return true;
}

void TubeReverb::ProcessReverbSignal() {
    switch (reverb_type) {
        case ReverbType::SPRING:
            ProcessSpringReverb();
            break;
        case ReverbType::PLATE:
            ProcessPlateReverb();
            break;
        default:
            // For other reverb types, use a simplified algorithm
            double feedback = 0.7 * (1.0 - reverb_damping);  // Less feedback with more damping
            double input = GetFromDelayLine(static_cast<size_t>(decay_time * 44100 * 0.3));  // Early reflections
            double delayed = GetFromDelayLine(static_cast<size_t>(decay_time * 44100 * 0.7)); // Late reverb
            
            // Mix early reflections and late reverb
            output_signal = input * early_reflections_level + delayed * late_reverb_level * feedback;
            ApplyDamping(output_signal);
            break;
    }
}

void TubeReverb::ProcessSpringReverb() {
    // Spring reverb simulation
    // This is a simplified model of how spring reverb works
    
    // Calculate spring parameters based on configuration
    double spring_factor = spring_tension * spring_length;
    
    // Get signal from delay line representing spring reflections
    size_t spring_delay = static_cast<size_t>(0.02 * 44100 * spring_factor); // 20ms base delay
    spring_delay = std::min(spring_delay, static_cast<size_t>(44100 * 0.1)); // Max 100ms
    
    double early_refl = GetFromDelayLine(spring_delay * 1);      // First reflection
    double second_refl = GetFromDelayLine(spring_delay * 2);     // Second reflection  
    double third_refl = GetFromDelayLine(spring_delay * 3);      // Third reflection
    
    // Spring reverb has characteristic "boing" sound due to reflections
    double feedback = 0.6 * (1.0 - reverb_damping);  // Less feedback when more damped
    
    // Mix reflections to create spring character
    output_signal = early_refl * 0.6 + 
                   second_refl * 0.3 * feedback + 
                   third_refl * 0.1 * feedback * feedback;
    
    // Apply spring-specific filtering
    output_signal *= (0.7 + 0.3 * reverb_damping);  // High frequencies decay faster in springs
    ApplyDamping(output_signal);
    
    // Apply diffusion to smooth out the discrete echoes
    static double prev_output = 0.0;
    output_signal = output_signal * 0.7 + prev_output * 0.3;  // Simple smoothing
    prev_output = output_signal;
}

void TubeReverb::ProcessPlateReverb() {
    // Plate reverb simulation (simplified)
    // Real plate reverbs use a metal plate driven by transducers
    
    // Get multiple reflections from different points in the delay line
    double early_refl = GetFromDelayLine(static_cast<size_t>(0.005 * 44100));  // 5ms
    double mid_refl = GetFromDelayLine(static_cast<size_t>(0.030 * 44100));    // 30ms
    double late_refl = GetFromDelayLine(static_cast<size_t>(0.100 * 44100));   // 100ms
    
    // Plate reverb has denser, more even reflections than spring
    double feedback = 0.8 * (1.0 - reverb_damping * 0.5);  // Plate has more feedback
    
    output_signal = early_refl * 0.4 * early_reflections_level +
                   mid_refl * 0.3 + 
                   late_refl * 0.3 * late_reverb_level * feedback;
    
    // Apply plate-specific characteristics
    ApplyDamping(output_signal);
}

void TubeReverb::UpdateDelayLine(double input) {
    // Add pre-delay if specified
    size_t pre_delay_samples = static_cast<size_t>(pre_delay * 44100);
    
    // Add input to delay line with feedback
    double feedback_signal = GetFromDelayLine(static_cast<size_t>(decay_time * 44100 * 0.8));
    double final_input = input + feedback_signal * 0.7 * (1.0 - reverb_damping);
    
    // Apply diffusion to the input signal
    if (reverb_diffusion > 0.0) {
        // Simple allpass filter for diffusion
        static double delay_storage = 0.0;
        double temp = final_input + delay_storage * 0.5;
        final_input = delay_storage + final_input * (-0.5);
        delay_storage = temp;
    }
    
    delay_line[write_index] = final_input;
    
    // Update read index based on delay
    read_index = (write_index + DELAY_LINE_SIZE - pre_delay_samples) % DELAY_LINE_SIZE;
    
    // Advance write index
    write_index = (write_index + 1) % DELAY_LINE_SIZE;
}

double TubeReverb::GetFromDelayLine(size_t delay_samples) {
    if (delay_samples >= DELAY_LINE_SIZE) {
        delay_samples = DELAY_LINE_SIZE - 1;
    }
    
    size_t read_pos = (write_index + DELAY_LINE_SIZE - delay_samples) % DELAY_LINE_SIZE;
    return delay_line[read_pos];
}

void TubeReverb::ApplyDamping(double& signal) {
    // Apply simple damping to simulate high-frequency absorption
    static double prev_signal = 0.0;
    
    // Simple low-pass filter for damping
    double damped_signal = signal * damping_coefficient + prev_signal * (1.0 - damping_coefficient);
    signal = damped_signal;
    
    prev_signal = signal;
}


// FenderStyleReverb implementation
FenderStyleReverb::FenderStyleReverb() 
    : TubeReverb(ReverbType::SPRING, SpringReverbConfig::FENDER_2_SPRING) {
    driver_type = DriverType::SINGLE_ENDED;
    
    // Initialize with classic Fender spring reverb characteristics
    decay_time = 2.0;              // Classic Fender decay
    reverb_damping = 0.4;          // Medium damping
    early_reflections_level = 0.7; // Strong early reflections
    late_reverb_level = 0.3;       // Mellow late reverb
    spring_tension = 1.0;          // Standard tension
    spring_length = 1.0;           // Standard length
    
    // Use a classic tube configuration for the driver
    driver_tubes.clear();
    driver_tubes.push_back(std::make_unique<Triode>(100.0, 100000.0, 1.6e-3));  // High gain driver stage
    driver_tubes.push_back(std::make_unique<Triode>(50.0, 4700.0, 6.0e-3));     // Power stage for driving spring
}

void FenderStyleReverb::InitializeReverb() {
    // Set up classic Fender-style parameters
    decay_time = 2.0;
    reverb_damping = 0.4;
    mix_level = 0.3;   // Fender units typically have subtle reverb
    
    damping_coefficient = 0.6 + 0.3 * reverb_damping;
}

void FenderStyleReverb::ProcessReverbSignal() {
    // Fender-style spring reverb processing
    // This has characteristic "boing" and metallic sound
    
    // Get multiple reflections with Fender-style spacing
    size_t base_delay = static_cast<size_t>(0.018 * 44100);  // Fender-style base delay ~18ms
    
    double refl1 = GetFromDelayLine(base_delay * 1);
    double refl2 = GetFromDelayLine(base_delay * 2);
    double refl3 = GetFromDelayLine(base_delay * 3);
    double refl4 = GetFromDelayLine(base_delay * 4);
    
    // Fender springs have distinctive "tank" sound
    double feedback = 0.55;
    output_signal = refl1 * 0.5 +
                   refl2 * 0.3 * feedback +
                   refl3 * 0.15 * feedback * feedback +
                   refl4 * 0.05 * feedback * feedback * feedback;
    
    // Apply Fender-style tone
    ApplyDamping(output_signal);
    
    // Add subtle harmonic distortion characteristic of tube-driven springs
    if (abs(output_signal) > 0.8) {
        output_signal = output_signal > 0 ? 0.8 + 0.2 * log(output_signal / 0.8) : 
                                         -0.8 - 0.2 * log(-output_signal / 0.8);
    }
}


// TubePlateReverb implementation
TubePlateReverb::TubePlateReverb() 
    : TubeReverb(ReverbType::PLATE, SpringReverbConfig::FENDER_2_SPRING) {  // Spring config is ignored for plate
    driver_type = DriverType::PUSH_PULL;
    
    // Initialize with plate reverb characteristics
    decay_time = 3.0;              // Plates have longer decay
    reverb_damping = 0.2;          // Less damping than springs
    early_reflections_level = 0.4; // Softer early reflections
    late_reverb_level = 0.6;       // Rich late reverb
    plate_size = 1.0;              // Standard virtual plate size
    plate_material = 1.0;          // Standard virtual material
    
    // Use tube configuration optimized for plate simulation
    driver_tubes.clear();
    driver_tubes.push_back(std::make_unique<Triode>(100.0, 100000.0, 1.6e-3));  // Driver
    // Plate simulation doesn't need the physical driver, just the algorithm
    
    // Initialize a simple 2D grid for plate simulation
    plate_grid.resize(64, std::vector<double>(64, 0.0));
}

void TubePlateReverb::InitializeReverb() {
    // Set up plate reverb specific parameters
    decay_time = 3.5;
    reverb_damping = 0.2;
    mix_level = 0.4;  // Nice blend for plate reverb
    
    damping_coefficient = 0.7 + 0.2 * reverb_damping;
}

void TubePlateReverb::ProcessReverbSignal() {
    // Simplified plate reverb simulation using a 2D grid
    // In a real implementation, this would be much more complex
    
    // Get input for the plate simulation
    double input = GetFromDelayLine(static_cast<size_t>(0.001 * 44100));  // Very short delay
    
    // Update the plate grid simulation (simplified wave equation)
    // This is a very basic simulation for demonstration
    static int grid_pos = 0;
    static double plate_output = 0.0;
    
    // Add input to a position in the grid
    if (plate_grid.size() > grid_pos && plate_grid[0].size() > grid_pos) {
        plate_grid[grid_pos][grid_pos] += input * 0.5;
    }
    
    // Propagate the wave across the grid
    for (size_t i = 1; i < plate_grid.size() - 1; i++) {
        for (size_t j = 1; j < plate_grid[i].size() - 1; j++) {
            // Simple wave equation approximation
            double avg_neighbors = (plate_grid[i-1][j] + plate_grid[i+1][j] + 
                                   plate_grid[i][j-1] + plate_grid[i][j+1]) * 0.25;
            plate_grid[i][j] = avg_neighbors * 0.995 - plate_grid[i][j] * 0.005;  // Damping
        }
    }
    
    // Get output from a different position in the grid
    size_t output_pos = (grid_pos + 32) % 60;  // Different position for output
    if (plate_grid.size() > output_pos && plate_grid[0].size() > output_pos) {
        plate_output = plate_grid[output_pos][output_pos];
    }
    
    // Update position for next iteration
    grid_pos = (grid_pos + 1) % 60;
    
    // Apply feedback and plate characteristics
    double feedback = 0.75 * (1.0 - reverb_damping * 0.3);
    output_signal = plate_output * 0.5 + input * 0.2 * feedback;
    
    // Apply plate-specific damping
    ApplyDamping(output_signal);
    
    // Add subtle tube harmonic richness
    double harmonic_content = 0.05 * output_signal * output_signal * (output_signal > 0 ? 1 : -1);
    output_signal += harmonic_content;
}