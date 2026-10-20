#ifndef _ProtoVM_MachineSnapshot_h_
#define _ProtoVM_MachineSnapshot_h_

#include <ProtoVM/Machine.h>  // real engine Machine (global)
#include <memory>
#include <string>
#include <vector>

namespace ProtoVMCLI {

class MachineSnapshot {
public:
    // Serialize the entire machine state to a binary file
    static bool SerializeToFile(const Machine& machine, const std::string& file_path);
    
    // Deserialize the entire machine state from a binary file
    static bool DeserializeFromFile(Machine& machine, const std::string& file_path);
    
    // Serialize the machine state to a binary buffer
    static bool SerializeToBuffer(const Machine& machine, std::string& buffer);
    
    // Deserialize the machine state from a binary buffer
    static bool DeserializeFromBuffer(Machine& machine, const std::string& buffer);
    
private:
    // Mirror record for Machine::SignalTrace; the engine type is private to
    // Machine, so this copyable record is what snapshot files serialize.
    struct SignalTraceSnapshot {
        Upp::String pin_name;
        Upp::byte last_value = 0;
        bool trace_enabled = true;
        std::vector<Upp::byte> value_history;
        std::vector<int> tick_history;
    };

    // Helper functions for serializing different parts of the Machine
    static bool SerializePcbs(const Array<Pcb>& pcbs, std::ostream& os);
    static bool DeserializePcbs(Array<Pcb>& pcbs, std::istream& is);

    static bool SerializeLinks(const LinkBaseMap& links, std::ostream& os);
    static bool DeserializeLinks(LinkBaseMap& links, std::istream& is);

    static bool SerializeClockDomains(const Vector<Machine::ClockDomain>& domains, std::ostream& os);
    static bool DeserializeClockDomains(Vector<Machine::ClockDomain>& domains, std::istream& is);

    static bool SerializeSignalTraces(const Machine& machine, std::ostream& os);
    static bool DeserializeSignalTraces(std::vector<SignalTraceSnapshot>& traces, std::istream& is);

    static bool SerializeSignalTransitions(const Vector<Machine::SignalTransition>& transitions, std::ostream& os);
    static bool DeserializeSignalTransitions(Vector<Machine::SignalTransition>& transitions, std::istream& is);
};

} // namespace ProtoVMCLI

#endif