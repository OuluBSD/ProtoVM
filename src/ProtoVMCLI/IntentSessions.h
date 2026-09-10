#ifndef _ProtoVM_IntentSessions_h_
#define _ProtoVM_IntentSessions_h_

#ifdef USE_UPP
    #include <ProtoVM/ProtoVM.h>  // Include U++ types
#else
    #include "../ProtoVM/MockProtoVM.h"  // Include mock types
#endif  // Include U++ types
#include "SessionTypes.h"
#include "InstrumentGraph.h"
#include "MusicalIntent.h"
#include <string>
#include <vector>
#include <chrono>

namespace ProtoVMCLI {

enum class IntentSessionStatus {
    Created,
    Active,
    Closed
};

struct IntentSessionMetadata {
    Upp::String intent_session_id;     // stable id, e.g. "is-000001"
    Upp::String created_at;
    Upp::String created_with;          // version string
    Upp::String current_branch;        // branch this session is attached to
    int instrument_revision = 0;  // increments per accepted step
    int head_step_index = -1;     // current applied step index (for undo/redo)
    IntentSessionStatus status = IntentSessionStatus::Active;

    IntentSessionMetadata() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::tm tm = *std::gmtime(&time_t);
        char buffer[32];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &tm);
        created_at = buffer;
        created_with = "ProtoVM-v0.1";  // TODO: Get actual version
    }
};

struct IntentStepRecord {
    int step_index = 0;
    Upp::String user_id;               // optional caller id
    IntentRequest request;
    IntentPlan plan;              // includes before/after QA reports + diff + accepted
    bool applied = false;         // true if applied to current instrument state
    Upp::String note;                  // optional user note
};

struct IntentSessionState {
    IntentSessionMetadata meta;

    // Current working instrument state for this intent session:
    InstrumentGraph current_instrument;

    // History:
    Vector<IntentStepRecord> steps;
};

class IntentSessionManager {
public:
    static Result<IntentSessionState> Create(
        const String& session_dir,
        const String& branch,
        const InstrumentGraph& initial_instrument,
        const String& user_id
    );

    static Result<IntentSessionState> Load(
        const String& session_dir,
        const String& intent_session_id
    );

    static Result<IntentSessionState> ProposeStep(
        IntentSessionState& st,
        const IntentRequest& req,
        const String& user_id
    );

    static Result<IntentSessionState> ApplyStep(
        IntentSessionState& st,
        int step_index,
        bool require_accepted
    );

    static Result<IntentSessionState> Undo(
        IntentSessionState& st
    );

    static Result<IntentSessionState> Redo(
        IntentSessionState& st
    );

    static Result<InstrumentGraph> ReplayStepsToInstrument(
        const IntentSessionState& st,
        const InstrumentGraph& target_initial,
        int up_to_step_index,           // inclusive, -1 = all applied steps
        bool require_accepted
    );

private:
    // Internal helpers for persistence
    static Result<void> SaveSession(
        const IntentSessionState& session_state,
        const String& session_dir
    );

    static Result<void> SaveStep(
        const IntentStepRecord& step_record,
        const String& session_dir,
        const String& intent_session_id
    );

    static Result<IntentStepRecord> LoadStep(
        const String& session_dir,
        const String& intent_session_id,
        int step_index
    );
};

} // namespace ProtoVMCLI

#endif // _ProtoVM_IntentSessions_h_