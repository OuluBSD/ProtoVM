#ifndef _ProtoVM_Fuse_h_
#define _ProtoVM_Fuse_h_

#include "AnalogCommon.h"

// Fuse component with current rating and blow characteristics
class Fuse : public AnalogNodeBase {
public:
    typedef Fuse CLASSNAME;

    // Constructor: current_rating in Amps, blown state initially (false=normal, true=blown)
    Fuse(double current_rating = 1.0, bool is_blown = false);
    virtual ~Fuse() {}

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "Fuse"; }

    void SetCurrentRating(double rating);
    double GetCurrentRating() const { return current_rating; }
    
    // Check if currently blown (open circuit)
    bool IsBlown() const { return blown; }
    
    // Blow the fuse manually
    void Blow();
    
    // Reset the fuse to normal state
    void Reset();

private:
    double current_rating;      // Current rating that causes fuse to blow (Amperes)
    bool blown;                // Whether the fuse is blown (open circuit)
    double last_current;       // Current through the fuse in the previous simulation step
    
    // Blow behavior parameters
    double blow_time_constant;  // Time constant for blow behavior (in seconds)
    double heat_accumulation;   // Accumulated heat that causes the fuse to blow
    double cooling_factor;      // How quickly the fuse cools down
    
    static constexpr double MIN_CURRENT_RATING = 0.001;  // 1mA minimum
    static constexpr double BLOW_CURRENT_MULTIPLIER = 2.0;  // Current multiplier to blow quickly
    static constexpr double HEAT_BUILD_RATE = 1.0;       // Rate of heat accumulation
    static constexpr double HEAT_DISSIPATION_RATE = 0.1;  // Rate of heat dissipation
    static constexpr double BLOW_THRESHOLD = 1.0;        // Heat level required to blow the fuse
};

#endif