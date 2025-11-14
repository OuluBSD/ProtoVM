#ifndef _ProtoVM_ADSR_h_
#define _ProtoVM_ADSR_h_

#include "AnalogCommon.h"

enum class ADSRState {
    IDLE,
    ATTACK,
    DECAY,
    SUSTAIN,
    RELEASE
};

class ADSR : public AnalogNodeBase {
public:
    typedef ADSR CLASSNAME;

    ADSR(double attack = 0.1, double decay = 0.2, double sustain = 0.7, double release = 0.3);
    virtual ~ADSR() {}

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "ADSR"; }

    void SetAttack(double attack);
    double GetAttack() const { return attack_time; }

    void SetDecay(double decay);
    double GetDecay() const { return decay_time; }

    void SetSustain(double sustain);
    double GetSustain() const { return sustain_level; }

    void SetRelease(double release);
    double GetRelease() const { return release_time; }

    void NoteOn();
    void NoteOff();

    double GetOutput() const { return output; }
    bool IsActive() const { return state != ADSRState::IDLE; }

    void SetSampleRate(int rate);
    int GetSampleRate() const { return sample_rate; }

private:
    double attack_time;           // Time in seconds for attack phase
    double decay_time;            // Time in seconds for decay phase
    double sustain_level;         // Level from 0.0 to 1.0 for sustain phase
    double release_time;          // Time in seconds for release phase

    ADSRState state;
    double output;                // Current output value
    int samples_in_current_phase; // Number of samples elapsed in current phase
    int total_samples_for_phase;  // Total samples required for current phase
    double phase_increment;       // How much output changes per sample
    double target_level;          // Target level for current phase
    
    int sample_rate;

    static constexpr double MIN_TIME = 0.001;  // Minimum time in seconds (1ms)
    static constexpr double MAX_TIME = 10.0;   // Maximum time in seconds
    static constexpr double MIN_LEVEL = 0.0;   // Minimum level
    static constexpr double MAX_LEVEL = 1.0;   // Maximum level
};

#endif