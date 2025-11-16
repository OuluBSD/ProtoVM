#ifndef EXAMPLE_PATCHES_H
#define EXAMPLE_PATCHES_H

#include "PresetManager.h"
#include "SynthArchitectures.h"

class ExamplePatches {
public:
    static void addClassicPatches(PresetManager& presetMgr) {
        // Add some basic patches to the standard preset manager
        createBassPatches(presetMgr);
        createLeadPatches(presetMgr);
        createPadPatches(presetMgr);
        createPercussivePatches(presetMgr);
        createAmbientPatches(presetMgr);
        
        std::cout << "Added " << presetMgr.getAllPresetNames().size() << " example patches to preset manager" << std::endl;
    }

private:
    static void createBassPatches(PresetManager& presetMgr) {
        // Moog-style bass
        PresetData moogBass;
        moogBass.name = "Moog Bass";
        moogBass.description = "Classic Moog-style bass sound using subtractive synthesis";
        moogBass.waveform = Waveform::SAWTOOTH;
        moogBass.attack = 0.01;
        moogBass.decay = 0.3;
        moogBass.sustain = 0.8;
        moogBass.release = 0.2;
        moogBass.filterCutoff = 0.2;
        moogBass.filterResonance = 0.7;
        moogBass.lfo1Rate = 0.5;
        moogBass.lfo1Depth = 0.1;
        moogBass.modulationConnections.push_back(
            std::make_tuple(ModulationSource::ADSR1, ModulationDestination::FILTER_CUTOFF, 0.8)
        );
        presetMgr.addPreset(moogBass);
        
        // TB-303 style acid bass
        PresetData acidBass;
        acidBass.name = "Acid Bass";
        acidBass.description = "Roland TB-303 style acid bass with squelchy filter";
        acidBass.waveform = Waveform::SAWTOOTH;
        acidBass.attack = 0.01;
        acidBass.decay = 0.1;
        acidBass.sustain = 0.9;
        acidBass.release = 0.1;
        acidBass.filterCutoff = 0.8;
        acidBass.filterResonance = 0.9;
        acidBass.modulationConnections.push_back(
            std::make_tuple(ModulationSource::ADSR1, ModulationDestination::FILTER_CUTOFF, 0.9)
        );
        presetMgr.addPreset(acidBass);
    }
    
    static void createLeadPatches(PresetManager& presetMgr) {
        // Classic analog lead
        PresetData analogLead;
        analogLead.name = "Analog Lead";
        analogLead.description = "Warm, smooth analog-style lead sound";
        analogLead.waveform = Waveform::SAWTOOTH;
        analogLead.attack = 0.05;
        analogLead.decay = 0.2;
        analogLead.sustain = 0.8;
        analogLead.release = 0.3;
        analogLead.filterCutoff = 0.6;
        analogLead.filterResonance = 0.4;
        analogLead.lfo1Rate = 3.5;
        analogLead.lfo1Depth = 0.05;
        analogLead.modulationConnections.push_back(
            std::make_tuple(ModulationSource::LFO1, ModulationDestination::OSC_FREQUENCY, 0.02)
        );
        analogLead.modulationConnections.push_back(
            std::make_tuple(ModulationSource::ADSR1, ModulationDestination::FILTER_CUTOFF, 0.4)
        );
        presetMgr.addPreset(analogLead);
        
        // Supersaw lead (using multiple oscillators)
        PresetData superSaw;
        superSaw.name = "SuperSaw";
        superSaw.description = "Thick, detuned sawtooth stack similar to Roland JP-8000";
        superSaw.waveform = Waveform::SAWTOOTH;
        superSaw.attack = 0.02;
        superSaw.decay = 0.15;
        superSaw.sustain = 0.9;
        superSaw.release = 0.25;
        superSaw.filterCutoff = 0.7;
        superSaw.filterResonance = 0.3;
        presetMgr.addPreset(superSaw);
    }
    
    static void createPadPatches(PresetManager& presetMgr) {
        // Warm string pad
        PresetData stringPad;
        stringPad.name = "Warm Strings";
        stringPad.description = "Rich, evolving string pad sound";
        stringPad.waveform = Waveform::SQUARE;
        stringPad.attack = 0.5;
        stringPad.decay = 0.3;
        stringPad.sustain = 0.9;
        stringPad.release = 0.5;
        stringPad.filterCutoff = 0.5;
        stringPad.filterResonance = 0.3;
        stringPad.lfo1Rate = 0.1;
        stringPad.lfo1Depth = 0.2;
        stringPad.lfo2Rate = 1.5;
        stringPad.lfo2Depth = 0.1;
        stringPad.modulationConnections.push_back(
            std::make_tuple(ModulationSource::LFO1, ModulationDestination::OSC_FREQUENCY, 0.01)
        );
        stringPad.modulationConnections.push_back(
            std::make_tuple(ModulationSource::LFO2, ModulationDestination::FILTER_CUTOFF, 0.1)
        );
        presetMgr.addPreset(stringPad);
        
        // Synthwave pad
        PresetData synthwavePad;
        synthwavePad.name = "Synthwave Pad";
        synthwavePad.description = "Ethereal pad with slow LFO movement";
        synthwavePad.waveform = Waveform::TRIANGLE;
        synthwavePad.attack = 0.8;
        synthwavePad.decay = 0.2;
        synthwavePad.sustain = 0.95;
        synthwavePad.release = 0.7;
        synthwavePad.filterCutoff = 0.4;
        synthwavePad.filterResonance = 0.4;
        synthwavePad.lfo1Rate = 0.05;
        synthwavePad.lfo1Depth = 0.3;
        synthwavePad.modulationConnections.push_back(
            std::make_tuple(ModulationSource::LFO1, ModulationDestination::FILTER_CUTOFF, 0.2)
        );
        presetMgr.addPreset(synthwavePad);
    }
    
