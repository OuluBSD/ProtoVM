#include "ProtoVM.h"
#include "VoltageSources.h"
#include "AdditionalSources.h"

// Simple test for the new voltage sources and input components
void TestVoltageSources(Machine& mach) {
    LOG("Testing Voltage Sources and Input Components...");
    
    Pcb& pcb = mach.AddPcb();
    
    // Test DC Voltage Source (2-terminal)
    DcVoltageSource& dc_source = pcb.Add<DcVoltageSource>("DC_SOURCE");
    dc_source.SetVoltage(5.0);
    LOG("Created DC Voltage Source with 5V output");
    
    // Test AC Voltage Source (2-terminal)
    AcVoltageSource& ac_source = pcb.Add<AcVoltageSource>("AC_SOURCE");
    ac_source.SetFrequency(60.0);  // 60 Hz
    ac_source.SetAmplitude(2.5);   // 2.5V amplitude
    LOG("Created AC Voltage Source with 60Hz, 2.5V amplitude");
    
    // Test DC Voltage Source (1-terminal)
    DcVoltageSource1T& dc1t_source = pcb.Add<DcVoltageSource1T>("DC1T_SOURCE");
    dc1t_source.SetVoltage(3.3);
    LOG("Created DC Voltage Source (1-terminal) with 3.3V output");
    
    // Test AC Voltage Source (1-terminal)
    AcVoltageSource1T& ac1t_source = pcb.Add<AcVoltageSource1T>("AC1T_SOURCE");
    ac1t_source.SetFrequency(1000.0);  // 1 kHz
    ac1t_source.SetAmplitude(1.0);     // 1V amplitude
    LOG("Created AC Voltage Source (1-terminal) with 1kHz, 1V amplitude");
    
    // Test Square Wave Source (1-terminal)
    SquareWaveSource& square_source = pcb.Add<SquareWaveSource>("SQUARE_SOURCE");
    square_source.SetFrequency(100.0);  // 100 Hz
    square_source.SetAmplitude(2.0);    // 2V amplitude
    LOG("Created Square Wave Source with 100Hz, 2V amplitude");
    
    // Test Clock Source (1-terminal)
    ClockSource& clock_source = pcb.Add<ClockSource>("CLOCK_SOURCE");
    clock_source.SetFrequency(1000000.0);  // 1 MHz
    clock_source.SetDutyCycle(0.5);        // 50% duty cycle
    LOG("Created Clock Source with 1MHz, 50% duty cycle");
    
    // Test AC Sweep Source
    AcSweepSource& sweep_source = pcb.Add<AcSweepSource>("SWEEP_SOURCE");
    sweep_source.SetStartFrequency(1.0);   // 1 Hz
    sweep_source.SetStopFrequency(1000.0); // 1000 Hz
    sweep_source.SetDuration(5.0);         // 5 seconds duration
    LOG("Created AC Sweep Source from 1Hz to 1000Hz over 5 seconds");
    
    // Test Variable Voltage Source
    VariableVoltageSource& var_source = pcb.Add<VariableVoltageSource>("VAR_SOURCE");
    var_source.SetVoltage(2.5);
    var_source.SetMinVoltage(0.0);
    var_source.SetMaxVoltage(5.0);
    LOG("Created Variable Voltage Source with 2.5V, range 0-5V");
    
    // Test Antenna
    Antenna& antenna = pcb.Add<Antenna>("ANTENNA");
    antenna.SetFrequency(100.0e6);  // 100 MHz
    antenna.SetSensitivity(1.0);
    LOG("Created Antenna with 100MHz center frequency");
    
    // Test AM Source
    AmSource& am_source = pcb.Add<AmSource>("AM_SOURCE");
    am_source.SetCarrierFrequency(1000.0);     // 1 kHz carrier
    am_source.SetModulationFrequency(10.0);    // 10 Hz modulation
    am_source.SetModulationIndex(0.5);         // 50% modulation
    LOG("Created AM Source with 1kHz carrier, 10Hz modulation, 50% index");
    
    // Test FM Source
    FmSource& fm_source = pcb.Add<FmSource>("FM_SOURCE");
    fm_source.SetCarrierFrequency(100000.0);   // 100 kHz carrier
    fm_source.SetModulationFrequency(5.0);     // 5 Hz modulation
    fm_source.SetModulationIndex(2.0);         // 2.0 modulation index
    LOG("Created FM Source with 100kHz carrier, 5Hz modulation, index 2.0");
    
    // Test Current Source
    CurrentSource& current_source = pcb.Add<CurrentSource>("CURRENT_SOURCE");
    current_source.SetCurrent(0.001);  // 1 mA
    LOG("Created Current Source with 1mA output");
    
    // Test Noise Generator
    NoiseGenerator& noise_gen = pcb.Add<NoiseGenerator>("NOISE_GEN");
    noise_gen.SetNoiseType(NoiseGenerator::NoiseType::WHITE);
    noise_gen.SetAmplitude(0.1);
    LOG("Created White Noise Generator with 0.1V amplitude");
    
    // Test Audio Input
    AudioInput& audio_in = pcb.Add<AudioInput>("AUDIO_IN");
    audio_in.SetFrequency(440.0);  // A4 note
    audio_in.SetAmplitude(0.5);
    LOG("Created Audio Input with 440Hz, 0.5V amplitude");
    
    // Test Data Input (Parallel)
    DataInput& data_in_parallel = pcb.Add<DataInput>("DATA_IN_PAR");
    data_in_parallel.SetInputType(DataInput::InputType::PARALLEL);
    data_in_parallel.SetBitCount(8);
    data_in_parallel.SetDataValue(0xAB);
    LOG("Created 8-bit Parallel Data Input with value 0xAB");
    
    // Test Data Input (Serial)
    DataInput& data_in_serial = pcb.Add<DataInput>("DATA_IN_SER");
    data_in_serial.SetInputType(DataInput::InputType::SERIAL);
    data_in_serial.SetClockFrequency(1000.0);  // 1 kHz
    std::vector<bool> serial_data = {true, false, true, false, false, true, true, false};  // 0b10100110 = 0xA6
    data_in_serial.SetSerialData(serial_data);
    LOG("Created Serial Data Input with 1kHz clock and data pattern");
    
    // Test External Voltage
    ExternalVoltage& ext_voltage = pcb.Add<ExternalVoltage>("EXT_VOLTAGE");
    ext_voltage.SetVoltage(1.65);
    LOG("Created External Voltage Source with 1.65V");
    
    LOG("All voltage sources and input components created successfully!");
    
    // Run the simulation for a few ticks to verify components work
    for (int i = 0; i < 10; i++) {
        mach.Tick();
    }
    
    LOG("Voltage sources test completed successfully!");
}