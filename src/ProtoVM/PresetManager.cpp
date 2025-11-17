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
bool PresetManager::SavePresetsToFile(const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        return false;
    }

    for (const auto& name : preset_order) {
        auto it = presets.find(name);
        if (it != presets.end()) {
            const auto& params = it->second;
            
            file << "[Preset]" << std::endl;
            file << "name=" << params.name << std::endl;
            file << "description=" << params.description << std::endl;
            file << "author=" << params.author << std::endl;
            file << "category=" << params.category << std::endl;
            file << "timestamp=" << params.created_timestamp << std::endl;
            
            // VCO parameters
            file << "vco_count=" << params.vco_params.size() << std::endl;
            for (size_t i = 0; i < params.vco_params.size(); ++i) {
                const auto& vco = params.vco_params[i];
                file << "vco" << i << "_waveform_type=" << vco.waveform_type << std::endl;
                file << "vco" << i << "_frequency=" << vco.frequency << std::endl;
                file << "vco" << i << "_amplitude=" << vco.amplitude << std::endl;
                file << "vco" << i << "_fm_amount=" << vco.fm_amount << std::endl;
                file << "vco" << i << "_pwm_duty_cycle=" << vco.pwm_duty_cycle << std::endl;
                file << "vco" << i << "_anti_aliasing=" << vco.anti_aliasing << std::endl;
            }
            
            // VCF parameters
            file << "vcf_filter_type=" << params.vcf_params.filter_type << std::endl;
            file << "vcf_cutoff_freq=" << params.vcf_params.cutoff_freq << std::endl;
            file << "vcf_resonance=" << params.vcf_params.resonance << std::endl;
            file << "vcf_env_amount=" << params.vcf_params.env_amount << std::endl;
            file << "vcf_key_track_amount=" << params.vcf_params.key_track_amount << std::endl;
            
            // VCA parameters
            file << "vca_level=" << params.vca_params.level << std::endl;
            file << "vca_linear_response=" << params.vca_params.linear_response << std::endl;
            
            // LFO parameters
            file << "lfo_count=" << params.lfo_params.size() << std::endl;
            for (size_t i = 0; i < params.lfo_params.size(); ++i) {
                const auto& lfo = params.lfo_params[i];
                file << "lfo" << i << "_waveform_type=" << lfo.waveform_type << std::endl;
                file << "lfo" << i << "_frequency=" << lfo.frequency << std::endl;
                file << "lfo" << i << "_amplitude=" << lfo.amplitude << std::endl;
            }
            
            // ADSR parameters
            file << "adsr_count=" << params.adsr_params.size() << std::endl;
            for (size_t i = 0; i < params.adsr_params.size(); ++i) {
                const auto& adsr = params.adsr_params[i];
                file << "adsr" << i << "_attack=" << adsr.attack << std::endl;
                file << "adsr" << i << "_decay=" << adsr.decay << std::endl;
                file << "adsr" << i << "_sustain=" << adsr.sustain << std::endl;
                file << "adsr" << i << "_release=" << adsr.release << std::endl;
            }
            
            // Modulation connections
            file << "mod_conn_count=" << params.modulation_params.connections.size() << std::endl;
            for (size_t i = 0; i < params.modulation_params.connections.size(); ++i) {
                const auto& conn = params.modulation_params.connections[i];
                file << "conn" << i << "_source=" << conn.source << std::endl;
                file << "conn" << i << "_destination=" << conn.destination << std::endl;
                file << "conn" << i << "_amount=" << conn.amount << std::endl;
                file << "conn" << i << "_active=" << conn.active << std::endl;
                file << "conn" << i << "_name=" << conn.name << std::endl;
            }
            
            file << "[EndPreset]" << std::endl << std::endl;
        }
    }
    
    file.close();
    return true;
}

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
bool PresetManager::LoadPresetsFromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return false;
    }

    // Clear existing presets
    presets.clear();
    preset_order.clear();

    std::string line;
    PatchParameters params;
    std::string current_preset_name;
    bool in_preset = false;

    while (std::getline(file, line)) {
        // Remove potential carriage return
        if (!line.empty() && line[line.length()-1] == '\r') {
            line.erase(line.length()-1);
        }

        if (line == "[Preset]") {
            in_preset = true;
            // Initialize new preset parameters
            params = PatchParameters();
        } else if (line == "[EndPreset]" && in_preset) {
            if (!current_preset_name.empty()) {
                presets[current_preset_name] = params;
                preset_order.push_back(current_preset_name);
            }
            in_preset = false;
            current_preset_name.clear();
        } else if (in_preset) {
            // Parse parameter lines
            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);

                if (key == "name") {
                    params.name = value;
                    current_preset_name = value;
                } else if (key == "description") {
                    params.description = value;
                } else if (key == "author") {
                    params.author = value;
                } else if (key == "category") {
                    params.category = std::stoi(value);
                } else if (key == "timestamp") {
                    params.created_timestamp = std::stod(value);
                }
                // VCO parameters
                else if (key == "vco_count") {
                    int count = std::stoi(value);
                    params.vco_params.resize(count);
                }
                else if (key.substr(0, 3) == "vco" && key.find("_waveform_type") != std::string::npos) {
                    size_t idx = std::stoi(key.substr(3, key.find("_") - 3));
                    if (idx < params.vco_params.size()) params.vco_params[idx].waveform_type = std::stoi(value);
                }
                else if (key.substr(0, 3) == "vco" && key.find("_frequency") != std::string::npos) {
                    size_t idx = std::stoi(key.substr(3, key.find("_") - 3));
                    if (idx < params.vco_params.size()) params.vco_params[idx].frequency = std::stod(value);
                }
                else if (key.substr(0, 3) == "vco" && key.find("_amplitude") != std::string::npos) {
                    size_t idx = std::stoi(key.substr(3, key.find("_") - 3));
                    if (idx < params.vco_params.size()) params.vco_params[idx].amplitude = std::stod(value);
                }
                else if (key.substr(0, 3) == "vco" && key.find("_fm_amount") != std::string::npos) {
                    size_t idx = std::stoi(key.substr(3, key.find("_") - 3));
                    if (idx < params.vco_params.size()) params.vco_params[idx].fm_amount = std::stod(value);
                }
                else if (key.substr(0, 3) == "vco" && key.find("_pwm_duty_cycle") != std::string::npos) {
                    size_t idx = std::stoi(key.substr(3, key.find("_") - 3));
                    if (idx < params.vco_params.size()) params.vco_params[idx].pwm_duty_cycle = std::stod(value);
                }
                else if (key.substr(0, 3) == "vco" && key.find("_anti_aliasing") != std::string::npos) {
                    size_t idx = std::stoi(key.substr(3, key.find("_") - 3));
                    if (idx < params.vco_params.size()) params.vco_params[idx].anti_aliasing = (value == "1");
                }
                // VCF parameters
                else if (key == "vcf_filter_type") {
                    params.vcf_params.filter_type = std::stoi(value);
                }
                else if (key == "vcf_cutoff_freq") {
                    params.vcf_params.cutoff_freq = std::stod(value);
                }
                else if (key == "vcf_resonance") {
                    params.vcf_params.resonance = std::stod(value);
                }
                else if (key == "vcf_env_amount") {
                    params.vcf_params.env_amount = std::stod(value);
                }
                else if (key == "vcf_key_track_amount") {
                    params.vcf_params.key_track_amount = std::stod(value);
                }
                // VCA parameters
                else if (key == "vca_level") {
                    params.vca_params.level = std::stod(value);
                }
                else if (key == "vca_linear_response") {
                    params.vca_params.linear_response = (value == "1");
                }
                // LFO parameters
                else if (key == "lfo_count") {
                    int count = std::stoi(value);
                    params.lfo_params.resize(count);
                }
                else if (key.substr(0, 3) == "lfo" && key.find("_waveform_type") != std::string::npos) {
                    size_t idx = std::stoi(key.substr(3, key.find("_") - 3));
                    if (idx < params.lfo_params.size()) params.lfo_params[idx].waveform_type = std::stoi(value);
                }
                else if (key.substr(0, 3) == "lfo" && key.find("_frequency") != std::string::npos) {
                    size_t idx = std::stoi(key.substr(3, key.find("_") - 3));
                    if (idx < params.lfo_params.size()) params.lfo_params[idx].frequency = std::stod(value);
                }
                else if (key.substr(0, 3) == "lfo" && key.find("_amplitude") != std::string::npos) {
                    size_t idx = std::stoi(key.substr(3, key.find("_") - 3));
                    if (idx < params.lfo_params.size()) params.lfo_params[idx].amplitude = std::stod(value);
                }
                // ADSR parameters
                else if (key == "adsr_count") {
                    int count = std::stoi(value);
                    params.adsr_params.resize(count);
                }
                else if (key.substr(0, 4) == "adsr" && key.find("_attack") != std::string::npos) {
                    size_t idx = std::stoi(key.substr(4, key.find("_") - 4));
                    if (idx < params.adsr_params.size()) params.adsr_params[idx].attack = std::stod(value);
                }
                else if (key.substr(0, 4) == "adsr" && key.find("_decay") != std::string::npos) {
                    size_t idx = std::stoi(key.substr(4, key.find("_") - 4));
                    if (idx < params.adsr_params.size()) params.adsr_params[idx].decay = std::stod(value);
                }
                else if (key.substr(0, 4) == "adsr" && key.find("_sustain") != std::string::npos) {
                    size_t idx = std::stoi(key.substr(4, key.find("_") - 4));
                    if (idx < params.adsr_params.size()) params.adsr_params[idx].sustain = std::stod(value);
                }
                else if (key.substr(0, 4) == "adsr" && key.find("_release") != std::string::npos) {
                    size_t idx = std::stoi(key.substr(4, key.find("_") - 4));
                    if (idx < params.adsr_params.size()) params.adsr_params[idx].release = std::stod(value);
                }
                // Modulation connections
                else if (key == "mod_conn_count") {
                    int count = std::stoi(value);
                    params.modulation_params.connections.resize(count);
                }
                else if (key.substr(0, 4) == "conn" && key.find("_source") != std::string::npos) {
                    size_t idx = std::stoi(key.substr(4, key.find("_") - 4));
                    if (idx < params.modulation_params.connections.size()) params.modulation_params.connections[idx].source = std::stoi(value);
                }
                else if (key.substr(0, 4) == "conn" && key.find("_destination") != std::string::npos) {
                    size_t idx = std::stoi(key.substr(4, key.find("_") - 4));
                    if (idx < params.modulation_params.connections.size()) params.modulation_params.connections[idx].destination = std::stoi(value);
                }
                else if (key.substr(0, 4) == "conn" && key.find("_amount") != std::string::npos) {
                    size_t idx = std::stoi(key.substr(4, key.find("_") - 4));
                    if (idx < params.modulation_params.connections.size()) params.modulation_params.connections[idx].amount = std::stod(value);
                }
                else if (key.substr(0, 4) == "conn" && key.find("_active") != std::string::npos) {
                    size_t idx = std::stoi(key.substr(4, key.find("_") - 4));
                    if (idx < params.modulation_params.connections.size()) params.modulation_params.connections[idx].active = (value == "1");
                }
                else if (key.substr(0, 4) == "conn" && key.find("_name") != std::string::npos) {
                    size_t idx = std::stoi(key.substr(4, key.find("_") - 4));
                    if (idx < params.modulation_params.connections.size()) params.modulation_params.connections[idx].name = value;
                }
            }
        }
    }

    file.close();
    return true;
}

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
Json::Value PresetManager::SerializeParameters(const PatchParameters& params) const {
    // For now, return nullptr since serialization is handled directly in SavePresetsToFile
    // This method is kept for API compatibility but not used in the new implementation
    return nullptr;
}

PatchParameters PresetManager::DeserializeParameters(void* json) const {
    // For now, return an empty PatchParameters since deserialization is handled directly in LoadPresetsFromFile
    // This method is kept for API compatibility but not used in the new implementation
    PatchParameters params;
    return params;
}

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