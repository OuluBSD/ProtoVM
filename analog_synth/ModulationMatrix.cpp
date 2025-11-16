#include "ModulationMatrix.h"
#include <cmath>

ModulationMatrix::ModulationMatrix() {
    // Initialize cached values to zero
    for (int i = 0; i < static_cast<int>(ModulationDestination::ADSR_RELEASE) + 1; i++) {
        cachedValues[static_cast<ModulationDestination>(i)] = 0.0;
    }
}

ModulationMatrix::~ModulationMatrix() {
    // Clean up if needed
}

void ModulationMatrix::addConnection(ModulationSource source, ModulationDestination destination, double amount) {
    // Check if a connection already exists and update it
    for (auto& conn : connections) {
        if (conn.source == source && conn.destination == destination) {
            conn.amount = amount;
            return;
        }
    }
    
    // If no existing connection, add a new one
    connections.emplace_back(source, destination, amount);
}

void ModulationMatrix::removeConnection(ModulationSource source, ModulationDestination destination) {
    connections.erase(
        std::remove_if(connections.begin(), connections.end(),
            [source, destination](const ModulationConnection& conn) {
                return conn.source == source && conn.destination == destination;
            }),
        connections.end()
    );
}

void ModulationMatrix::clearAllConnections() {
    connections.clear();
}

void ModulationMatrix::processModulation() {
    // Initialize all cached values to zero
    for (auto& pair : cachedValues) {
        pair.second = 0.0;
    }
    
    // Calculate keyboard tracking based on current note
    // Assuming MIDI note values where C4 = 60
    currentSourceValues.keyboardTracking = (currentSourceValues.currentNote - 60.0) / 60.0; // Normalize to -1 to 1 range around middle C
    
    // Process each connection and accumulate modulation values
    for (const auto& conn : connections) {
        double sourceValue = 0.0;
        
        // Get the source value based on the source type
        switch (conn.source) {
            case ModulationSource::LFO1:
                sourceValue = currentSourceValues.lfo1;
                break;
            case ModulationSource::LFO2:
                sourceValue = currentSourceValues.lfo2;
                break;
            case ModulationSource::ADSR1:
                sourceValue = currentSourceValues.adsr1;
                break;
            case ModulationSource::ADSR2:
                sourceValue = currentSourceValues.adsr2;
                break;
            case ModulationSource::VELOCITY:
                sourceValue = (currentSourceValues.velocity / 127.0) * 2.0 - 1.0; // Convert MIDI velocity (0-127) to -1 to 1 range
                break;
            case ModulationSource::KEYBOARD_TRACKING:
                sourceValue = currentSourceValues.keyboardTracking;
                break;
            case ModulationSource::MIDI_CC:
                sourceValue = (currentSourceValues.midiCC / 127.0) * 2.0 - 1.0; // Convert MIDI CC (0-127) to -1 to 1 range
                break;
            case ModulationSource::ENVELOPE_FOLLOWER:
                sourceValue = currentSourceValues.envelopeFollower;
                break;
            case ModulationSource::NOISE:
                sourceValue = currentSourceValues.noise;
                break;
        }
        
        // Calculate the modulation contribution
        double modulationAmount = sourceValue * conn.amount;
        
        // Add to the cached value for this destination
        cachedValues[conn.destination] += modulationAmount;
    }
    
    // Clamp values if needed (for some parameters)
    for (auto& pair : cachedValues) {
        // For parameters like cutoff frequency, we might want to ensure positive values
        if (pair.first == ModulationDestination::FILTER_CUTOFF || 
            pair.first == ModulationDestination::FILTER_RESONANCE) {
            // Ensure the result stays positive
            pair.second = std::max(0.0, pair.second);
        }
    }
}

double ModulationMatrix::getModulationValue(ModulationDestination destination) {
    return cachedValues[destination];
}

void ModulationMatrix::setCurrentLFO1Value(double value) {
    currentSourceValues.lfo1 = value;
}

void ModulationMatrix::setCurrentLFO2Value(double value) {
    currentSourceValues.lfo2 = value;
}

void ModulationMatrix::setCurrentADSR1Value(double value) {
    currentSourceValues.adsr1 = value;
}

void ModulationMatrix::setCurrentADSR2Value(double value) {
    currentSourceValues.adsr2 = value;
}

void ModulationMatrix::setCurrentVelocity(double value) {
    currentSourceValues.velocity = value;
}

void ModulationMatrix::setCurrentNote(double note) {
    currentSourceValues.currentNote = note;
}

void ModulationMatrix::setCurrentMidiCC(double value) {
    currentSourceValues.midiCC = value;
}

void ModulationMatrix::setCurrentEnvelopeFollower(double value) {
    currentSourceValues.envelopeFollower = value;
}

void ModulationMatrix::setCurrentNoise(double value) {
    currentSourceValues.noise = value;
}