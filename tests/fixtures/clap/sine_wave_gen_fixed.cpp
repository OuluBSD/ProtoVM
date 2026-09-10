#include <math.h>
#include <string.h>
#include <stdlib.h>

// Minimal CLAP test plugin - sine wave generator
// This is a simplified implementation for testing purposes

#define CLAP_VERSION_MAJOR 1
#define CLAP_VERSION_MINOR 1
#define CLAP_VERSION_REVISION 0

typedef struct clap_version {
    uint32_t major;
    uint32_t minor;
    uint32_t revision;
} clap_version;

static const clap_version CLAP_VERSION = {CLAP_VERSION_MAJOR, CLAP_VERSION_MINOR, CLAP_VERSION_REVISION};

#define CLAP_NAME "sine_wave_gen"
#define CLAP_VENDOR "ProtoVM"
#define CLAP_URL "https://github.com/protovm"
#define CLAP_MANUAL_URL "https://github.com/protovm/manual"
#define CLAP_SUPPORT_URL "https://github.com/protovm/support"
#define CLAP_ID "com.protovm.sine_wave_gen"

// Basic types
typedef uint32_t clap_id;
typedef bool clap_bool;

// Minimal clap_host structure for our test
typedef struct clap_host {
    clap_version clap_version;
    void *host_data;
    const char *name;
    const char *vendor;
    const char *url;
    const char *version;
    
    // Minimal callbacks
    void (*request_restart)(const struct clap_host *host);
    void (*request_process)(const struct clap_host *host);
    void (*request_callback)(const struct clap_host *host);
    
    const void* (*get_extension)(const struct clap_host *host, const char *extension_id);
} clap_host;

// Plugin descriptor
typedef struct clap_plugin_descriptor {
    clap_version clap_version;
    const char *id;          // required
    const char *name;        // required
    const char *vendor;
    const char *url;
    const char *manual_url;
    const char *support_url;
    const char *version;     // required
    const char *description; // optional
    const char **features;   // null terminated array, optional
} clap_plugin_descriptor;

// Define our plugin descriptor
static const clap_plugin_descriptor sine_wave_descriptor = {
    .clap_version = CLAP_VERSION,
    .id = CLAP_ID,
    .name = "Sine Wave Generator",
    .vendor = CLAP_VENDOR,
    .url = CLAP_URL,
    .manual_url = CLAP_MANUAL_URL,
    .support_url = CLAP_SUPPORT_URL,
    .version = "1.0.0",
    .description = "Simple sine wave generator for testing",
    .features = NULL // Simplified for our test
};

// Audio buffer structure
typedef struct clap_audio_buffer {
    float **data32;
    double **data64;
    uint32_t channel_count;
    uint32_t constant_mask;
    uint32_t latency;
} clap_audio_buffer;

// Process structure
typedef enum clap_process_status {
    CLAP_PROCESS_CONTINUE = 0,
    CLAP_PROCESS_TAIL,
    CLAP_PROCESS_SLEEP,
    CLAP_PROCESS_ERROR,
} clap_process_status;

typedef struct clap_process {
    // Time info, -1 if not available
    int64_t steady_time;
    
    // Number of frames to process
    uint32_t frame_count;
    
    // Per-audio-port array of buffers.
    // The host does not mix the audio ports, the plugin receives the raw input buffers
    const struct clap_audio_buffer *audio_inputs; // [num_audio_inputs]
    uint32_t                   audio_input_count;
    
    struct clap_audio_buffer *audio_outputs; // [num_audio_outputs]
    uint32_t             audio_output_count;
    
    // Event structures are complex, for simplicity we'll skip them in this test
    const void *in_events;  // Simplified
    void *out_events;       // Simplified
    
    const void *transport; // Simplified
} clap_process;

// Our plugin instance structure
typedef struct {
    uint32_t sample_rate;
    
    // Sine wave parameters
    double phase;
    double frequency;
} SineWavePlugin;

