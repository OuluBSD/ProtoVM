#ifndef _ProtoVM_Component_h_
#define _ProtoVM_Component_h_




struct Pin : public ElcBase {
	//RTTI_DECL1(Pin, ElcBase);
	
	byte is_high = 0;
	
	Pin();
	Pin& SetReference(bool is_high);
	
	bool Tick() override;
	bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
	bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
	int GetFixedPriority() const override;
};

struct Port : public ElcBase {
	//RTTI_DECL1(Port, ElcBase);
	
	Array<Pin> pins;
	
	
	void SetCount(int i) {pins.SetCount(i);}
	
	Pin& GetPin(int i) {return pins[i];}
	
};


//template <int Ohm>
class Resistor : public ElcBase {
	//RTTI_DECL1(Resistor, ElcBase);
	
	
public:
	Resistor() {
		AddSink("A");
		AddSource("B");
		
	}
	
	
	
};

using Resistor4k7 = Resistor;//<4700>;



class ResistorPack : public ElcBase {
	//RTTI_DECL1(ResistorPack, ElcBase);
	
	
public:
	ResistorPack(int c) {
		AddSink("A");
		for(int i = 0; i < c; i++)
			AddSource("B" + IntStr(i));
		
	}
	
	
	
};



class Crystal : public ElcBase {
	//RTTI_DECL1(Crystal, ElcBase);
	int hz = 0;
	
public:
	Crystal();
	
};




class ElcNor : public ElcBase {
	//RTTI_DECL1(ElcNor, ElcBase);
	bool in0 = 0;
	bool in1 = 0;
	bool out = 0;
	
public:
	ElcNor();
	
	bool Tick() override;
	bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
	bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
};


class ElcNand : public ElcBase {
	//RTTI_DECL1(ElcNand, ElcBase);
	bool in0 = 0;
	bool in1 = 0;
	bool out = 0;
	
public:
	ElcNand();
	
	bool Tick() override;
	bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
	bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
};


class ElcNot : public ElcBase {
	//RTTI_DECL1(ElcNot, ElcBase);
	bool in = 0;  // Input value
	bool out = 1; // Output value (starts high by default)
	
public:
	ElcNot();
	
	bool Tick() override;
	bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
	bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
};

class ElcXor : public ElcBase {
	//RTTI_DECL1(ElcXor, ElcBase);
	bool in0 = 0;
	bool in1 = 0;
	bool out = 0;
	
public:
	ElcXor();
	
	bool Tick() override;
	bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
	bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
};

class ElcXnor : public ElcBase {
	//RTTI_DECL1(ElcXnor, ElcBase);
	bool in0 = 0;
	bool in1 = 0;
	bool out = 0;
	
public:
	ElcXnor();
	
	bool Tick() override;
	bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
	bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
};

// Generic 2-to-1 Multiplexer with select line
class Mux2to1 : public ElcBase {
	//RTTI_DECL1(Mux2to1, ElcBase);
	bool in0 = 0;
	bool in1 = 0;
	bool sel = 0;
	bool out = 0;
	
public:
	Mux2to1();
	
	bool Tick() override;
	bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
	bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
};

// Generic 4-to-1 Multiplexer with 2 select lines
class Mux4to1 : public ElcBase {
	//RTTI_DECL1(Mux4to1, ElcBase);
	bool in[4] = {0, 0, 0, 0};  // 4 input lines
	bool sel[2] = {0, 0};       // 2 select lines
	bool out = 0;
	
public:
	Mux4to1();
	
	bool Tick() override;
	bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
	bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
};

// Generic 1-to-2 Demultiplexer with select line
class Demux1to2 : public ElcBase {
	//RTTI_DECL1(Demux1to2, ElcBase);
	bool input = 0;
	bool sel = 0;
	bool out[2] = {0, 0};  // 2 output lines
	
public:
	Demux1to2();
	
