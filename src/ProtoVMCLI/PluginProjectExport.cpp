#include "PluginProjectExport.h"
#include "PluginSkeletonExport.h"
#include <sstream>
#include <fstream>
#include <sys/stat.h>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

Result<void> PluginProjectExport::CreateDirectories(const String& output_dir) {
    // Create the main output directory and subdirectories
    try {
        // Create main output directory
        fs::create_directories(output_dir);
        
        // Create src directory
        fs::create_directories(output_dir + "/src");
        
        // Create build directory
        fs::create_directories(output_dir + "/build");
        
        // Create metadata directory (for LV2/CLAP)
        fs::create_directories(output_dir + "/metadata");
    } catch (const std::exception& e) {
        return Result<void>::Error(String("Failed to create directories: ") + e.what());
    }
    
    return Result<void>::Success();
}

Result<void> PluginProjectExport::WriteWrapperSource(
    const String& wrapper_path,
    const PluginProjectExportOptions& opts
) {
    // Generate the plugin skeleton source with options
    PluginSkeletonOptions skeleton_opts;
    skeleton_opts.target = opts.target;
    skeleton_opts.plugin_name = opts.plugin_name;
    skeleton_opts.plugin_id = opts.plugin_id;
    skeleton_opts.vendor = opts.vendor;
    skeleton_opts.num_inputs = opts.num_inputs;
    skeleton_opts.num_outputs = opts.num_outputs;
    
    auto skeleton_result = PluginSkeletonExport::EmitPluginSkeletonSource(skeleton_opts);
    if (skeleton_result.IsError()) {
        return Result<void>::Error(skeleton_result.Error());
    }
    
    try {
        std::ofstream file(wrapper_path);
        if (!file.is_open()) {
            return Result<void>::Error("Cannot open wrapper file for writing: " + wrapper_path);
        }
        file << skeleton_result.Value();
        file.close();
    } catch (const std::exception& e) {
        return Result<void>::Error(String("Failed to write wrapper source: ") + e.what());
    }
    
    return Result<void>::Success();
}

Result<void> PluginProjectExport::WriteBuildFiles(
    const String& output_dir,
    const PluginProjectExportOptions& opts
) {
    std::ostringstream cmake_content;
    
    // Generate CMakeLists.txt content
    cmake_content << "cmake_minimum_required(VERSION 3.10)\n";
    cmake_content << "project(" << opts.plugin_name << " VERSION " << opts.version << ")\n\n";
    
    cmake_content << "set(CMAKE_CXX_STANDARD 17)\n";
    cmake_content << "set(CMAKE_CXX_STANDARD_REQUIRED ON)\n\n";
    
    // Determine source files based on target
    String wrapper_file = "PluginWrapper.cpp";
    String target_specific_file = "";
    
    switch (opts.target) {
        case PluginTargetKind::Vst3:
            target_specific_file = opts.plugin_name + "Vst3.cpp";
            break;
        case PluginTargetKind::Lv2:
            target_specific_file = opts.plugin_name + "Lv2.c";
            break;
        case PluginTargetKind::Clap:
            target_specific_file = opts.plugin_name + "Clap.cpp";
            break;
        case PluginTargetKind::Ladspa:
            target_specific_file = opts.plugin_name + "Ladspa.c";
            break;
    }
    
    cmake_content << "add_library(" << opts.plugin_name << " MODULE\n";
    cmake_content << "    src/" << wrapper_file << "\n";
    if (!target_specific_file.empty()) {
        cmake_content << "    src/" << target_specific_file << "\n";
    }
    cmake_content << ")\n\n";
    
    cmake_content << "# TODO: Add include directories for actual VST3/LV2/CLAP/LADSPA SDKs.\n";
    cmake_content << "# For now, this scaffold assumes you will add the correct SDK paths.\n\n";
    
    // Add target-specific build configurations
    switch (opts.target) {
        case PluginTargetKind::Vst3:
            cmake_content << "# VST3-specific configuration\n";
            cmake_content << "set_target_properties(" << opts.plugin_name << " PROPERTIES PREFIX \"\")\n";
            cmake_content << "set_target_properties(" << opts.plugin_name << " PROPERTIES SUFFIX \".vst3\")\n";
            cmake_content << "# Add VST3 SDK include/lib directories when available\n";
            break;
        case PluginTargetKind::Lv2:
            cmake_content << "# LV2-specific configuration\n";
            cmake_content << "set_target_properties(" << opts.plugin_name << " PROPERTIES PREFIX \"\")\n";
            cmake_content << "set_target_properties(" << opts.plugin_name << " PROPERTIES SUFFIX \"_lv2.so\")\n";
            cmake_content << "# Install to ~/.lv2 directory after building\n";
            break;
        case PluginTargetKind::Clap:
            cmake_content << "# CLAP-specific configuration\n";
            cmake_content << "set_target_properties(" << opts.plugin_name << " PROPERTIES PREFIX \"\")\n";
            cmake_content << "set_target_properties(" << opts.plugin_name << " PROPERTIES SUFFIX \".clap\")\n";
            cmake_content << "# Add CLAP SDK include/lib directories when available\n";
            break;
        case PluginTargetKind::Ladspa:
            cmake_content << "# LADSPA-specific configuration\n";
            cmake_content << "set_target_properties(" << opts.plugin_name << " PROPERTIES PREFIX \"\")\n";
            cmake_content << "set_target_properties(" << opts.plugin_name << " PROPERTIES SUFFIX \".so\")\n";
            cmake_content << "# Install to appropriate LADSPA directory after building\n";
            break;
    }
    
    try {
        std::string cmake_path = std::string(output_dir) + "/CMakeLists.txt";
        std::ofstream file(cmake_path);
        if (!file.is_open()) {
            return Result<void>::Error("Cannot open CMakeLists.txt for writing: " + cmake_path);
        }
        file << cmake_content.str();
        file.close();
    } catch (const std::exception& e) {
        return Result<void>::Error(String("Failed to write CMakeLists.txt: ") + e.what());
    }
    
    return Result<void>::Success();
}

