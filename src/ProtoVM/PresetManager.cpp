#include "PresetManager.h"
#include <fstream>
#include <algorithm>
#include <cctype>

PresetManager::PresetManager() 
    : current_preset(nullptr) {
}

PresetManager::~PresetManager() {
    // Clean up resources if needed
}

bool PresetManager::CaptureCurrentState(const std::string& name, const std::string& description) {
    // This would capture the current state of all synthesizer components
    // Since we don't have direct access to the synth components here,
    // we'll return false for now - this would need to be implemented
    // when connected to an actual synthesizer
    PatchParameters params;
    params.name = name;
    params.description = description;
    
    return CreatePreset(params, name);
}

bool PresetManager::CreatePreset(const PatchParameters& params, const std::string& name) {
    if (!IsValidPresetName(name)) {
        return false;
    }
    
    std::string normalized_name = NormalizePresetName(name);
    
    // Check if preset already exists
    auto it = presets.find(normalized_name);
    if (it != presets.end()) {
        // Update existing preset
        it->second = params;
    } else {
        // Add new preset
        presets[normalized_name] = params;
        preset_order.push_back(normalized_name);
    }
    
    return true;
}

bool PresetManager::LoadPreset(const std::string& name) {
    auto it = presets.find(NormalizePresetName(name));
    if (it != presets.end()) {
        current_preset = &it->second;
        if (preset_loaded_callback) {
            preset_loaded_callback(it->second);
        }
        return true;
    }
    return false;
}

bool PresetManager::LoadPreset(int index) {
    if (index < 0 || index >= preset_order.size()) {
        return false;
    }
    
    auto it = presets.find(preset_order[index]);
    if (it != presets.end()) {
        current_preset = &it->second;
        if (preset_loaded_callback) {
            preset_loaded_callback(it->second);
        }
        return true;
    }
    return false;
}

bool PresetManager::ApplyPreset(const PatchParameters& params) {
    // This method would apply the parameters to the actual synthesizer components
    // In a real implementation, this would connect to VCOs, VCFs, VCAs, etc.
    // For now, we'll just update the current preset
    if (current_preset) {
        *current_preset = params;
        if (preset_loaded_callback) {
            preset_loaded_callback(params);
        }
        return true;
    }
    return false;
}

bool PresetManager::SavePresetsToFile(const std::string& filepath) {
    Json::Value root;
    Json::Value presets_array(Json::arrayValue);
    
    for (const auto& name : preset_order) {
        auto it = presets.find(name);
        if (it != presets.end()) {
            Json::Value preset_obj = SerializeParameters(it->second);
            preset_obj["preset_name"] = it->second.name;
            preset_obj["preset_description"] = it->second.description;
            preset_obj["preset_author"] = it->second.author;
            preset_obj["preset_category"] = it->second.category;
            preset_obj["preset_timestamp"] = it->second.created_timestamp;
            presets_array.append(preset_obj);
        }
    }
    
    root["presets"] = presets_array;
    
    std::ofstream file(filepath);
    if (file.is_open()) {
        file << root;
        file.close();
        return true;
    }
    return false;
}

bool PresetManager::LoadPresetsFromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return false;
    }
    
    Json::Value root;
    file >> root;
    file.close();
    
    const Json::Value& presets_array = root["presets"];
    if (!presets_array.isArray()) {
        return false;
    }
    
    // Clear existing presets
    presets.clear();
    preset_order.clear();
    
    for (const auto& preset_json : presets_array) {
        PatchParameters params = DeserializeParameters(preset_json);
        
        // Extract additional fields
        if (preset_json.isMember("preset_name")) {
            params.name = preset_json["preset_name"].asString();
        }
        if (preset_json.isMember("preset_description")) {
            params.description = preset_json["preset_description"].asString();
        }
        if (preset_json.isMember("preset_author")) {
            params.author = preset_json["preset_author"].asString();
        }
        if (preset_json.isMember("preset_category")) {
            params.category = preset_json["preset_category"].asInt();
        }
        if (preset_json.isMember("preset_timestamp")) {
            params.created_timestamp = preset_json["preset_timestamp"].asDouble();
        }
        
        std::string name = params.name.empty() ? "unnamed" : params.name;
        std::string normalized_name = NormalizePresetName(name);
        
        presets[normalized_name] = params;
        preset_order.push_back(normalized_name);
    }
    
    return true;
}

std::vector<std::string> PresetManager::GetPresetNames() const {
    std::vector<std::string> names;
    for (const auto& name : preset_order) {
        names.push_back(name);
    }
    return names;
}

