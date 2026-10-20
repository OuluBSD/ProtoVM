#ifndef _ProtoVMCLI_CircuitSerializer_h_
#define _ProtoVMCLI_CircuitSerializer_h_

// Headless port of the wxWidgets CircuitSerializer: same on-disk format,
// but takes std::string paths and drops the CircuitCanvas conversions
// (no GUI in the CLI).

#include "CircuitData.h"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

class CircuitSerializer
{
public:
    // Serialize the circuit to a file
    static bool SaveCircuit(const CircuitData& circuitData, const std::string& filepath);

    // Deserialize a circuit from a file
    static bool LoadCircuit(const std::string& filepath, CircuitData& circuitData);

private:
    static std::vector<std::string> SplitTokens(const std::string& line);

    // Helper functions for serialization
    static std::string EscapeString(const std::string& str);
    static std::string UnescapeString(const std::string& str);
};

#endif // _ProtoVMCLI_CircuitSerializer_h_