	bool Tick() override;
	bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
	bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
};

// Generic 1-to-4 Demultiplexer with 2 select lines
class Demux1to4 : public ElcBase {
	//RTTI_DECL1(Demux1to4, ElcBase);
	bool input = 0;
	bool sel[2] = {0, 0};  // 2 select lines
	bool out[4] = {0, 0, 0, 0};  // 4 output lines
	
public:
	Demux1to4();
	
	bool Tick() override;
	bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
	bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
};

// Generic 2-to-4 Decoder - converts 2-bit input to 4 active outputs
class Decoder2to4 : public ElcBase {
	//RTTI_DECL1(Decoder2to4, ElcBase);
	bool in[2] = {0, 0};     // 2 input lines
	bool en = 1;             // Enable line (active high)
	bool out[4] = {0, 0, 0, 0};  // 4 output lines
	
public:
	Decoder2to4();
	
	bool Tick() override;
	bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
	bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
};

// Generic 3-to-8 Decoder - converts 3-bit input to 8 active outputs
class Decoder3to8 : public ElcBase {
	//RTTI_DECL1(Decoder3to8, ElcBase);
	bool in[3] = {0, 0, 0};      // 3 input lines
	bool en = 1;                 // Enable line (active high)
	bool out[8] = {0, 0, 0, 0, 0, 0, 0, 0};  // 8 output lines
	
public:
	Decoder3to8();
	
	bool Tick() override;
	bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
	bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
};

// Generic 4-to-2 Priority Encoder - converts 4 input lines to 2-bit output
class Encoder4to2 : public ElcBase {
	//RTTI_DECL1(Encoder4to2, ElcBase);
	bool in[4] = {0, 0, 0, 0};   // 4 input lines
	bool out[2] = {0, 0};        // 2 output lines (binary representation)
	bool valid = 0;              // Indicates if any input is high
	
public:
	Encoder4to2();
	
	bool Tick() override;
	bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
	bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
};

// Generic 8-to-3 Priority Encoder - converts 8 input lines to 3-bit output
class Encoder8to3 : public ElcBase {
	//RTTI_DECL1(Encoder8to3, ElcBase);
	bool in[8] = {0, 0, 0, 0, 0, 0, 0, 0};  // 8 input lines
	bool out[3] = {0, 0, 0};                 // 3 output lines (binary representation)
	bool valid = 0;                          // Indicates if any input is high
	
public:
	Encoder8to3();
	
	bool Tick() override;
	bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
	bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
};


class ElcCapacitor : public ElcBase {
	//RTTI_DECL1(ElcCapacitor, ElcBase);
	
	
public:
	ElcCapacitor();
	
};


class FlipFlopJK : public ElcBase {
	//RTTI_DECL1(FlipFlopJK, ElcBase);
	
public:
	FlipFlopJK();
	
	
};


class FlipFlopD : public ElcBase {
	//RTTI_DECL1(FlipFlopD, ElcBase);
	bool d = 0;      // Data input
	bool clk = 0;    // Clock input
	bool q = 0;      // Output
	bool qn = 1;     // Inverted output
	bool en = 1;     // Enable input (active high)
	bool clr = 0;    // Clear input (active high)
	bool last_clk = 0;  // Previous clock state for edge detection

public:
	FlipFlopD();

	bool Tick() override;
	bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
	bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
};
class Register4Bit : public ElcBase {
	//RTTI_DECL1(Register4Bit, ElcBase);
	bool d[4] = {0, 0, 0, 0};   // 4-bit data input
	bool clk = 0;               // Clock input
	bool en = 1;                // Enable input (active high)
	bool clr = 0;               // Clear input (active high)
	bool q[4] = {0, 0, 0, 0};   // 4-bit output
	bool last_clk = 0;          // Previous clock state for edge detection

public:
	Register4Bit();

	bool Tick() override;
	bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
	bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
};

#include "TubeLogic.h"

#endif
