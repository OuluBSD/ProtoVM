#ifndef _ProtoVM_TubeMultiplexers_h_
#define _ProtoVM_TubeMultiplexers_h_

#include "AnalogCommon.h"
#include "TubeLogicGates.h"
#include "TubeFlipFlops.h"
#include "TubeCountersRegisters.h"
#include <vector>
#include <memory>

// Enum for multiplexer types
enum class MultiplexerType {
    TWO_TO_ONE,        // 2:1 multiplexer
    FOUR_TO_ONE,       // 4:1 multiplexer
    EIGHT_TO_ONE,      // 8:1 multiplexer
    SIXTEEN_TO_ONE,    // 16:1 multiplexer
    THIRTY_TWO_TO_ONE, // 32:1 multiplexer
    SIXTY_FOUR_TO_ONE, // 64:1 multiplexer
    ANALOG_SWITCH      // Analog switch multiplexer
};

// Enum for demultiplexer types
enum class DemultiplexerType {
    ONE_TO_TWO,        // 1:2 demultiplexer
    ONE_TO_FOUR,       // 1:4 demultiplexer
    ONE_TO_EIGHT,      // 1:8 demultiplexer
    ONE_TO_SIXTEEN,    // 1:16 demultiplexer
    ONE_TO_THIRTY_TWO, // 1:32 demultiplexer
    ONE_TO_SIXTY_FOUR  // 1:64 demultiplexer
};

// Base class for tube-based multiplexers
class TubeMultiplexer : public AnalogNodeBase {
public:
    typedef TubeMultiplexer CLASSNAME;

    TubeMultiplexer(int input_count = 2);
    virtual ~TubeMultiplexer();

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "TubeMultiplexer"; }

    // Get/set input signals
    void SetInput(int input_id, double signal);
    double GetInput(int input_id) const;
    
    // Get/set selector inputs
    void SetSelector(int selector_id, double signal);
    void SetSelectorValue(unsigned int value);  // Set all selectors from a single integer value
    double GetSelector(int selector_id) const;
    unsigned int GetSelectorValue() const;
    
    // Get output signal
    double GetOutput() const { return output_signal; }
    
    // Get number of inputs and selectors
    int GetInputCount() const { return input_count; }
    int GetSelectorCount() const { return selector_count; }
    
    // Get/set multiplexer type
    void SetMultiplexerType(MultiplexerType type) { mux_type = type; }
    MultiplexerType GetMultiplexerType() const { return mux_type; }
    
    // Enable/disable the multiplexer
    void SetEnabled(bool enabled) { is_enabled = enabled; }
    bool IsEnabled() const { return is_enabled; }
    
    // Get selected input (index of input currently connected to output)
    int GetSelectedInput() const { return selected_input; }
    
    // Get/set propagation delay
    void SetPropagationDelay(double delay) { propagation_delay = std::max(0.0, std::min(0.001, delay)); }
    double GetPropagationDelay() const { return propagation_delay; }
    
    // Get/set switching speed (impacts propagation delay)
    void SetSwitchingSpeed(double speed) { switching_speed = std::max(0.1, std::min(10.0, speed)); }
    double GetSwitchingSpeed() const { return switching_speed; }
    
    // Get/set rise/fall time (for analog multiplexers)
    void SetRiseTime(double time) { rise_time = std::max(0.0, std::min(0.001, time)); }
    double GetRiseTime() const { return rise_time; }
    
    void SetFallTime(double time) { fall_time = std::max(0.0, std::min(0.001, time)); }
    double GetFallTime() const { return fall_time; }
    
    // Reset the multiplexer
    virtual void Reset();

protected:
    MultiplexerType mux_type;
    int input_count;         // Number of input signals
    int selector_count;      // Number of selector lines
    std::vector<double> input_signals;  // Input signals
    std::vector<double> selector_signals;  // Selector signals
    double output_signal;     // Output signal
    bool is_enabled;          // Whether the multiplexer is enabled
    int selected_input;       // Index of the currently selected input
    double propagation_delay; // Propagation delay
    double switching_speed;   // Switching speed factor
    double rise_time;         // Rise time (for analog switches)
    double fall_time;         // Fall time (for analog switches)
    
    // Vector of tubes used in the multiplexer design
    std::vector<std::unique_ptr<Tube>> mux_tubes;
    
    // Internal processing method
    virtual void ProcessMultiplexer() = 0;
    
    // Initialize the multiplexer based on input count
    virtual void InitializeMultiplexer();
    
    // Convert selector value to input index
    virtual int SelectorValueToInputIndex() const;
    
    // Apply switching characteristics to output
    virtual void ApplySwitchingCharacteristics();
    
    static constexpr double MIN_PROPAGATION_DELAY = 0.0;
    static constexpr double MAX_PROPAGATION_DELAY = 0.001;  // 1ms
    static constexpr double MIN_SWITCHING_SPEED = 0.1;      // 0.1x (slow)
    static constexpr double MAX_SWITCHING_SPEED = 10.0;     // 10x (fast)
    static constexpr double MIN_RISE_FALL_TIME = 0.0;       // 0 seconds
    static constexpr double MAX_RISE_FALL_TIME = 0.001;     // 1ms
};

