#ifndef _ProtoVM_LinkBase_h_
#define _ProtoVM_LinkBase_h_




struct LinkBase {
	LinkBase* to = 0;
	ElectricNodeBase::Connector* sink = 0;
	ElectricNodeBase::Connector* src = 0;
	
	typedef LinkBase CLASSNAME;
	LinkBase();
	bool operator()(const LinkBase& a, const LinkBase& b) const;
	
	String ToString() const;
	
};

struct ProcessOp {
	ProcessType type = INVALID;
	LinkBase* link = 0;
	int priority = -1;
	ElectricNodeBase* processor = 0;
	ElectricNodeBase* dest = 0;
	ElectricNodeBase::Connector* src = 0;
	ElectricNodeBase::Connector* sink = 0;
	ProcessOp* successor = 0; // priority successor link (write after read)
	uint16 id = 0;
	uint16 dest_id = 0;
	int mem_bits = 0;
	int mem_bytes = 0;
	//int mem_id = -1;
	
	bool operator()(const ProcessOp& a, const ProcessOp& b) const;
	bool IsBiDir() const {return type == WRITE && successor != 0;}
	bool HasPriority() const {return priority != -1;}
	String ToString() const;
	
};

struct UnitOps : Moveable<UnitOps> {
	ElcBase* unit = 0;
	Vector<ProcessOp*> read_ops;
	Vector<ProcessOp*> write_ops;
	ProcessOp* tick_op = 0;
	
	//bool HasOps(ProcessType type) const;
	//bool HasReadOps() const {return HasOps(ProcessType::READ);}
	bool HasReadOps() const {return !read_ops.IsEmpty();}
};

class LinkBaseMap {
public:
	Array<LinkBase> links;
	Array<ProcessOp> rt_ops;
	VectorMap<size_t, UnitOps> units;
	
	
	void UpdateLinkBaseLayers();
	bool UpdateProcess();
	//void GetLayerRange(const ElectricNodeBase& n, int& min, int& max);
	
};




#endif
