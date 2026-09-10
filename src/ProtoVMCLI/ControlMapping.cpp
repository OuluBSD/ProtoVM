#include "ControlMapping.h"
#include "InstrumentRuntime.h"
#include "InstrumentGraph.h"
#include "JsonIO.h"
#include "AutomationExport.h"

#include <cmath>
#include <algorithm>

using namespace Upp;

Result<ControlMap> ControlMappingEngine::CreateDefaultMapForInstrument(const InstrumentGraph& instrument) {
    ControlMap control_map;
    control_map.map_id = "default_" + instrument.instrument_id;
    control_map.name = "Default Control Map for " + instrument.name;
    
    // Auto-map common parameters to MIDI CCs
    int cc_counter = 1; // Start from CC1 to leave CC0 for bank selection
    
    // Map instrument parameters
    for (const auto& param : instrument.parameters) {
        if (param.type == "float") { // Only map float parameters
            ControlMapping mapping;
            mapping.mapping_id = "map_" + instrument.instrument_id + "_" + param.id;
            mapping.source.kind = ControlSourceKind::MidiCC;
            mapping.source.channel = 0;
            mapping.source.number = cc_counter++;
            mapping.source.name = "CC" + String(cc_counter - 1);
            
            mapping.target.kind = ControlTargetKind::InstrumentParam;
            mapping.target.target_id = param.id;
            
            // Set value range based on parameter range, if available
            mapping.min_value = param.min_value;
            mapping.max_value = param.max_value;
            mapping.curve = 1.0; // Linear by default
            
            // Skip if we run out of CC numbers (MIDI has 128)
            if (cc_counter > 120) break;
            
            control_map.mappings.Add(mapping);
        }
    }
    
    // Map automation lanes if present
    for (int i = 0; i < std::min(instrument.automation_lanes.GetCount(), 5); ++i) {
        ControlMapping mapping;
        mapping.mapping_id = "lane_map_" + std::to_string(i);
        mapping.source.kind = ControlSourceKind::MidiCC;
        mapping.source.channel = 0;
        mapping.source.number = cc_counter++;
        mapping.source.name = "CC" + String(cc_counter - 1);
        
        mapping.target.kind = ControlTargetKind::AutomationLane;
        mapping.target.target_id = instrument.automation_lanes[i].id;
        
        // Skip if we run out of CC numbers
        if (cc_counter > 120) break;
        
        control_map.mappings.Add(mapping);
    }
    
    // Map intent profiles if present
    for (int i = 0; i < std::min(instrument.intent_profiles.GetCount(), 4); ++i) {
        ControlMapping mapping;
        mapping.mapping_id = "intent_map_" + std::to_string(i);
        mapping.source.kind = ControlSourceKind::MidiCC;
        mapping.source.channel = 0;
        mapping.source.number = cc_counter++;
        mapping.source.name = "CC" + String(cc_counter - 1);
        
        mapping.target.kind = ControlTargetKind::IntentProfileMix;
        mapping.target.target_id = instrument.intent_profiles[i].profile_id;
        
        // Skip if we run out of CC numbers
        if (cc_counter > 120) break;
        
        control_map.mappings.Add(mapping);
    }
    
    return control_map;
}