// Define the plugin structure
typedef struct clap_plugin {
    const clap_plugin_descriptor *desc;
    const clap_host *host;
    
    void (*init)(const struct clap_plugin *plugin);
    void (*destroy)(const struct clap_plugin *plugin);
    bool (*activate)(const struct clap_plugin *plugin, 
                     uint32_t sample_rate, 
                     uint32_t min_frames_count, 
                     uint32_t max_frames_count);
    void (*deactivate)(const struct clap_plugin *plugin);
    void (*reset)(const struct clap_plugin *plugin);
    clap_process_status (*process)(const struct clap_plugin *plugin, 
                                   const struct clap_process *process);
    const void* (*get_extension)(const struct clap_plugin *plugin, 
                                const char *id);
} clap_plugin;

// Forward declarations for plugin methods
static void clap_plugin_destroy(const clap_plugin *plugin);
static bool clap_plugin_activate(const clap_plugin *plugin, 
                                 uint32_t sample_rate, 
                                 uint32_t min_frames_count, 
                                 uint32_t max_frames_count);
static void clap_plugin_deactivate(const clap_plugin *plugin);
static void clap_plugin_reset(const clap_plugin *plugin);
static clap_process_status clap_plugin_process(const clap_plugin *plugin, 
                                              const clap_process *process);
static const void* clap_plugin_get_extension(const clap_plugin *plugin, 
                                            const char *id);

static const clap_plugin sine_wave_plugin_template = {
    .desc = &sine_wave_descriptor,
    .init = NULL,  // No initialization needed
    .destroy = clap_plugin_destroy,
    .activate = clap_plugin_activate,
    .deactivate = clap_plugin_deactivate,
    .reset = clap_plugin_reset,
    .process = clap_plugin_process,
    .get_extension = clap_plugin_get_extension,
};

// Create plugin function
static const clap_plugin* clap_plugin_create(const clap_host *host, const char *plugin_id) {
    if (strcmp(plugin_id, CLAP_ID) != 0) {
        return NULL;
    }
    
    SineWavePlugin *plugin_data = (SineWavePlugin*)calloc(1, sizeof(SineWavePlugin));
    if (!plugin_data) {
        return NULL;
    }
    
    // Initialize plugin data
    plugin_data->sample_rate = 48000;  // Default sample rate
    plugin_data->frequency = 440.0;    // Default to A4
    plugin_data->phase = 0.0;
    
    // Create a plugin instance with the correct vtable
    clap_plugin *instance = (clap_plugin*)malloc(sizeof(clap_plugin));
    if (!instance) {
        free(plugin_data);
        return NULL;
    }
    
    *instance = sine_wave_plugin_template;
    instance->desc = &sine_wave_descriptor;
    instance->host = host;
    
    // Store our plugin data in the host_data field (or use a more sophisticated approach)
    // For this test, we'll cast the plugin_data to host_data
    ((clap_plugin*)instance)->host->host_data = plugin_data;
    
    return instance;
}

// Helper function to get plugin data from plugin
static SineWavePlugin* get_plugin_data(const clap_plugin *plugin) {
    return (SineWavePlugin*)plugin->host->host_data;
}

// Implement plugin methods
static void clap_plugin_destroy(const clap_plugin *plugin) {
    SineWavePlugin *plugin_data = get_plugin_data(plugin);
    if (plugin_data) {
        free(plugin_data);
    }
    free((void*)plugin);
}

static bool clap_plugin_activate(const clap_plugin *plugin, 
                                 uint32_t sample_rate, 
                                 uint32_t min_frames_count, 
                                 uint32_t max_frames_count) {
    SineWavePlugin *plugin_data = get_plugin_data(plugin);
    if (plugin_data) {
        plugin_data->sample_rate = sample_rate;
        plugin_data->phase = 0.0;
        return true;
    }
    return false;
}

static void clap_plugin_deactivate(const clap_plugin *plugin) {
    SineWavePlugin *plugin_data = get_plugin_data(plugin);
    if (plugin_data) {
        plugin_data->sample_rate = 0;
    }
}

