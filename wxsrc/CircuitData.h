#ifndef CIRCUITDATA_H
#define CIRCUITDATA_H

#include <vector>
#include <string>

// Structure to represent a pin
struct PinData {
    std::string name;
    bool is_input;
    int x, y;
};

// Structure to represent a component
struct ComponentData {
    std::string type;  // "NAND", "NOR", "NOT", "BUF", etc.
    std::string name;
    int x, y;
    std::vector<PinData> inputs;
    std::vector<PinData> outputs;
};

// Structure to represent a wire connection
struct WireData {
    int start_component_id;  // Index in the components array
    std::string start_pin_name;  // Name of the pin
    int end_component_id;    // Index in the components array
    std::string end_pin_name;    // Name of the pin
};

// Structure to represent an entire circuit
struct CircuitData {
    std::string name;
    std::string description;
    std::vector<ComponentData> components;
    std::vector<WireData> wires;
};

#endif // CIRCUITDATA_H