// Tube-based demultiplexer base class
class TubeDemultiplexer : public AnalogNodeBase {
public:
    typedef TubeDemultiplexer CLASSNAME;

    TubeDemultiplexer(int output_count = 2);
    virtual ~TubeDemultiplexer();

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "TubeDemultiplexer"; }

    // Get/set input signal
    void SetInput(double signal) { input_signal = signal; }
    double GetInput() const { return input_signal; }
    
    // Get/set selector inputs
    void SetSelector(int selector_id, double signal);
    void SetSelectorValue(unsigned int value);  // Set all selectors from a single integer value
    double GetSelector(int selector_id) const;
    unsigned int GetSelectorValue() const;
    
    // Get output signals
    double GetOutput(int output_id) const;
    std::vector<double> GetAllOutputs() const { return output_signals; }
    
    // Get number of outputs and selectors
    int GetOutputCount() const { return output_count; }
    int GetSelectorCount() const { return selector_count; }
    
    // Get/set demultiplexer type
    void SetDemultiplexerType(DemultiplexerType type) { demux_type = type; }
    DemultiplexerType GetDemultiplexerType() const { return demux_type; }
    
    // Enable/disable the demultiplexer
    void SetEnabled(bool enabled) { is_enabled = enabled; }
    bool IsEnabled() const { return is_enabled; }
    
    // Get selected output (index of output currently connected to input)
    int GetSelectedOutput() const { return selected_output; }
    
    // Get/set propagation delay
    void SetPropagationDelay(double delay) { propagation_delay = std::max(0.0, std::min(0.001, delay)); }
    double GetPropagationDelay() const { return propagation_delay; }
    
    // Get/set switching speed (impacts propagation delay)
    void SetSwitchingSpeed(double speed) { switching_speed = std::max(0.1, std::min(10.0, speed)); }
    double GetSwitchingSpeed() const { return switching_speed; }
    
    // Reset the demultiplexer
    virtual void Reset();

protected:
    DemultiplexerType demux_type;
    int output_count;        // Number of output signals
    int selector_count;      // Number of selector lines
    double input_signal;      // Input signal
    std::vector<double> selector_signals;  // Selector signals
    std::vector<double> output_signals;    // Output signals
    bool is_enabled;          // Whether the demultiplexer is enabled
    int selected_output;      // Index of the currently selected output
    double propagation_delay; // Propagation delay
    double switching_speed;   // Switching speed factor
    
    // Vector of tubes used in the demultiplexer design
    std::vector<std::unique_ptr<Tube>> demux_tubes;
    
    // Internal processing method
    virtual void ProcessDemultiplexer() = 0;
    
    // Initialize the demultiplexer based on output count
    virtual void InitializeDemultiplexer();
    
    // Convert selector value to output index
    virtual int SelectorValueToOutputIndex() const;
    
    // Apply switching characteristics to outputs
    virtual void ApplySwitchingCharacteristics();
    
    static constexpr double MIN_PROPAGATION_DELAY = 0.0;
    static constexpr double MAX_PROPAGATION_DELAY = 0.001;  // 1ms
    static constexpr double MIN_SWITCHING_SPEED = 0.1;      // 0.1x (slow)
    static constexpr double MAX_SWITCHING_SPEED = 10.0;     // 10x (fast)
};

// 2:1 Multiplexer using tubes
class Tube2To1Multiplexer : public TubeMultiplexer {
public:
    typedef Tube2To1Multiplexer CLASSNAME;

    Tube2To1Multiplexer();
    virtual ~Tube2To1Multiplexer() {}

    virtual String GetClassName() const override { return "Tube2To1Multiplexer"; }

protected:
    virtual void ProcessMultiplexer() override;
    virtual void InitializeMultiplexer() override;
    
    // Internal switching mechanism
    void SelectInputBasedOnSelectors();
};

// 4:1 Multiplexer using tubes
class Tube4To1Multiplexer : public TubeMultiplexer {
public:
    typedef Tube4To1Multiplexer CLASSNAME;

    Tube4To1Multiplexer();
    virtual ~Tube4To1Multiplexer() {}

    virtual String GetClassName() const override { return "Tube4To1Multiplexer"; }

protected:
    virtual void ProcessMultiplexer() override;
    virtual void InitializeMultiplexer() override;
    
    // Internal switching mechanism
    void SelectInputBasedOnSelectors();
};