Result<void> PluginProjectExport::WriteMetadataFiles(
    const String& output_dir,
    const PluginProjectExportOptions& opts
) {
    try {
        switch (opts.target) {
            case PluginTargetKind::Lv2: {
                // Write LV2 manifest.ttl
                std::ofstream manifest_file(std::string(output_dir) + "/metadata/manifest.ttl");
                if (!manifest_file.is_open()) {
                    return Result<void>::Error("Cannot open LV2 manifest.ttl for writing");
                }
                
                manifest_file << "@prefix lv2:  <http://lv2plug.in/ns/lv2core#> .\n";
                manifest_file << "@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .\n";
                manifest_file << "@prefix doap: <http://usefulinc.com/ns/doap#> .\n\n";
                
                manifest_file << "<" << opts.plugin_id << ">\n";
                manifest_file << "    a lv2:Plugin ;\n";
                manifest_file << "    lv2:binary <" << opts.plugin_name << "_lv2.so> ;\n";
                manifest_file << "    rdfs:label \"" << opts.plugin_name << "\" ;\n";
                manifest_file << "    rdfs:comment \"ProtoVM generated plugin\" .\n";
                
                manifest_file.close();
                
                // Write plugin-specific TTL file
                std::string ttl_filename = opts.plugin_id.substr(opts.plugin_id.find_last_of('/') + 1);
                if (ttl_filename.empty()) ttl_filename = opts.plugin_id;  // Fallback if no slash found
                
                std::ofstream ttl_file(std::string(output_dir) + "/metadata/" + ttl_filename + ".ttl");
                if (!ttl_file.is_open()) {
                    return Result<void>::Error("Cannot open LV2 plugin TTL file for writing");
                }
                
                ttl_file << "@prefix lv2:  <http://lv2plug.in/ns/lv2core#> .\n";
                ttl_file << "@prefix atom: <http://lv2plug.in/ns/ext/atom#> .\n";
                ttl_file << "@prefix rdf:  <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .\n";
                ttl_file << "@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .\n\n";
                
                ttl_file << "<" << opts.plugin_id << ">\n";
                ttl_file << "    a lv2:InstrumentPlugin ;\n";
                ttl_file << "    lv2:project <" << opts.plugin_id << "> ;\n";
                ttl_file << "    lv2:name \"" << opts.plugin_name << "\" ;\n";
                ttl_file << "    lv2:optionalFeature lv2:hardRTCapable ;\n";
                ttl_file << "    lv2:port [\n";
                ttl_file << "        a lv2:AudioPort , lv2:OutputPort ;\n";
                ttl_file << "        lv2:index 0 ;\n";
                ttl_file << "        lv2:symbol \"out_left\" ;\n";
                ttl_file << "        lv2:name \"Left Output\" ;\n";
                ttl_file << "    ] , [\n";
                ttl_file << "        a lv2:AudioPort , lv2:OutputPort ;\n";
                ttl_file << "        lv2:index 1 ;\n";
                ttl_file << "        lv2:symbol \"out_right\" ;\n";
                ttl_file << "        lv2:name \"Right Output\" ;\n";
                ttl_file << "    ] .\n";
                
                ttl_file.close();
                break;
            }
            
            case PluginTargetKind::Clap: {
                // Write CLAP manifest JSON
                std::ofstream manifest_file(std::string(output_dir) + "/metadata/clap_manifest.json");
                if (!manifest_file.is_open()) {
                    return Result<void>::Error("Cannot open CLAP manifest.json for writing");
                }
                
                manifest_file << "{\n";
                manifest_file << "  \"clap-version\": \"1.1.8\",\n";
                manifest_file << "  \"name\": \"" << opts.plugin_name << "\",\n";
                manifest_file << "  \"id\": \"" << opts.plugin_id << "\",\n";
                manifest_file << "  \"version\": \"" << opts.version << "\",\n";
                manifest_file << "  \"url\": \"https://github.com/protovm\",\n";
                manifest_file << "  \"manual-url\": \"https://github.com/protovm/manual\",\n";
                manifest_file << "  \"support-url\": \"https://github.com/protovm/support\",\n";
                manifest_file << "  \"description\": \"ProtoVM generated plugin\",\n";
                manifest_file << "  \"creator\": \"" << opts.vendor << "\",\n";
                manifest_file << "  \"website\": \"https://github.com/protovm\",\n";
                manifest_file << "  \"type\": [\"instrument\", \"stereo\"],\n";
                manifest_file << "  \"features\": [\"instrument\", \"stereo\"]\n";
                manifest_file << "}\n";
                
                manifest_file.close();
                break;
            }
            
            default:
                // VST3 and LADSPA don't typically require separate metadata files
                break;
        }
    } catch (const std::exception& e) {
        return Result<void>::Error(String("Failed to write metadata files: ") + e.what());
    }
    
    return Result<void>::Success();
}

