#ifndef CIRCUITSERIALIZER_H
#define CIRCUITSERIALIZER_H

#include "CircuitData.h"
#include "CircuitCanvas.h"
#include <fstream>
#include <sstream>

class CircuitSerializer
{
public:
    // Serialize the circuit canvas to a file
    static bool SaveCircuit(const CircuitData& circuitData, const wxString& filepath);
    
    // Deserialize the circuit from a file
    static bool LoadCircuit(const wxString& filepath, CircuitData& circuitData);
    
    // Convert CircuitCanvas to CircuitData
    static CircuitData CanvasToData(CircuitCanvas* canvas);
    
    // Convert CircuitData to CircuitCanvas
    static void DataToCanvas(const CircuitData& circuitData, CircuitCanvas* canvas);

private:
    // Helper functions for serialization
    static std::string EscapeString(const std::string& str);
    static std::string UnescapeString(const std::string& str);
};

#endif // CIRCUITSERIALIZER_H