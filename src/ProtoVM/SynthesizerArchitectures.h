#ifndef _ProtoVM_SynthesizerArchitectures_h_
#define _ProtoVM_SynthesizerArchitectures_h_

#include "AnalogCommon.h"
#include "VCO.h"
#include "VCF.h"
#include "VCA.h"
#include "LFO.h"
#include "ADSR.h"
#include "AudioOutputSystem.h"
#include "ModulationMatrix.h"
#include "PresetManager.h"
#include <vector>
#include <memory>

// Enum for different synthesizer architectures
enum class SynthArchitecture {
    SUBTRACTIVE,    // Classic subtractive synthesis (VCO -> VCF -> VCA)
    FM,             // Frequency modulation synthesis
    WAVE_TABLE,     // Wave table synthesis
    SAMPLER,        // Sample-based synthesis
    ADDITIVE,       // Additive synthesis
    PHYSICAL_MODEL, // Physical modeling synthesis
    GRANULAR,       // Granular synthesis
    WAVEGUIDE,      // Waveguide synthesis
    ALGORITHMIC     // Algorithmic synthesis
};

// Base class for synthesizer architectures
class SynthArchitectureBase : public AnalogNodeBase {
public:
    typedef SynthArchitectureBase CLASSNAME;

    SynthArchitectureBase(SynthArchitecture type);
    virtual ~SynthArchitectureBase() {}

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "SynthArchitectureBase"; }

    SynthArchitecture GetType() const { return architecture_type; }
    
    // Virtual methods for different synthesis types
    virtual bool NoteOn(int note, int velocity, int channel = 0) = 0;
    virtual bool NoteOff(int note, int channel = 0) = 0;
    virtual bool AllNotesOff() = 0;
    
    // Set parameters
    virtual bool SetParameter(const std::string& name, double value) = 0;
    virtual double GetParameter(const std::string& name) const = 0;
    
    // Get audio output
    virtual std::vector<double> GetAudioOutput() = 0;

protected:
    SynthArchitecture architecture_type;
    std::vector<std::unique_ptr<VCO>> vcos;
    std::vector<std::unique_ptr<VCF>> vcfs;
    std::vector<std::unique_ptr<VCA>> vcas;
    std::vector<std::unique_ptr<LFO>> lfos;
    std::vector<std::unique_ptr<ADSR>> adsrs;
    std::unique_ptr<ModulationMatrix> modulation_matrix;
    std::unique_ptr<AudioOutputSystem> audio_output;
    std::unique_ptr<PresetManager> preset_manager;
    
    // Helper methods
    double NoteToFrequency(int note) const;  // Convert MIDI note to frequency
    int FrequencyToNote(double freq) const;  // Convert frequency to MIDI note
};

// Subtractive synthesis architecture (VCO -> VCF -> VCA)
class SubtractiveSynth : public SynthArchitectureBase {
public:
    typedef SubtractiveSynth CLASSNAME;

    SubtractiveSynth();
    virtual ~SubtractiveSynth() {}

    virtual String GetClassName() const override { return "SubtractiveSynth"; }

    virtual bool NoteOn(int note, int velocity, int channel = 0) override;
    virtual bool NoteOff(int note, int channel = 0) override;
    virtual bool AllNotesOff() override;
    
    virtual bool SetParameter(const std::string& name, double value) override;
    virtual double GetParameter(const std::string& name) const override;
    
    virtual std::vector<double> GetAudioOutput() override;
    
    // Specific methods for subtractive synthesis
    void SetVCOCount(int count);
    void SetVCFCount(int count);
    void SetLFOCount(int count);
    void SetADSRCount(int count);
    
    // Configure the filter routing
    void SetFilterRouting(int vco_id, int vcf_id); // Which VCO connects to which VCF

private:
    std::vector<int> filter_routing;  // Maps VCO index to VCF index
    std::vector<double> current_note_frequencies;  // Active note frequencies
    std::vector<int> active_vcos;  // Active VCOs for current notes
};

// FM synthesis architecture (multiple operators)
class FMSynth : public SynthArchitectureBase {
public:
    typedef FMSynth CLASSNAME;

    FMSynth(int operators = 4);  // Default to 4 operators (4-op FM)
    virtual ~FMSynth() {}