Result<void> PluginProjectExport::WriteReadme(
    const String& readme_path,
    const PluginProjectExportOptions& opts
) {
    std::ostringstream readme_content;
    
    readme_content << "# " << opts.plugin_name << " Plugin\n\n";
    readme_content << "This is a " << opts.plugin_id << " plugin generated by ProtoVM.\n\n";
    
    // Describe the plugin format
    String format_name;
    switch (opts.target) {
        case PluginTargetKind::Vst3:
            format_name = "VST3";
            break;
        case PluginTargetKind::Lv2:
            format_name = "LV2";
            break;
        case PluginTargetKind::Clap:
            format_name = "CLAP";
            break;
        case PluginTargetKind::Ladspa:
            format_name = "LADSPA";
            break;
    }
    
    readme_content << "## Format\n";
    readme_content << "This plugin follows the " << format_name << " specification and is intended for use in " << format_name << " compatible hosts.\n\n";
    
    readme_content << "## Project Structure\n";
    readme_content << "The project structure is organized as follows:\n";
    readme_content << "- `src/` - Contains plugin wrapper source code\n";
    readme_content << "- `metadata/` - Contains format-specific metadata files (e.g., LV2 TTL, CLAP manifest)\n";
    readme_content << "- `CMakeLists.txt` - Build configuration for CMake\n";
    readme_content << "- `README.md` - This file\n\n";
    
    readme_content << "## Building the Plugin\n";
    readme_content << "1. Navigate to the project directory\n";
    readme_content << "2. Create a build directory: `mkdir build && cd build`\n";
    readme_content << "3. Configure the build: `cmake ..`\n";
    readme_content << "4. Compile the plugin: `make` (or `cmake --build .`)\n\n";
    
    readme_content << "## Installing the Plugin\n";
    readme_content << "After building, install the plugin to your DAW's plugin directory:\n";
    switch (opts.target) {
        case PluginTargetKind::Vst3:
            readme_content << "Place the .vst3 bundle in your VST3 plugin directory (typically `~/VST3/` or `Program Files/Common Files/VST3/`).\n";
            break;
        case PluginTargetKind::Lv2:
            readme_content << "Place the generated .so file and associated .ttl files in your LV2 plugin directory (typically `~/.lv2/` or `/usr/lib/lv2/`).\n";
            break;
        case PluginTargetKind::Clap:
            readme_content << "Place the .clap bundle in your CLAP plugin directory (typically `~/CLAP/` or `Program Files/CLAP/`).\n";
            break;
        case PluginTargetKind::Ladspa:
            readme_content << "Place the .so file in your LADSPA plugin directory (typically `~/.ladspa/` or `/usr/lib/ladspa/`).\n";
            break;
    }
    readme_content << "\n\n";
    
    readme_content << "## Dependencies\n";
    readme_content << "This plugin uses the ProtoVM Audio Engine C ABI. You may need to link against the compiled ProtoVM engine library.\n";
    readme_content << "Additionally, you'll need to add the relevant SDK includes and libraries for " << format_name << " development.\n\n";
    
    readme_content << "## Notes\n";
    readme_content << "This is a generated plugin wrapper that connects your instrument to the " << format_name << " host.\n";
    readme_content << "To fully implement a production-ready plugin, you may need to:\n";
    readme_content << "- Add proper parameter mapping for the instrument controls\n";
    readme_content << "- Implement GUI support if required\n";
    readme_content << "- Add proper licensing information\n";
    readme_content << "- Test with various host applications\n\n";
    
    try {
        std::ofstream file(readme_path);
        if (!file.is_open()) {
            return Result<void>::Error("Cannot open README.md for writing: " + readme_path);
        }
        file << readme_content.str();
        file.close();
    } catch (const std::exception& e) {
        return Result<void>::Error(String("Failed to write README.md: ") + e.what());
    }
    
    return Result<void>::Success();
}