PatchParameters* PresetManager::GetPreset(const std::string& name) {
    auto it = presets.find(NormalizePresetName(name));
    if (it != presets.end()) {
        return &it->second;
    }
    return nullptr;
}

PatchParameters* PresetManager::GetPreset(int index) {
    if (index < 0 || index >= preset_order.size()) {
        return nullptr;
    }
    
    auto it = presets.find(preset_order[index]);
    if (it != presets.end()) {
        return &it->second;
    }
    return nullptr;
}

bool PresetManager::DeletePreset(const std::string& name) {
    std::string normalized_name = NormalizePresetName(name);
    auto it = presets.find(normalized_name);
    if (it == presets.end()) {
        return false;
    }
    
    presets.erase(it);
    
    // Remove from order vector
    auto order_it = std::find(preset_order.begin(), preset_order.end(), normalized_name);
    if (order_it != preset_order.end()) {
        preset_order.erase(order_it);
    }
    
    // Update current preset if it was deleted
    if (current_preset == &it->second) {
        current_preset = nullptr;
    }
    
    return true;
}

bool PresetManager::DeletePreset(int index) {
    if (index < 0 || index >= preset_order.size()) {
        return false;
    }
    
    std::string name = preset_order[index];
    preset_order.erase(preset_order.begin() + index);
    
    auto it = presets.find(name);
    if (it != presets.end()) {
        presets.erase(it);
        
        // Update current preset if it was deleted
        if (current_preset == &it->second) {
            current_preset = nullptr;
        }
        
        return true;
    }
    
    return false;
}

Json::Value PresetManager::SerializeParameters(const PatchParameters& params) const {
    Json::Value root;
    
    // Serialize VCO parameters
    Json::Value vco_array(Json::arrayValue);
    for (const auto& vco : params.vco_params) {
        Json::Value vco_obj;
        vco_obj["waveform_type"] = vco.waveform_type;
        vco_obj["frequency"] = vco.frequency;
        vco_obj["amplitude"] = vco.amplitude;
        vco_obj["fm_amount"] = vco.fm_amount;
        vco_obj["pwm_duty_cycle"] = vco.pwm_duty_cycle;
        vco_obj["anti_aliasing"] = vco.anti_aliasing;
        vco_array.append(vco_obj);
    }
    root["vco_params"] = vco_array;
    
    // Serialize VCF parameters
    Json::Value vcf_obj;
    vcf_obj["filter_type"] = params.vcf_params.filter_type;
    vcf_obj["cutoff_freq"] = params.vcf_params.cutoff_freq;
    vcf_obj["resonance"] = params.vcf_params.resonance;
    vcf_obj["env_amount"] = params.vcf_params.env_amount;
    vcf_obj["key_track_amount"] = params.vcf_params.key_track_amount;
    root["vcf_params"] = vcf_obj;
    
    // Serialize VCA parameters
    Json::Value vca_obj;
    vca_obj["level"] = params.vca_params.level;
    vca_obj["linear_response"] = params.vca_params.linear_response;
    root["vca_params"] = vca_obj;
    
    // Serialize LFO parameters
    Json::Value lfo_array(Json::arrayValue);
    for (const auto& lfo : params.lfo_params) {
        Json::Value lfo_obj;
        lfo_obj["waveform_type"] = lfo.waveform_type;
        lfo_obj["frequency"] = lfo.frequency;
        lfo_obj["amplitude"] = lfo.amplitude;
        lfo_array.append(lfo_obj);
    }
    root["lfo_params"] = lfo_array;
    
    // Serialize ADSR parameters
    Json::Value adsr_array(Json::arrayValue);
    for (const auto& adsr : params.adsr_params) {
        Json::Value adsr_obj;
        adsr_obj["attack"] = adsr.attack;
        adsr_obj["decay"] = adsr.decay;
        adsr_obj["sustain"] = adsr.sustain;
        adsr_obj["release"] = adsr.release;
        adsr_array.append(adsr_obj);
    }
    root["adsr_params"] = adsr_array;
    
    // Serialize modulation parameters
    Json::Value mod_obj;
    Json::Value connections_array(Json::arrayValue);
    for (const auto& conn : params.modulation_params.connections) {
        Json::Value conn_obj;
        conn_obj["source"] = conn.source;
        conn_obj["destination"] = conn.destination;
        conn_obj["amount"] = conn.amount;
        conn_obj["active"] = conn.active;
        conn_obj["name"] = conn.name;
        connections_array.append(conn_obj);
    }
    mod_obj["connections"] = connections_array;
    root["modulation_params"] = mod_obj;
    
    return root;
}

