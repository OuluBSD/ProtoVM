#include "CircuitData.h"
#include <atomic>
#include <sstream>

namespace CircuitIdGenerator {

    static std::atomic<int> component_counter{1};
    static std::atomic<int> wire_counter{1};
    static std::atomic<int> pin_counter{1};

    CircuitEntityId GenerateComponentId() {
        int id = component_counter.fetch_add(1);
        std::stringstream ss;
        ss << "C" << std::setw(7) << std::setfill('0') << id;
        return CircuitEntityId(ss.str());
    }

    CircuitEntityId GenerateWireId() {
        int id = wire_counter.fetch_add(1);
        std::stringstream ss;
        ss << "W" << std::setw(7) << std::setfill('0') << id;
        return CircuitEntityId(ss.str());
    }

    CircuitEntityId GeneratePinId() {
        int id = pin_counter.fetch_add(1);
        std::stringstream ss;
        ss << "P" << std::setw(7) << std::setfill('0') << id;
        return CircuitEntityId(ss.str());
    }

} // namespace CircuitIdGenerator