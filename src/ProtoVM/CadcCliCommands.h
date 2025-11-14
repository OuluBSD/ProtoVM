#ifndef ProtoVM_CadcCliCommands_h
#define ProtoVM_CadcCliCommands_h

#include "Cli.h"
#include "CadcSystem.h"

/*
 * CADC-specific CLI commands and debugging tools
 */

class CadcCliCommands : public CliCommands {
public:
    CadcCliCommands(CadcSystem* cadc) : cadc_system(cadc) {}
    
    virtual void RegisterCommands(Cli& cli) override {
        // Register CADC-specific commands
        cli.RegisterCommand("cadc-status", [this](const Vector<String>& args) { return CmdCadcStatus(args); });
        cli.RegisterCommand("cadc-polynomial", [this](const Vector<String>& args) { return CmdCadcPolynomial(args); });
        cli.RegisterCommand("cadc-airdata", [this](const Vector<String>& args) { return CmdCadcAirData(args); });
        cli.RegisterCommand("cadc-modules", [this](const Vector<String>& args) { return CmdCadcModules(args); });
        cli.RegisterCommand("cadc-microcode", [this](const Vector<String>& args) { return CmdCadcMicrocode(args); });
    }

private:
    CadcSystem* cadc_system;

    // Command implementations
    String CmdCadcStatus(const Vector<String>& args);
    String CmdCadcPolynomial(const Vector<String>& args);
    String CmdCadcAirData(const Vector<String>& args);
    String CmdCadcModules(const Vector<String>& args);
    String CmdCadcMicrocode(const Vector<String>& args);
};

// Implementation of command methods
String CadcCliCommands::CmdCadcStatus(const Vector<String>& args) {
    if (!cadc_system) return "Error: CADC system not available";

    String result = "CADC System Status:\n";
    result += Format("System Cycle: %d\n", cadc_system->system_cycle);
    result += Format("Bit Time: %d\n", cadc_system->bit_time);
    result += Format("Word Time: %d\n", cadc_system->word_time);
    result += Format("Operation Time: %d\n", cadc_system->operation_time);
    result += Format("Frame Mark: %s\n", cadc_system->frame_mark ? "YES" : "NO");
    result += Format("Word Mark: %s\n", cadc_system->word_mark ? "YES" : "NO");
    result += Format("Is Running: %s\n", cadc_system->is_running ? "YES" : "NO");
    result += Format("Is Busy: %s\n", cadc_system->is_busy ? "YES" : "NO");

    return result;
}

String CadcCliCommands::CmdCadcPolynomial(const Vector<String>& args) {
    if (!cadc_system) return "Error: CADC system not available";
    
    if (args.GetCount() < 2) {
        return "Usage: cadc-polynomial <x_value> <coefficients...>\nExample: cadc-polynomial 10 1 2 3 (for 1 + 2*x + 3*x^2)";
    }

    try {
        int20 x = StrInt(args[1]);
        
        // Parse coefficients
        Vector<int20> coeffs;
        for (int i = 2; i < args.GetCount(); i++) {
            coeffs.Add(StrInt(args[i]));
        }

        // Evaluate the polynomial
        int20 result = cadc_system->EvaluatePolynomial(x, coeffs.Begin(), coeffs.GetCount() - 1);
        
        String output = "Polynomial Evaluation:\n";
        output += Format("x = %d\n", x);
        output += "Coefficients: ";
        for (int i = 0; i < coeffs.GetCount(); i++) {
            if (i > 0) output += ", ";
            output += Format("%d", coeffs[i]);
        }
        output += Format("\nResult = %d\n", result);
        
        return output;
    } catch (...) {
        return "Error: Invalid number format";
    }
}

String CadcCliCommands::CmdCadcAirData(const Vector<String>& args) {
    if (!cadc_system) return "Error: CADC system not available";
    
    String result = "CADC Air Data Computations:\n";
    
    // For demonstration, we'll use some example values
    int20 static_pressure = 0x20000;  // Example static pressure
    int20 temperature = 0x18000;      // Example temperature
    int20 impact_pressure = 0x21000;  // Example impact pressure
    
    int20 altitude = cadc_system->ComputeAltitude(static_pressure, temperature);
    int20 vertical_speed = cadc_system->ComputeVerticalSpeed(cadc_system->prev_altitude, altitude);
    int20 air_speed = cadc_system->ComputeAirSpeed(impact_pressure, static_pressure);
    int20 mach_number = cadc_system->ComputeMachNumber(air_speed, temperature);
    
    result += Format("Altitude: %d\n", altitude);
    result += Format("Vertical Speed: %d\n", vertical_speed);
    result += Format("Air Speed: %d\n", air_speed);
    result += Format("Mach Number: %d\n", mach_number);
    
    return result;
}

String CadcCliCommands::CmdCadcModules(const Vector<String>& args) {
    if (!cadc_system) return "Error: CADC system not available";

    String result = "CADC Module Information:\n";
    
    auto mul_module = cadc_system->GetMultiplyModule();
    auto div_module = cadc_system->GetDivideModule();
    auto slf_module = cadc_system->GetSpecialLogicModule();
    
    result += "Multiply Module (PMU): ";
    result += mul_module ? "Present\n" : "Not available\n";
    
    result += "Divide Module (PDU): ";
    result += div_module ? "Present\n" : "Not available\n";
    
    result += "Special Logic Function Module (SLF): ";
    result += slf_module ? "Present\n" : "Not available\n";
    
    return result;
}

String CadcCliCommands::CmdCadcMicrocode(const Vector<String>& args) {
    if (!cadc_system) return "Error: CADC system not available";
    
    String result = "CADC Microcode Information:\n";
    result += "The F-14 CADC uses specialized microcode stored in ROMs\n";
    result += "to execute polynomial evaluations and air data computations.\n";
    result += "This includes algorithms for:\n";
    result += "- Altitude computation from pressure inputs\n";
    result += "- Airspeed computation from impact/static pressure\n";
    result += "- Mach number computation from airspeed and temperature\n";
    result += "- Vertical speed computation from altitude changes\n";
    result += "- Data limiting and transfer functions\n";
    
    return result;
}

#endif