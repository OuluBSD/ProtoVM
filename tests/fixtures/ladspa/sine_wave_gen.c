#include <math.h>

// Simple sine wave generator LADSPA plugin
// Output a sine wave at 440Hz (A4) for testing

typedef float LADSPA_Data;

#define LADSPA_PORT_INPUT          0x1
#define LADSPA_PORT_OUTPUT         0x2
#define LADSPA_PORT_CONTROL        0x4
#define LADSPA_PORT_AUDIO          0x8

#define LADSPA_IS_PORT_INPUT(x)    ((x) & LADSPA_PORT_INPUT)
#define LADSPA_IS_PORT_OUTPUT(x)   ((x) & LADSPA_PORT_OUTPUT)
#define LADSPA_IS_PORT_CONTROL(x)  ((x) & LADSPA_PORT_CONTROL)
#define LADSPA_IS_PORT_AUDIO(x)    ((x) & LADSPA_PORT_AUDIO)

typedef unsigned long LADSPA_PortDescriptor;
typedef void * LADSPA_Handle;

// Our plugin instance data
typedef struct {
    LADSPA_Data* output;
    unsigned long sample_rate;
    double phase;
    double frequency;
} SineWavePlugin;

// Plugin descriptor structure
static LADSPA_PortDescriptor port_descriptors[2];
static const char* port_names[2] = {
    "Frequency",
    "Output"
};

enum {
    FREQ_PORT = 0,
    OUTPUT_PORT = 1
};

// Port range hints for the control port
static LADSPA_Data port_range_hints[2];

// Plugin descriptor
static const struct _LADSPA_Descriptor {
    const char * Label;
    const char * Name;
    const char * Maker;
    const char * Copyright;
    unsigned long UniqueID;
    unsigned long PortCount;
    const LADSPA_PortDescriptor * PortDescriptors;
    const char ** PortNames;
    const LADSPA_Data * PortRangeHints;
    void (*activate)(LADSPA_Handle Instance);
    void (*cleanup)(LADSPA_Handle Instance);
    void (*connect_port)(LADSPA_Handle Instance, unsigned long Port, LADSPA_Data * DataLocation);
    void (*deactivate)(LADSPA_Handle Instance);
    LADSPA_Handle (*instantiate)(const struct _LADSPA_Descriptor * Descriptor, unsigned long SampleRate);
    void (*run)(LADSPA_Handle Instance, unsigned long SampleCount);
    void (*run_adding)(LADSPA_Handle Instance, unsigned long SampleCount);
    void (*set_run_adding_gain)(LADSPA_Handle Instance, LADSPA_Data Gain);
} sine_wave_descriptor = {
    "sine_wave_gen",
    "Sine Wave Generator",
    "ProtoVM Test Plugin",
    "Public Domain",
    2025,
    2,
    port_descriptors,
    port_names,
    port_range_hints,
    activate,
    cleanup,
    connect_port,
    NULL, // deactivate
    instantiate,
    run,
    NULL, // run_adding
    NULL  // set_run_adding_gain
};

const struct _LADSPA_Descriptor* ladspa_descriptor(unsigned long Index) {
    if (Index == 0) {
        return &sine_wave_descriptor;
    }
    return NULL;
}

void activate(LADSPA_Handle Instance) {
    SineWavePlugin *plugin = (SineWavePlugin *)Instance;
    plugin->phase = 0.0;
}

LADSPA_Handle instantiate(const struct _LADSPA_Descriptor * Descriptor, unsigned long SampleRate) {
    SineWavePlugin *plugin = (SineWavePlugin *)malloc(sizeof(SineWavePlugin));
    if (plugin) {
        plugin->sample_rate = SampleRate;
        plugin->frequency = 440.0f;  // Default to A4
        plugin->phase = 0.0;
    }
    return (LADSPA_Handle)plugin;
}

void connect_port(LADSPA_Handle Instance, unsigned long Port, LADSPA_Data * DataLocation) {
    SineWavePlugin *plugin = (SineWavePlugin *)Instance;
    switch (Port) {
        case FREQ_PORT:
            plugin->frequency = *DataLocation;
            break;
        case OUTPUT_PORT:
            plugin->output = DataLocation;
            break;
    }
}

void run(LADSPA_Handle Instance, unsigned long SampleCount) {
    SineWavePlugin *plugin = (SineWavePlugin *)Instance;
    
    for (unsigned int i = 0; i < SampleCount; i++) {
        // Generate sine wave at the specified frequency
        plugin->output[i] = sin(plugin->phase) * 0.5f;  // 0.5 amplitude
        
        // Update phase for next sample
        plugin->phase += 2.0 * M_PI * plugin->frequency / plugin->sample_rate;
        
        // Keep phase in [0, 2π] range
        if (plugin->phase > 2.0 * M_PI) {
            plugin->phase -= 2.0 * M_PI;
        }
    }
}

void cleanup(LADSPA_Handle Instance) {
    free(Instance);
}

// Initialize port descriptors
__attribute__((constructor))
void init() {
    port_descriptors[FREQ_PORT] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
    port_descriptors[OUTPUT_PORT] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
    
    port_range_hints[FREQ_PORT].LowerBound = 20.0f;   // 20 Hz
    port_range_hints[FREQ_PORT].UpperBound = 20000.0f; // 20 kHz
    port_range_hints[FREQ_PORT].DefaultValue = 440.0f; // A4
}