PatchParameters PresetManager::DeserializeParameters(const Json::Value& json) const {
    PatchParameters params;
    
    // Deserialize VCO parameters
    if (json.isMember("vco_params") && json["vco_params"].isArray()) {
        const Json::Value& vco_array = json["vco_params"];
        for (const auto& vco_json : vco_array) {
            PatchParameters::VCOParams vco;
            vco.waveform_type = vco_json["waveform_type"].asInt();
            vco.frequency = vco_json["frequency"].asDouble();
            vco.amplitude = vco_json["amplitude"].asDouble();
            vco.fm_amount = vco_json["fm_amount"].asDouble();
            vco.pwm_duty_cycle = vco_json["pwm_duty_cycle"].asDouble();
            vco.anti_aliasing = vco_json["anti_aliasing"].asBool();
            params.vco_params.push_back(vco);
        }
    }
    
    // Deserialize VCF parameters
    if (json.isMember("vcf_params")) {
        const Json::Value& vcf_obj = json["vcf_params"];
        params.vcf_params.filter_type = vcf_obj["filter_type"].asInt();
        params.vcf_params.cutoff_freq = vcf_obj["cutoff_freq"].asDouble();
        params.vcf_params.resonance = vcf_obj["resonance"].asDouble();
        params.vcf_params.env_amount = vcf_obj["env_amount"].asDouble();
        params.vcf_params.key_track_amount = vcf_obj["key_track_amount"].asDouble();
    }
    
    // Deserialize VCA parameters
    if (json.isMember("vca_params")) {
        const Json::Value& vca_obj = json["vca_params"];
        params.vca_params.level = vca_obj["level"].asDouble();
        params.vca_params.linear_response = vca_obj["linear_response"].asBool();
    }
    
    // Deserialize LFO parameters
    if (json.isMember("lfo_params") && json["lfo_params"].isArray()) {
        const Json::Value& lfo_array = json["lfo_params"];
        for (const auto& lfo_json : lfo_array) {
            PatchParameters::LFOParams lfo;
            lfo.waveform_type = lfo_json["waveform_type"].asInt();
            lfo.frequency = lfo_json["frequency"].asDouble();
            lfo.amplitude = lfo_json["amplitude"].asDouble();
            params.lfo_params.push_back(lfo);
        }
    }
    
    // Deserialize ADSR parameters
    if (json.isMember("adsr_params") && json["adsr_params"].isArray()) {
        const Json::Value& adsr_array = json["adsr_params"];
        for (const auto& adsr_json : adsr_array) {
            PatchParameters::ADSRParams adsr;
            adsr.attack = adsr_json["attack"].asDouble();
            adsr.decay = adsr_json["decay"].asDouble();
            adsr.sustain = adsr_json["sustain"].asDouble();
            adsr.release = adsr_json["release"].asDouble();
            params.adsr_params.push_back(adsr);
        }
    }
    
    // Deserialize modulation parameters
    if (json.isMember("modulation_params")) {
        const Json::Value& mod_obj = json["modulation_params"];
        if (mod_obj.isMember("connections") && mod_obj["connections"].isArray()) {
            const Json::Value& connections_array = mod_obj["connections"];
            for (const auto& conn_json : connections_array) {
                PatchParameters::ModulationParams::ConnectionParams conn;
                conn.source = conn_json["source"].asInt();
                conn.destination = conn_json["destination"].asInt();
                conn.amount = conn_json["amount"].asDouble();
                conn.active = conn_json["active"].asBool();
                conn.name = conn_json["name"].asString();
                params.modulation_params.connections.push_back(conn);
            }
        }
    }
    
    return params;
}

void PresetManager::SetPresetLoadedCallback(std::function<void(const PatchParameters& params)> callback) {
    preset_loaded_callback = callback;
}

std::string PresetManager::NormalizePresetName(const std::string& name) const {
    std::string normalized = name;
    // Convert to lowercase for case-insensitive comparison
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
    return normalized;
}

bool PresetManager::IsValidPresetName(const std::string& name) const {
    if (name.empty()) {
        return false;
    }
    
    // Check for invalid characters
    for (char c : name) {
        if (!std::isalnum(c) && c != ' ' && c != '-' && c != '_' && c != '.') {
            return false;
        }
    }
    
    return true;
}