Result<void> PluginProjectExport::ExportPluginProject(
    const ProtoVMCLI::InstrumentGraph& instrument,
    const PluginProjectExportOptions& opts
) {
    // Create the directory structure
    auto dir_result = CreateDirectories(opts.output_dir);
    if (dir_result.IsError()) {
        return dir_result;
    }
    
    // Write the wrapper source file
    String wrapper_path = opts.output_dir + "/src/PluginWrapper.cpp";
    auto wrapper_result = WriteWrapperSource(wrapper_path, opts);
    if (wrapper_result.IsError()) {
        return wrapper_result;
    }
    
    // Write build files if requested
    if (opts.emit_build_files) {
        auto build_result = WriteBuildFiles(opts.output_dir, opts);
        if (build_result.IsError()) {
            return build_result;
        }
    }
    
    // Write metadata files
    auto metadata_result = WriteMetadataFiles(opts.output_dir, opts);
    if (metadata_result.IsError()) {
        return metadata_result;
    }
    
    // Write README if requested
    if (opts.emit_readme) {
        String readme_path = opts.output_dir + "/README.md";
        auto readme_result = WriteReadme(readme_path, opts);
        if (readme_result.IsError()) {
            return readme_result;
        }
    }
    
    // Delegate to specific target exporters
    switch (opts.target) {
        case PluginTargetKind::Vst3:
            return ExportVst3Project(instrument, opts);
        case PluginTargetKind::Lv2:
            return ExportLv2Project(instrument, opts);
        case PluginTargetKind::Clap:
            return ExportClapProject(instrument, opts);
        case PluginTargetKind::Ladspa:
            return ExportLadspaProject(instrument, opts);
        default:
            return Result<void>::Error("Unsupported plugin target kind");
    }
}

Result<void> PluginProjectExport::ExportVst3Project(
    const ProtoVMCLI::InstrumentGraph& instrument,
    const PluginProjectExportOptions& opts
) {
    // Additional VST3-specific files can be written here
    // For now, we just return success as the basic files are already generated
    return Result<void>::Success();
}

Result<void> PluginProjectExport::ExportLv2Project(
    const ProtoVMCLI::InstrumentGraph& instrument,
    const PluginProjectExportOptions& opts
) {
    // Additional LV2-specific files are handled in WriteMetadataFiles
    return Result<void>::Success();
}

Result<void> PluginProjectExport::ExportClapProject(
    const ProtoVMCLI::InstrumentGraph& instrument,
    const PluginProjectExportOptions& opts
) {
    // Additional CLAP-specific files are handled in WriteMetadataFiles
    return Result<void>::Success();
}

Result<void> PluginProjectExport::ExportLadspaProject(
    const ProtoVMCLI::InstrumentGraph& instrument,
    const PluginProjectExportOptions& opts
) {
    // Additional LADSPA-specific files can be written here
    return Result<void>::Success();
}