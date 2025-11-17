#include "CircuitSerializer.h"
#include <wx/wx.h>
#include <wx/tokenzr.h>

bool CircuitSerializer::SaveCircuit(const CircuitData& circuitData, const wxString& filepath)
{
    std::ofstream file(filepath.ToStdString());
    if (!file.is_open())
        return false;

    // Write header
    file << "# ProtoVM Circuit File\n";
    file << "name=" << circuitData.name << "\n";
    file << "description=" << circuitData.description << "\n";
    file << "\n";

    // Write components
    file << "# Components (" << circuitData.components.size() << ")\n";
    for (size_t i = 0; i < circuitData.components.size(); ++i)
    {
        const auto& comp = circuitData.components[i];
        file << "component " << i << " " << comp.type << " " << comp.name 
             << " " << comp.x << " " << comp.y << "\n";
        
        // Write input pins for this component
        for (const auto& input : comp.inputs)
        {
            file << "  input " << input.name << " " << input.x << " " << input.y << "\n";
        }
        
        // Write output pins for this component
        for (const auto& output : comp.outputs)
        {
            file << "  output " << output.name << " " << output.x << " " << output.y << "\n";
        }
    }
    file << "\n";

    // Write wires
    file << "# Wires (" << circuitData.wires.size() << ")\n";
    for (const auto& wire : circuitData.wires)
    {
        file << "wire " << wire.start_component_id << " " << wire.start_pin_name
             << " " << wire.end_component_id << " " << wire.end_pin_name << "\n";
    }

    file.close();
    return true;
}

bool CircuitSerializer::LoadCircuit(const wxString& filepath, CircuitData& circuitData)
{
    std::ifstream file(filepath.ToStdString());
    if (!file.is_open())
        return false;

    std::string line;
    std::getline(file, line); // Skip header comment

    // Read name
    std::getline(file, line);
    size_t pos = line.find('=');
    if (pos != std::string::npos)
        circuitData.name = line.substr(pos + 1);

    // Read description
    std::getline(file, line);
    pos = line.find('=');
    if (pos != std::string::npos)
        circuitData.description = line.substr(pos + 1);

    // Skip empty line
    std::getline(file, line);

    // Read components section header
    std::getline(file, line);

    // Read components
    while (std::getline(file, line) && !line.empty() && line[0] != '#')
    {
        if (line.substr(0, 9) == "component")
        {
            // Parse component: "component id type name x y"
            wxArrayString tokens = wxSplit(line, ' ');
            if (tokens.GetCount() >= 6)
            {
                ComponentData comp;
                comp.type = tokens[2].ToStdString();
                comp.name = tokens[3].ToStdString();
                comp.x = wxAtoi(tokens[4]);
                comp.y = wxAtoi(tokens[5]);
                
                // Read input and output pins for this component
                while (file.peek() != '#' && !file.eof())
                {
                    std::getline(file, line);
                    if (line.empty()) break;
                    
                    if (line[0] == ' ')  // Indented line - pin definition
                    {
                        wxArrayString pinTokens = wxSplit(line, ' ');
                        if (pinTokens.GetCount() >= 4)
                        {
                            PinData pin;
                            std::string pinType = pinTokens[1].ToStdString();
                            pin.name = pinTokens[2].ToStdString();
                            pin.x = wxAtoi(pinTokens[3]);
                            pin.y = wxAtoi(pinTokens[4]);
                            pin.is_input = (pinType == "input");
                            
                            if (pin.is_input)
                                comp.inputs.push_back(pin);
                            else
                                comp.outputs.push_back(pin);
                        }
                    }
                    else  // Not an indented line - break to process next component or wire
                    {
                        break;
                    }
                }
                
                circuitData.components.push_back(comp);
                
                // If the line we just read was not a pin, we need to process it again
                if (!line.empty() && line[0] != ' ')
                    continue;  // Need to reprocess this line
            }
        }
    }

    // If the last line was not a pin definition, process it as the start of wires
    if (!line.empty() && line[0] == '#')
    {
        // Read wires
        while (std::getline(file, line) && !line.empty())
        {
            if (line.substr(0, 4) == "wire")
            {
                // Parse wire: "wire start_id start_pin end_id end_pin"
                wxArrayString tokens = wxSplit(line, ' ');
                if (tokens.GetCount() >= 5)
                {
                    WireData wire;
                    wire.start_component_id = wxAtoi(tokens[1]);
                    wire.start_pin_name = tokens[2].ToStdString();
                    wire.end_component_id = wxAtoi(tokens[3]);
                    wire.end_pin_name = tokens[4].ToStdString();
                    
                    circuitData.wires.push_back(wire);
                }
            }
        }
    }

    file.close();
    return true;
}

CircuitData CircuitSerializer::CanvasToData(CircuitCanvas* canvas)
{
    CircuitData circuitData;
    circuitData.name = "Untitled Circuit";
    circuitData.description = "A digital logic circuit";
    
    // In a more complete implementation, we would convert actual canvas components to data
    // For now, we'll just return an empty circuit
    // This is a simplified approach - we'd need to properly map the GUI components to data
    
    return circuitData;
}

void CircuitSerializer::DataToCanvas(const CircuitData& circuitData, CircuitCanvas* canvas)
{
    // This would convert circuit data back to GUI components
    // For now, we'll just return
    // In a complete implementation, we would create actual GUI components from the data
}