Result<void> ControlMappingEngine::ApplyControlValue(InstrumentRuntime& runtime, 
                                                    const ControlMapping& mapping, 
                                                    double normalized_value) {
    // Apply curve transformation to the normalized value
    double transformed_value;
    if (mapping.bipolar) {
        // Transform normalized 0..1 to bipolar -1..1
        double bipolar_value = (normalized_value * 2.0) - 1.0;
        
        // Apply curve to bipolar value (-1 to 1 becomes -1 to 1 regardless of curve)
        if (mapping.curve != 1.0) {
            double sign = (bipolar_value >= 0) ? 1.0 : -1.0;
            transformed_value = sign * pow(abs(bipolar_value), mapping.curve);
        } else {
            transformed_value = bipolar_value;
        }
        
        // Map back to the target range
        double mid_range = (mapping.max_value + mapping.min_value) / 2.0;
        double half_range = (mapping.max_value - mapping.min_value) / 2.0;
        transformed_value = mid_range + (transformed_value * half_range);
    } else {
        // Apply curve transformation to unipolar value
        if (mapping.curve != 1.0) {
            transformed_value = pow(normalized_value, mapping.curve);
        } else {
            transformed_value = normalized_value;
        }
        
        // Map to the target range
        transformed_value = mapping.min_value + 
                           (transformed_value * (mapping.max_value - mapping.min_value));
    }
    
    // Apply the transformed value to the target
    switch (mapping.target.kind) {
        case ControlTargetKind::InstrumentParam:
            // Update the runtime parameter
            runtime.SetParameter(mapping.target.target_id, transformed_value);
            break;

        case ControlTargetKind::AutomationLane:
            // Update automation lane value
            runtime.SetAutomationLaneValue(mapping.target.target_id, transformed_value);
            break;

        case ControlTargetKind::IntentStrength:
            // Update intent strength (0.0f to 1.0f range)
            transformed_value = std::max(0.0, std::min(1.0, transformed_value)); // Clamp to 0-1 range
            runtime.SetIntentionStrength(mapping.target.target_id, static_cast<float>(transformed_value));
            break;

        case ControlTargetKind::IntentProfileMix:
            // Update intent profile mix (0.0f to 1.0f range)
            transformed_value = std::max(0.0, std::min(1.0, transformed_value)); // Clamp to 0-1 range
            runtime.SetIntentionProfileMix(mapping.target.target_id, static_cast<float>(transformed_value));
            break;

        case ControlTargetKind::SceneMorph:
            // Update scene morph position (0.0 to 1.0 range)
            transformed_value = std::max(0.0, std::min(1.0, transformed_value)); // Clamp to 0-1 range
            // In a real implementation, this would morph between two scenes
            // For now, we'll store this value as a parameter that can be used for morphing
            runtime.SetParameter("scene_morph_position", transformed_value);
            break;

        default:
            return Result<void>::Error("Unknown control target kind");
    }
    
    return Result<void>::Success();
}

Result<void> ControlMappingEngine::ApplyMidiEvent(InstrumentRuntime& runtime,
                                                 const ControlMap& map,
                                                 const MidiEvent& event) {
    // Find the corresponding control mapping based on the MIDI event
    for (const auto& mapping : map.mappings) {
        if (mapping.source.channel != event.channel) continue;
        
        bool source_matches = false;
        
        switch (mapping.source.kind) {
            case ControlSourceKind::MidiCC:
                if (event.kind == MidiEventKind::CC && mapping.source.number == event.number) {
                    source_matches = true;
                }
                break;
                
            case ControlSourceKind::MidiNote:
                if ((event.kind == MidiEventKind::NoteOn || event.kind == MidiEventKind::NoteOff) 
                    && mapping.source.number == event.number) {
                    source_matches = true;
                }
                break;
                
            case ControlSourceKind::MidiPitchBend:
                if (event.kind == MidiEventKind::PitchBend) {
                    source_matches = true; // All pitch bend events map to the same source number
                }
                break;
                
            default:
                // For now, we only handle the most common MIDI events
                continue;
        }
        
        if (!source_matches) continue;
        
        // Convert MIDI value to normalized value (0.0 to 1.0)
        double normalized_value;
        switch (event.kind) {
            case MidiEventKind::NoteOn:
                normalized_value = 1.0; // Full on
                break;
                
            case MidiEventKind::NoteOff:
                normalized_value = 0.0; // Full off
                break;
                
            case MidiEventKind::CC:
                normalized_value = static_cast<double>(event.value) / 127.0;
                break;
                
            case MidiEventKind::PitchBend:
                // Pitch bend typically ranges from 0-16383, centered at 8192
                normalized_value = static_cast<double>(event.value) / 16383.0;
                break;
                
            default:
                continue; // Unknown event type
        }
        
        // Apply the control value using the existing method
        auto result = ApplyControlValue(runtime, mapping, normalized_value);
        if (!result.IsOK()) {
            return result;
        }
    }
    
    return Result<void>::Success();
}