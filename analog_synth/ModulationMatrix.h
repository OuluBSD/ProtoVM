#ifndef MODULATION_MATRIX_H
#define MODULATION_MATRIX_H

#include <vector>
#include <string>
#include <functional>
#include <map>
#include <memory>

// Forward declarations
class Oscillator;
class ADSR;
class LFO;
class Filter;

// Modulation sources
enum class ModulationSource {
    LFO1,
    LFO2,
    ADSR1,
    ADSR2,
    VELOCITY,
    KEYBOARD_TRACKING,
    MIDI_CC,
    ENVELOPE_FOLLOWER,
    NOISE
};

// Modulation destinations
enum class ModulationDestination {
    OSC_FREQUENCY,
    OSC_WAVEFORM,
    FILTER_CUTOFF,
    FILTER_RESONANCE,
    VCA_LEVEL,
    LFO_RATE,
    ADSR_ATTACK,
    ADSR_DECAY,
    ADSR_SUSTAIN,
    ADSR_RELEASE
};

struct ModulationConnection {
    ModulationSource source;
    ModulationDestination destination;
    double amount;  // Modulation amount (-1.0 to 1.0)
    
    ModulationConnection(ModulationSource s, ModulationDestination d, double a) 
        : source(s), destination(d), amount(a) {}
};

class ModulationMatrix {
public:
    ModulationMatrix();
    ~ModulationMatrix();

    // Add a modulation connection
    void addConnection(ModulationSource source, ModulationDestination destination, double amount);
    
    // Remove a modulation connection
    void removeConnection(ModulationSource source, ModulationDestination destination);
    
    // Clear all connections
    void clearAllConnections();
    
    // Process modulation for all connections
    void processModulation();
    
    // Get modulation value for a specific destination
    double getModulationValue(ModulationDestination destination);
    
    // Set current values for sources
    void setCurrentLFO1Value(double value);
    void setCurrentLFO2Value(double value);
    void setCurrentADSR1Value(double value);
    void setCurrentADSR2Value(double value);
    void setCurrentVelocity(double value);
    void setCurrentNote(double note);
    void setCurrentMidiCC(double value);
    void setCurrentEnvelopeFollower(double value);
    void setCurrentNoise(double value);
    
    // Get number of active connections
    size_t getConnectionCount() const { return connections.size(); }
    
    // Get all connections for preset system
    const std::vector<ModulationConnection>& getAllConnections() const { return connections; }
    
private:
    std::vector<ModulationConnection> connections;
    
    // Current values for modulation sources
    struct SourceValues {
        double lfo1 = 0.0;
        double lfo2 = 0.0;
        double adsr1 = 0.0;
        double adsr2 = 0.0;
        double velocity = 0.0;
        double keyboardTracking = 0.0;  // Based on current note
        double midiCC = 0.0;
        double envelopeFollower = 0.0;
        double noise = 0.0;
    } currentSourceValues;
    
    // Cached modulation values for each destination
    std::map<ModulationDestination, double> cachedValues;
};

#endif // MODULATION_MATRIX_H