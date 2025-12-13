#ifndef INSTRUMENT_ENGINE_FACTORY_H
#define INSTRUMENT_ENGINE_FACTORY_H

#include "AudioEngineCAbi.h"
#include "InstrumentGraph.h"
#include "Result.h"  // Assuming Result<T> is defined somewhere in the codebase

class InstrumentEngineFactory {
public:
    static Result<ProtoVM_AudioEngine*> CreateEngineForInstrument(
        const InstrumentGraph& instrument,
        const ProtoVM_AudioEngineConfig& cfg
    );
};

#endif // INSTRUMENT_ENGINE_FACTORY_H