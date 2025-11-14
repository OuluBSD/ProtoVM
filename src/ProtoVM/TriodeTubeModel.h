#ifndef ProtoVM_TriodeTubeModel_h
#define ProtoVM_TriodeTubeModel_h

#include "Component.h"
#include "Common.h"
#include "AnalogCommon.h"
#include "AnalogSimulation.h"

/*
 * Triode Tube Model Implementation
 *
 * Implements a realistic model of a triode vacuum tube based on the basic physics
 * of electron flow between cathode, grid, and anode (plate).
 * 
 * The simplest model uses the Child-Langmuir law modified for triodes:
 * Ip = Kp * (Vg - mu*Vp)^1.5
 * 
 * For small signal analysis, transconductance gm and plate resistance rp are used.
 */

class TriodeTube : public AnalogNodeBase {
public:
    TriodeTube();
    virtual ~TriodeTube() {}

    bool Tick() override;
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;

    String GetClassName() const override { return "TriodeTube"; }

    // Set tube parameters
    void SetAmplificationFactor(double mu);
    void SetPlateResistance(double rp);  // in ohms
    void SetTransconductance(double gm);  // in siemens

    // Get current operating point
    double GetPlateCurrent() const { return plate_current; }
    double GetGridVoltage() const { return grid_voltage; }
    double GetPlateVoltage() const { return plate_voltage; }

private:
    // Tube parameters
    double amplification_factor;   // mu (amplification factor)
    double plate_resistance;       // rp (plate resistance in ohms)
    double transconductance;       // gm (transconductance in siemens)
    double max_plate_current;      // Maximum plate current for saturation modeling

    // Operating point
    double plate_current;
    double grid_voltage;
    double plate_voltage;
    double cathode_voltage;

    // Pin connections
public:
    enum PinNames {
        GRID = 0,        // Control grid
        PLATE = 1,       // Plate (anode)
        CATHODE = 2,     // Cathode (heater not modeled separately)
    };

private:
    void UpdateTriodeState();
    double CalculatePlateCurrent(double vg, double vp);
};

#endif