// 8:1 Multiplexer using tubes
class Tube8To1Multiplexer : public TubeMultiplexer {
public:
    typedef Tube8To1Multiplexer CLASSNAME;

    Tube8To1Multiplexer();
    virtual ~Tube8To1Multiplexer() {}

    virtual String GetClassName() const override { return "Tube8To1Multiplexer"; }

protected:
    virtual void ProcessMultiplexer() override;
    virtual void InitializeMultiplexer() override;
    
    // Internal switching mechanism
    void SelectInputBasedOnSelectors();
};

// Analog switch multiplexer using tubes
class TubeAnalogSwitchMultiplexer : public TubeMultiplexer {
public:
    typedef TubeAnalogSwitchMultiplexer CLASSNAME;

    TubeAnalogSwitchMultiplexer(int input_count = 4);
    virtual ~TubeAnalogSwitchMultiplexer() {}

    virtual String GetClassName() const override { return "TubeAnalogSwitchMultiplexer"; }
    
    // Set channel isolation (how well other channels are blocked)
    void SetChannelIsolation(double isolation_db) { channel_isolation = std::max(20.0, std::min(100.0, isolation_db)); }
    double GetChannelIsolation() const { return channel_isolation; }
    
    // Set on-resistance of the switch
    void SetOnResistance(double resistance) { on_resistance = std::max(1.0, std::min(1000.0, resistance)); }
    double GetOnResistance() const { return on_resistance; }

protected:
    double channel_isolation;  // Channel isolation in dB
    double on_resistance;      // ON resistance of the switch in Ohms
    
    virtual void ProcessMultiplexer() override;
    virtual void InitializeMultiplexer() override;
    
    // Simulate analog switching behavior
    void ApplyAnalogSwitching();
};

// 1:4 Demultiplexer using tubes
class Tube1To4Demultiplexer : public TubeDemultiplexer {
public:
    typedef Tube1To4Demultiplexer CLASSNAME;

    Tube1To4Demultiplexer();
    virtual ~Tube1To4Demultiplexer() {}

    virtual String GetClassName() const override { return "Tube1To4Demultiplexer"; }

protected:
    virtual void ProcessDemultiplexer() override;
    virtual void InitializeDemultiplexer() override;
    
    // Internal switching mechanism
    void RouteInputToSelectedOutput();
};

// 1:8 Demultiplexer using tubes
class Tube1To8Demultiplexer : public TubeDemultiplexer {
public:
    typedef Tube1To8Demultiplexer CLASSNAME;

    Tube1To8Demultiplexer();
    virtual ~Tube1To8Demultiplexer() {}

    virtual String GetClassName() const override { return "Tube1To8Demultiplexer"; }

protected:
    virtual void ProcessDemultiplexer() override;
    virtual void InitializeDemultiplexer() override;
    
    // Internal switching mechanism
    void RouteInputToSelectedOutput();
};

// Multiplexer/demultiplexer combined component
class TubeMuxDemux : public AnalogNodeBase {
public:
    typedef TubeMuxDemux CLASSNAME;

    TubeMuxDemux(int channel_count = 4);
    virtual ~TubeMuxDemux();

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "TubeMuxDemux"; }

    // Multiplexer side
    void SetMuxInput(int input_id, double signal);
    double GetMuxInput(int input_id) const;
    void SetMuxSelector(int selector_id, double signal);
    void SetMuxSelectorValue(unsigned int value);
    double GetMuxOutput() const { return mux_output; }
    
    // Demultiplexer side
    void SetDemuxInput(double signal);
    double GetDemuxOutput(int output_id) const;
    void SetDemuxSelector(int selector_id, double signal);
    void SetDemuxSelectorValue(unsigned int value);
    std::vector<double> GetDemuxOutputs() const { return demux_outputs; }
    
    // Combined selector for both sides
    void SetCombinedSelector(int selector_id, double signal);
    void SetCombinedSelectorValue(unsigned int value);
    
    // Enable/disable
    void SetEnabled(bool enabled) { is_enabled = enabled; }
    bool IsEnabled() const { return is_enabled; }

protected:
    int channel_count;           // Number of channels
    int selector_count;          // Number of selector lines
    std::vector<double> mux_inputs;      // Multiplexer inputs
    std::vector<double> mux_selectors;   // Multiplexer selectors
    double mux_output;                    // Multiplexer output
    double demux_input;                   // Demultiplexer input
    std::vector<double> demux_selectors; // Demultiplexer selectors
    std::vector<double> demux_outputs;   // Demultiplexer outputs
    bool is_enabled;                      // Whether the component is enabled
    
    std::unique_ptr<TubeMultiplexer> mux;     // Internal multiplexer
    std::unique_ptr<TubeDemultiplexer> demux; // Internal demultiplexer
    
    virtual void ProcessMuxDemux();
    virtual void InitializeMuxDemux();
};

#endif