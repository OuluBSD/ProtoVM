#ifndef _ProtoVM_Common_h_
#define _ProtoVM_Common_h_




typedef enum {
	INVALID,
	WRITE,
	TICK,
} ProcessType;

class Pcb;
struct LinkBase;

class ElectricNodeBase {
	
	
public:
	typedef enum {
		V_WHOLE,
		V_PARTIAL,
		V_PARTIAL_RANGE,
	} Type;
	
	struct Connector;
	
	struct CLink : Moveable<CLink> {
		Connector* conn = 0;
		::LinkBase* link = 0;
	};
	
	struct Connector : Moveable<Connector> {
		String name;
		uint16 id = 0;
		bool is_sink = false;
		bool is_src = false;
		bool accept_multiconn = false;
		bool required = true;
		 
		Vector<CLink> links;
		ElectricNodeBase* base = 0;
		
		bool IsConnected() const {return !links.IsEmpty();}
		bool IsRequired() const {return required;}
		bool IsConnectable() const {return links.IsEmpty() || accept_multiconn;}
		void SetMultiConn() {accept_multiconn = true;}
		void SetRequired(bool b=true) {required = b;}
	};
	
	
protected:
	friend class Pcb;
	friend class LinkBaseMap;
	friend class Machine;  // Allow Machine to access connections for topological sort
	
	Pcb* pcb = 0;
	ElectricNodeBase* ptr = 0;
	int ptr_i = -1;
	int ptr_n = 0;
	Connector* ptr_conn = 0;
	String name;
	Array<Connector> conns;
	Type type = V_WHOLE;
	int sink_count = 0;
	int src_count = 0;
	int bi_count = 0;
	
	
	

	Connector& AddSource(String name);
	Connector& AddSink(String name);
	Connector& AddBidirectional(String name);
	
public:
	typedef ElectricNodeBase CLASSNAME;
	ElectricNodeBase();
	virtual ~ElectricNodeBase() {}

	void Clear();
	ElectricNodeBase& SetName(String s);
	ElectricNodeBase& NotRequired(String s);
	
	bool IsEmpty() const;
	bool IsTrivialSourceDefault() const;
	bool IsTrivialSourceDefaultRange() const;
	bool IsTrivialSinkDefault() const;
	bool IsTrivialSinkDefaultRange() const;
	int GetPinWidth() const;
	int GetPinBegin() const;
	Connector& Get(int i);
	Connector& GetTrivialSource();
	Connector& GetTrivialSink();
	//ElectricNodeBase& GetRange(int off, int len);
	String GetName() const {return name;}
	String GetClassName() const {return "ElectricNodeBase";}
	String GetDynamicName() const {return GetClassName() + "(" + name + ")";}
	
	ElectricNodeBase& operator>>(ElectricNodeBase& b);
	ElectricNodeBase& operator[](String code);
	ElectricNodeBase& operator[](int i);
	
	virtual int GetMemorySize() const {return 0;}
	virtual int GetFixedPriority() const {return -1;}
private:
	bool has_changed = true;  // Default to true to ensure first update
	int delay_ticks = 0;  // Propagation delay for this component in simulation ticks
	
	// Timing constraint tracking
	struct TimingInfo {
		int last_clock_edge_tick = -1;  // Tick when last clock edge occurred
		int data_change_tick = -1;      // Tick when input data last changed
		bool last_clock_state = false;  // Previous state of the clock signal
	};
	
	// Vector to track timing info for input pins (using Vector instead of Map to avoid template issues)
	Vector<TimingInfo> timing_info;
	Vector<String> timing_info_names;  // Names corresponding to timing_info entries
	
	// Setup and hold time requirements (in simulation ticks)
	int setup_time_ticks = 0;  // Minimum setup time in ticks
	int hold_time_ticks = 0;   // Minimum hold time in ticks
	
public:
	// Dependency tracking for topological sorting
	Vector<ElectricNodeBase*> dependents;    // Components that depend on this one
	Vector<ElectricNodeBase*> dependencies;  // Components this one depends on

public:
	bool HasChanged() const { return has_changed; }
	void SetChanged(bool changed = true) { has_changed = changed; }
	
	// Get/set propagation delay for this component
	int GetDelayTicks() const { return delay_ticks; }
	void SetDelayTicks(int delay) { delay_ticks = delay; }
	
	// Get/set timing constraints (setup and hold times) in simulation ticks
	int GetSetupTimeTicks() const { return setup_time_ticks; }
	void SetSetupTimeTicks(int setup_time) { setup_time_ticks = setup_time; }
	
	int GetHoldTimeTicks() const { return hold_time_ticks; }
	void SetHoldTimeTicks(int hold_time) { hold_time_ticks = hold_time; }
	
	// Schedule this component to tick after a specified delay
	void ScheduleTick(int delay);
	
	// Update timing information when input changes
	void UpdateTimingInfo(String input_name, int current_tick, bool is_clock = false, bool clock_state = false);
	
	// Check if timing constraints are satisfied
	bool CheckTimingConstraints(String input_name, int current_tick, bool is_clock_edge = false) const;
	
	// Methods for topological sorting
	void AddDependency(ElectricNodeBase& dependent);  // This component depends on 'dependent'
	Vector<ElectricNodeBase*>& GetDependents();      // Components that depend on this one
	Vector<ElectricNodeBase*>& GetDependencies();    // Components this one depends on
	
	virtual bool Tick() {
		LOG("error: Tick not implemented in " << GetClassName()); return false;
	}
	virtual bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
		LOG("error: Process not implemented in " << GetClassName()); return false;
	}
	virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
		LOG("error: PutRaw not implemented in " << GetClassName()); return false;
	}
	
};

using ElcBase = ElectricNodeBase;
using ElcConn = ElectricNodeBase::Connector;




class ElectricNode : public ElectricNodeBase {
	
	
public:
	typedef ElectricNode CLASSNAME;
	ElectricNode();
	
	//RTTI_DECL1(ElectricNode, ElectricNodeBase);
	
};


using ENode = ElectricNode;




#endif