#ifndef _ProtoVM_MachineSnapshot_h_
#define _ProtoVM_MachineSnapshot_h_

#include "Machine.h"
#include <memory>
#include <string>

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
    // Helper functions for serializing different parts of the Machine
    static bool SerializePcbs(const Array<Pcb>& pcbs, std::ostream& os);
    static bool DeserializePcbs(Array<Pcb>& pcbs, std::istream& is);
    
    static bool SerializeLinks(const LinkBaseMap& links, std::ostream& os);
    static bool DeserializeLinks(LinkBaseMap& links, std::istream& is);
    
    static bool SerializeClockDomains(const Vector<Machine::ClockDomain>& domains, std::ostream& os);
    static bool DeserializeClockDomains(Vector<Machine::ClockDomain>& domains, std::istream& is);
    
    static bool SerializeSignalTraces(const Vector<Machine::SignalTrace>& traces, std::ostream& os);
    static bool DeserializeSignalTraces(Vector<Machine::SignalTrace>& traces, std::istream& is);
    
    static bool SerializeSignalTransitions(const Vector<Machine::SignalTransition>& transitions, std::ostream& os);
    static bool DeserializeSignalTransitions(Vector<Machine::SignalTransition>& transitions, std::istream& is);
};

} // namespace ProtoVMCLI

#endif