    static void createPercussivePatches(PresetManager& presetMgr) {
        // Analog kick
        PresetData analogKick;
        analogKick.name = "Analog Kick";
        analogKick.description = "Classic analog kick drum sound";
        analogKick.waveform = Waveform::SINE;
        analogKick.attack = 0.001;
        analogKick.decay = 0.3;
        analogKick.sustain = 0.0;
        analogKick.release = 0.01;
        analogKick.filterCutoff = 0.8;
        analogKick.filterResonance = 0.9;
        analogKick.modulationConnections.push_back(
            std::make_tuple(ModulationSource::ADSR1, ModulationDestination::OSC_FREQUENCY, -0.9)
        );
        analogKick.modulationConnections.push_back(
            std::make_tuple(ModulationSource::ADSR1, ModulationDestination::FILTER_CUTOFF, -0.8)
        );
        presetMgr.addPreset(analogKick);
        
        // Analog snare - note: Waveform::NOISE doesn't exist yet, so using sine as placeholder
        PresetData analogSnare;
        analogSnare.name = "Analog Snare";
        analogSnare.description = "Analog snare with characteristic 'pop'";
        analogSnare.waveform = Waveform::SINE;  // Placeholder - would be noise in actual implementation
        analogSnare.attack = 0.001;
        analogSnare.decay = 0.2;
        analogSnare.sustain = 0.0;
        analogSnare.release = 0.05;
        analogSnare.filterCutoff = 0.6;
        analogSnare.filterResonance = 0.7;
        presetMgr.addPreset(analogSnare);
    }
    
    static void createAmbientPatches(PresetManager& presetMgr) {
        // Ambient texture
        PresetData ambientTexture;
        ambientTexture.name = "Ambient Texture";
        ambientTexture.description = "Slow evolving ambient pad";
        ambientTexture.waveform = Waveform::TRIANGLE;
        ambientTexture.attack = 1.5;
        ambientTexture.decay = 0.5;
        ambientTexture.sustain = 0.9;
        ambientTexture.release = 1.5;
        ambientTexture.filterCutoff = 0.3;
        ambientTexture.filterResonance = 0.6;
        ambientTexture.lfo1Rate = 0.02;
        ambientTexture.lfo1Depth = 0.4;
        ambientTexture.lfo2Rate = 0.05;
        ambientTexture.lfo2Depth = 0.3;
        ambientTexture.modulationConnections.push_back(
            std::make_tuple(ModulationSource::LFO1, ModulationDestination::FILTER_CUTOFF, 0.3)
        );
        ambientTexture.modulationConnections.push_back(
            std::make_tuple(ModulationSource::LFO2, ModulationDestination::OSC_FREQUENCY, 0.05)
        );
        presetMgr.addPreset(ambientTexture);
        
        // Bell pad
        PresetData bellPad;
        bellPad.name = "Bell Pad";
        bellPad.description = "Harmonic-rich bell-like pad";
        bellPad.waveform = Waveform::SAWTOOTH;
        bellPad.attack = 0.2;
        bellPad.decay = 1.0;
        bellPad.sustain = 0.5;
        bellPad.release = 1.0;
        bellPad.filterCutoff = 0.7;
        bellPad.filterResonance = 0.4;
        bellPad.modulationConnections.push_back(
            std::make_tuple(ModulationSource::ADSR1, ModulationDestination::FILTER_CUTOFF, 0.4)
        );
        presetMgr.addPreset(bellPad);
    }
};

// Additional FM synthesis patches
class FMPatches {
public:
    static void addFMPatches(PresetManager& presetMgr) {
        // Electric piano FM sound
        PresetData ePiano;
        ePiano.name = "FM Electric Piano";
        ePiano.description = "Classic FM electric piano sound";
        // For FM-specific patches, we'd apply different parameters
        presetMgr.addPreset(ePiano);
        
        // Bright bell sound
        PresetData bell;
        bell.name = "FM Bell";
        bell.description = "Bright, harmonic-rich bell sound";
        presetMgr.addPreset(bell);
        
        // Harp-like sound
        PresetData harp;
        harp.name = "FM Harp";
        harp.description = "Plucked string-like harp sound";
        presetMgr.addPreset(harp);
    }
};

// Wavetable synthesis patches
class WavetablePatches {
public:
    static void addWavetablePatches(PresetManager& presetMgr) {
        // Morphing pad
        PresetData morphPad;
        morphPad.name = "Wavetable Morph Pad";
        morphPad.description = "Pad that morphs between different waveforms";
        presetMgr.addPreset(morphPad);
        
        // Digital lead
        PresetData digitalLead;
        digitalLead.name = "Digital Lead";
        digitalLead.description = "Clean, digital-style lead sound";
        presetMgr.addPreset(digitalLead);
    }
};

#endif // EXAMPLE_PATCHES_H