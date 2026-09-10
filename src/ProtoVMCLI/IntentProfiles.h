#ifndef _ProtoVM_IntentProfiles_h_
#define _ProtoVM_IntentProfiles_h_

#ifdef USE_UPP
    #include <ProtoVM/ProtoVM.h>  // Include U++ types
#else
    #include "../ProtoVM/MockProtoVM.h"  // Include mock types
#endif  // Include U++ types
#include "SessionTypes.h"
#include "InstrumentGraph.h"
#include "MusicalIntent.h"
#include "IntentSessions.h"
#include <vector>
#include <string>

namespace ProtoVMCLI {

enum class IntentProfileScope {
    Global,        // reusable across projects
    Workspace      // local to a workspace
};

struct IntentProfileMetadata {
    Upp::String profile_id;           // e.g. "ip-warm-analog-pad"
    Upp::String name;                 // human-readable
    Upp::String description;
    IntentProfileScope scope;
    Upp::String created_at;
    Upp::String created_with;
    Upp::String author;               // optional
    Vector<Upp::String> tags;         // e.g. ["pad", "analog", "warm"]

    IntentProfileMetadata() {
        // Set default values
        scope = IntentProfileScope::Workspace;
        auto now = std::time(nullptr);
        char buffer[100];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&now));
        created_at = buffer;
        created_with = "ProtoVM-v0.1";  // TODO: Get actual version
    }
};

struct IntentProfileStep {
    IntentRequest request;             // same structure as Phase 29
    Upp::String note;                  // optional description
};

struct IntentProfile {
    IntentProfileMetadata meta;
    Vector<IntentProfileStep> steps;
};

class IntentProfileManager {
public:
    static Result<IntentProfile> CreateProfile(
        const IntentProfileMetadata& meta,
        const Vector<IntentProfileStep>& steps,
        const String& base_dir
    );

    static Result<IntentProfile> LoadProfile(
        const String& base_dir,
        const String& profile_id
    );

    static Result<Vector<IntentProfileMetadata>> ListProfiles(
        const String& base_dir,
        Optional<String> tag_filter = Null
    );

    static Result<IntentSessionState> ApplyProfileToSession(
        IntentSessionState& session,
        const IntentProfile& profile,
        const String& user_id,
        double strength_scale = 1.0   // e.g. 0.5 = softer, 1.0 = normal
    );

    static Result<InstrumentGraph> ApplyProfileToInstrument(
        const InstrumentGraph& initial,
        const IntentProfile& profile,
        double strength_scale = 1.0,
        bool require_accepted = true
    );

private:
    // Internal helper for profile persistence
    static Result<void> SaveProfile(
        const IntentProfile& profile,
        const String& base_dir
    );

    // Internal helper for validating profile IDs
    static bool IsValidProfileId(const String& profile_id);
};

// Helper for applying scaled intents
Result<IntentRequest> ScaleIntentRequest(
    const IntentRequest& original,
    double strength_scale
);

} // namespace ProtoVMCLI

#endif // _ProtoVM_IntentProfiles_h_