// Simple test to verify the headers compile correctly
#include "src/ProtoVMCLI/AutomationExport.h"

int main() {
    // Just verify that we can instantiate the basic types
    ProtoVMCLI::AutomationPoint point;
    point.time_sec = 1.0;
    point.value = 0.5;
    
    ProtoVMCLI::AutomationLane lane;
    lane.param_name = "test_param";
    lane.points.Add(point);
    
    ProtoVMCLI::AutomationClip clip;
    clip.clip_id = "test_clip";
    clip.lanes.Add(lane);
    
    // Verify we can access the enum
    ProtoVMCLI::AutomationTargetKind target = ProtoVMCLI::AutomationTargetKind::GenericJson;
    
    return 0;
}