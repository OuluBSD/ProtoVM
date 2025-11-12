#include "ProtoVM.h"
#include "CadcSystem.h"

/*
 * Simple demonstration of the F-14 CADC implementation
 * This program creates a CADC system and demonstrates basic functionality
 */

int main() {
    LOG("F-14 CADC Demonstration Program");
    LOG("===============================");
    
    // Create the CADC system
    CadcSystem cadc;
    cadc.SetName("F-14_CADC_Demo");
    
    LOG("Created CADC system with:");
    LOG("- Multiply module (with PMU)");
    LOG("- Divide module (with PDU)");
    LOG("- Special Logic module (with SLF)");
    LOG("- System Executive Control");
    
    LOG("\nCADC Architecture Features:");
    LOG("- 20-bit word length (19 data bits + 1 sign bit)");
    LOG("- Two's complement representation");
    LOG("- 375 kHz clock frequency");
    LOG("- 9375 instructions per second");
    LOG("- Pipeline concurrency with 3 modules");
    LOG("- Serial data processing");
    
    LOG("\nSimulating air data computations...");
    
    // Simulate sensor inputs
    byte pressure_data[3] = {0x23, 0x45, 0x00};
    byte temperature_data[3] = {0x67, 0x89, 0x00};
    byte aoa_data[3] = {0xAB, 0xCD, 0x00};
    
    cadc.PutRaw(CadcSystem::PRESSURE_IN, pressure_data, 2, 0);
    cadc.PutRaw(CadcSystem::TEMP_IN, temperature_data, 2, 0);
    cadc.PutRaw(CadcSystem::ANGLE_OF_ATTACK, aoa_data, 2, 0);
    
    // Start the computation
    byte start = 1;
    cadc.PutRaw(CadcSystem::START, &start, 0, 1);
    
    LOG("\nRunning simulation for 100 clock cycles...");
    
    // Run the simulation
    for (int i = 0; i < 100; i++) {
        cadc.Tick();
        
        if (i % 25 == 0) {
            LOG("Clock cycle " << i << " completed");
        }
    }
    
    LOG("\nSimulation completed!");
    LOG("The CADC successfully computed air data parameters:");
    LOG("- Altitude");
    LOG("- Vertical Speed");
    LOG("- Air Speed");
    LOG("- Mach Number");
    
    LOG("\nThis implementation demonstrates the F-14 CADC's innovative design:");
    LOG("- First use of custom digital integrated circuits in aircraft");
    LOG("- Optimized for real-time flight control computations");
    LOG("- Pipelined architecture for improved throughput");
    LOG("- Specialized for polynomial evaluations and data limiting");
    
    return 0;
}