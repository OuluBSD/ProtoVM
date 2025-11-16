#ifndef _ProtoVM_ExamplePatches_h_
#define _ProtoVM_ExamplePatches_h_

#include "PresetManager.h"
#include "SynthesizerArchitectures.h"
#include <vector>
#include <string>

// Enum for different types of classic synthesizer sounds
enum class PatchCategory {
    LEAD,
    BASS,
    PAD,
    FX,
    PLUCKED,
    BELL,
    BRASS,
    STRING,
    VOCAL,
    DRUM
};

// Structure to hold information about a patch
struct PatchInfo {
    std::string name;
    std::string description;
    PatchCategory category;
    SynthArchitecture architecture;  // The type of synth architecture used
    std::string author;
    double rating;  // 0.0-5.0 user rating
    
    PatchInfo(const std::string& n, const std::string& desc, PatchCategory cat, 
              SynthArchitecture arch, const std::string& auth = "ProtoVM", double rt = 0.0)
        : name(n), description(desc), category(cat), architecture(arch), author(auth), rating(rt) {}
};

// Class that provides example patches for different synthesizer architectures
class ExamplePatches {
public:
    ExamplePatches();
    virtual ~ExamplePatches();
    
    // Create and register all example patches with the preset manager
    void CreateAllExamplePatches(PresetManager& preset_manager);
    
    // Create patches for specific architecture
    void CreateSubtractivePatches(PresetManager& preset_manager);
    void CreateFMPatches(PresetManager& preset_manager);
    void CreateWaveTablePatches(PresetManager& preset_manager);
    void CreateAdditivePatches(PresetManager& preset_manager);
    
    // Individual patch creation methods
    void CreateWarmPad(PresetManager& preset_manager);
    void CreateSharpLead(PresetManager& preset_manager);
    void CreateBassPatch(PresetManager& preset_manager);
    void CreateBellSound(PresetManager& preset_manager);
    void CreateStringSound(PresetManager& preset_manager);
    void CreateBrassSection(PresetManager& preset_manager);
    void CreateAnalogBass(PresetManager& preset_manager);
    void CreateVocoderEffect(PresetManager& preset_manager);
    void CreateChiptuneSound(PresetManager& preset_manager);
    void CreateFMBrass(PresetManager& preset_manager);
    void CreateFMBell(PresetManager& preset_manager);
    void CreateFMElectricPiano(PresetManager& preset_manager);
    void CreateWaveTableSaw(PresetManager& preset_manager);
    void CreateWaveTableSquare(PresetManager& preset_manager);
    void CreateWaveTableSync(PresetManager& preset_manager);
    void CreateAdditiveHarmonic(PresetManager& preset_manager);
    void CreateAdditiveFormant(PresetManager& preset_manager);
    void CreateAdditiveBell(PresetManager& preset_manager);
    
    // Get list of all available patch names
    std::vector<PatchInfo> GetAvailablePatches() const { return available_patches; }
    
    // Set parameters for different categories
    PatchParameters CreateWarmPadParams();
    PatchParameters CreateSharpLeadParams();
    PatchParameters CreateBassParams();
    PatchParameters CreateBellParams();
    PatchParameters CreateStringParams();
    PatchParameters CreateBrassParams();
    PatchParameters CreateAnalogBassParams();
    PatchParameters CreateVocoderEffectParams();
    PatchParameters CreateChiptuneParams();
    
private:
    std::vector<PatchInfo> available_patches;
    
    // Helper methods to create parameter structures for different patches
    PatchParameters CreateSubtractiveBaseParams();
    PatchParameters CreateFMBaseParams();
    PatchParameters CreateWaveTableBaseParams();
    PatchParameters CreateAdditiveBaseParams();
    
    // Helper methods for specific patch characteristics
    void ApplyADSRToParams(PatchParameters& params, double a, double d, double s, double r);
    void AddModulationConnection(PatchParameters& params, ModulationSource source, 
                                ModulationDestination dest, double amount, const std::string& name = "");
};

#endif