    virtual String GetClassName() const override { return "FMSynth"; }

    virtual bool NoteOn(int note, int velocity, int channel = 0) override;
    virtual bool NoteOff(int note, int channel = 0) override;
    virtual bool AllNotesOff() override;
    
    virtual bool SetParameter(const std::string& name, double value) override;
    virtual double GetParameter(const std::string& name) const override;
    
    virtual std::vector<double> GetAudioOutput() override;
    
    // Specific methods for FM synthesis
    void SetAlgorithm(int alg);  // Set the operator algorithm
    void SetOperatorFrequencyRatio(int op_id, double ratio);  // Set frequency ratio for operator
    void SetOperatorLevel(int op_id, double level);  // Set output level for operator
    void SetModulationIndex(int modulator_id, int carrier_id, double index);  // Set modulation index
    
    struct Operator {
        std::unique_ptr<VCO> oscillator;  // The operator's oscillator
        double level;                     // Output level of the operator
        double frequency_ratio;          // Frequency ratio relative to base note
        std::vector<std::pair<int, double>> modulations;  // Which operators modulate this one and by how much
    };
    
private:
    std::vector<Operator> operators;
    int algorithm;  // The routing algorithm for operators
    std::vector<double> current_note_frequencies;  // Active note frequencies
    std::vector<std::vector<double>> operator_outputs;  // Each operator's output for each active note
};

// Wave table synthesis architecture
class WaveTableSynth : public SynthArchitectureBase {
public:
    typedef WaveTableSynth CLASSNAME;

    WaveTableSynth();
    virtual ~WaveTableSynth() {}

    virtual String GetClassName() const override { return "WaveTableSynth"; }

    virtual bool NoteOn(int note, int velocity, int channel = 0) override;
    virtual bool NoteOff(int note, int channel = 0) override;
    virtual bool AllNotesOff() override;
    
    virtual bool SetParameter(const std::string& name, double value) override;
    virtual double GetParameter(const std::string& name) const override;
    
    virtual std::vector<double> GetAudioOutput() override;
    
    // Specific methods for wave table synthesis
    void AddWaveTable(const std::vector<double>& wave_table);
    void SetWaveTableIndex(double index);  // Set wave table position (0.0 to 1.0)
    void SetWaveTableCrossfade(double crossfade_pos);  // Set crossfade position between tables

private:
    std::vector<std::vector<double>> wave_tables;  // Collection of wave tables
    std::vector<double> current_note_frequencies;  // Active note frequencies
    std::vector<double> current_wave_table_indices;  // Current position in wave table for each note
    double wave_table_position;  // Global wave table position (0.0 to 1.0)
    double wave_table_crossfade; // Crossfade position between adjacent tables
};

// Additive synthesis architecture
class AdditiveSynth : public SynthArchitectureBase {
public:
    typedef AdditiveSynth CLASSNAME;

    AdditiveSynth(int harmonics = 32);  // Default to 32 harmonics
    virtual ~AdditiveSynth() {}

    virtual String GetClassName() const override { return "AdditiveSynth"; }

    virtual bool NoteOn(int note, int velocity, int channel = 0) override;
    virtual bool NoteOff(int note, int channel = 0) override;
    virtual bool AllNotesOff() override;
    
    virtual bool SetParameter(const std::string& name, double value) override;
    virtual double GetParameter(const std::string& name) const override;
    
    virtual std::vector<double> GetAudioOutput() override;
    
    // Specific methods for additive synthesis
    void SetHarmonicLevel(int harmonic, double level);  // Set level of a specific harmonic
    void SetHarmonicFrequencyRatio(int harmonic, double ratio);  // Set frequency ratio of a harmonic
    void SetHarmonicPhase(int harmonic, double phase);  // Set phase of a harmonic

private:
    int harmonic_count;
    std::vector<std::vector<double>> harmonic_levels;  // Level of each harmonic for each active note
    std::vector<std::vector<double>> harmonic_ratios;  // Frequency ratio of each harmonic for each active note
    std::vector<std::vector<double>> harmonic_phases;  // Phase of each harmonic for each active note
    std::vector<double> current_note_frequencies;  // Active note frequencies
};

#endif