#ifndef ProtoVM_SerialOutputDevice_h
#define ProtoVM_SerialOutputDevice_h

#include "Component.h"
#include "ICs.h"
#include "Common.h"
#include "Bus.h"

/*
 * Serial output device that captures data from CPU output pins and writes to stdout
 */
class SerialOutputDevice : public ElectricNodeBase {
public:
    SerialOutputDevice() {
        // Add sink pins to receive data from CPU output pins
        AddSink("IN0");  // Input for OUT0 from CPU
        AddSink("IN1");  // Input for OUT1 from CPU 
        AddSink("IN2");  // Input for OUT2 from CPU
        AddSink("IN3");  // Input for OUT3 from CPU
        
        last_out0 = 0;
        last_out1 = 0;
        last_out2 = 0;
        last_out3 = 0;
    }
    
    String GetClassName() const override { return "SerialOutputDevice"; }
    
    bool Tick() override {
        return true;
    }
    
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override {
        return true; // No processing needed
    }
    
    // This is called when data comes in from CPU output pins
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override {
        if (conn_id >= 0 && conn_id <= 3 && data_bytes == 0 && data_bits == 1) {
            // Update the bit value in our internal state
            if (conn_id == 0) {
                last_out0 = *data & 1;
            } else if (conn_id == 1) {
                last_out1 = *data & 1;
            } else if (conn_id == 2) {
                last_out2 = *data & 1;
            } else if (conn_id == 3) {
                last_out3 = *data & 1;
            }
            
            // Check if all 4 bits of a nibble have been received
            // This is called for each bit, so we'll check if all bits are received
            // and then combine them into a character
            if (conn_id == 3) {  // If we just received OUT3, process the nibble
                byte combined = (last_out3 << 3) | (last_out2 << 2) | (last_out1 << 1) | last_out0;
                
                // Output the character to stdout
                Cout() << (char)combined;
                Cout().Flush(); // Ensure immediate output
            }
        }
        
        return true;
    }

private:
    byte last_out0, last_out1, last_out2, last_out3;
};

#endif