static void clap_plugin_reset(const clap_plugin *plugin) {
    SineWavePlugin *plugin_data = get_plugin_data(plugin);
    if (plugin_data) {
        plugin_data->phase = 0.0;
    }
}

static clap_process_status clap_plugin_process(const clap_plugin *plugin, 
                                              const clap_process *process) {
    SineWavePlugin *plugin_data = get_plugin_data(plugin);
    
    if (!plugin_data || !process || !process->audio_outputs || process->audio_output_count == 0) {
        return CLAP_PROCESS_CONTINUE;
    }
    
    // For our simple test, assume stereo output if available
    uint32_t channels = 1; // Default to mono
    if (process->audio_outputs[0].channel_count >= 2) {
        channels = 2; // Stereo
    }
    
    // Generate sine wave in output buffers
    for (uint32_t frame = 0; frame < process->frame_count; ++frame) {
        // Generate sine wave at the specified frequency
        float sample = (float)(sin(plugin_data->phase) * 0.5);  // 0.5 amplitude
        
        // Write to all output channels (stereo typically has 2)
        for (uint32_t ch = 0; ch < channels; ++ch) {
            if (process->audio_outputs[0].data32 && 
                process->audio_outputs[0].data32[ch]) {
                process->audio_outputs[0].data32[ch][frame] = sample;
            }
        }
        
        // Update phase for next sample
        plugin_data->phase += 2.0 * M_PI * plugin_data->frequency / plugin_data->sample_rate;
        
        // Keep phase in [0, 2π] range
        while (plugin_data->phase > 2.0 * M_PI) {
            plugin_data->phase -= 2.0 * M_PI;
        }
    }
    
    return CLAP_PROCESS_CONTINUE;
}

static const void* clap_plugin_get_extension(const clap_plugin *plugin, 
                                            const char *id) {
    // No extensions implemented for this simple test plugin
    return NULL;
}

// Factory implementation
static uint32_t plugin_factory_get_plugin_count(const void *factory) {
    return 1;  // We have one plugin
}

static const clap_plugin_descriptor *plugin_factory_get_plugin_descriptor(
    const void *factory, uint32_t index) {
    if (index == 0) {
        return &sine_wave_descriptor;
    }
    return NULL;
}

static const clap_plugin *plugin_factory_create_plugin(
    const void *factory,
    const clap_host *host,
    const char *plugin_id) {
    return clap_plugin_create(host, plugin_id);
}

static const struct clap_plugin_factory {
    uint32_t (*get_plugin_count)(const void *factory);
    const clap_plugin_descriptor_t *(*get_plugin_descriptor)(const void *factory, uint32_t index);
    const clap_plugin_t *(*create_plugin)(const void *factory,
                                          const clap_host_t *host,
                                          const char *plugin_id);
} plugin_factory = {
    .get_plugin_count = plugin_factory_get_plugin_count,
    .get_plugin_descriptor = plugin_factory_get_plugin_descriptor,
    .create_plugin = plugin_factory_create_plugin,
};

// Entry point
static bool clap_entry_init(const char *plugin_path) {
    // Initialization code if needed
    return true;
}

static void clap_entry_deinit(void) {
    // Cleanup code if needed
}

static const void* clap_entry_get_factory(const char *factory_id) {
    if (strcmp(factory_id, CLAP_PLUGIN_FACTORY_ID) == 0) {
        return &plugin_factory;
    }
    return NULL;
}

#define CLAP_PLUGIN_FACTORY_ID "clap.plugin-factory/2"

// Define the entry point function
static const struct clap_plugin_entry {
    clap_version clap_version;
    bool (*init)(const char *plugin_path);
    void (*deinit)(void);
    const void* (*get_factory)(const char *factory_id);
} clap_entry = {
    .clap_version = CLAP_VERSION,
    .init = clap_entry_init,
    .deinit = clap_entry_deinit,
    .get_factory = clap_entry_get_factory,
};

// Export the entry point function
extern "C" {
    const struct clap_plugin_entry *clap_entry() {
        return &clap_entry;
    }
}