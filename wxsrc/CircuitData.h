#ifndef CIRCUITDATA_H
#define CIRCUITDATA_H

#include <vector>
#include <string>

// Structure to represent an ID for a circuit entity
struct CircuitEntityId {
    std::string id;  // Format: "C<nnnnnnn>" for components, "W<nnnnnnn>" for wires/pins

    CircuitEntityId() : id("") {}
    explicit CircuitEntityId(const std::string& _id) : id(_id) {}

    bool IsValid() const { return !id.empty(); }
    bool operator==(const CircuitEntityId& other) const { return id == other.id; }
    bool operator!=(const CircuitEntityId& other) const { return id != other.id; }
};

// Structure to represent a pin
struct PinData {
    CircuitEntityId id;
    std::string name;
    bool is_input;
    int x, y;

    PinData() : is_input(false), x(0), y(0) {}
    PinData(const CircuitEntityId& _id, const std::string& _name, bool _is_input, int _x, int _y)
        : id(_id), name(_name), is_input(_is_input), x(_x), y(_y) {}
};

// Structure to represent a component
struct ComponentData {
    CircuitEntityId id;
    std::string type;  // "NAND", "NOR", "NOT", "BUF", etc.
    std::string name;
    int x, y;
    std::vector<PinData> inputs;
    std::vector<PinData> outputs;

    ComponentData() : x(0), y(0) {}
    ComponentData(const CircuitEntityId& _id, const std::string& _type, const std::string& _name, int _x, int _y)
        : id(_id), type(_type), name(_name), x(_x), y(_y) {}
};

// Structure to represent a wire connection
struct WireData {
    CircuitEntityId id;
    CircuitEntityId start_component_id;  // ID of the component
    std::string start_pin_name;  // Name of the pin
    CircuitEntityId end_component_id;    // ID of the component
    std::string end_pin_name;    // Name of the pin

    WireData() {}
    WireData(const CircuitEntityId& _id, const CircuitEntityId& _start_comp_id,
             const std::string& _start_pin, const CircuitEntityId& _end_comp_id,
             const std::string& _end_pin)
        : id(_id), start_component_id(_start_comp_id), start_pin_name(_start_pin),
          end_component_id(_end_comp_id), end_pin_name(_end_pin) {}
};

// Structure to represent an entire circuit
struct CircuitData {
    std::string name;
    std::string description;
    std::vector<ComponentData> components;
    std::vector<WireData> wires;

    CircuitData() {}
};

#endif // CIRCUITDATA_H