#include "Transformations.h"
#include "CircuitFacade.h"
#include "JsonIO.h"
#include <iostream>
#include <cassert>

void TestTransformationDataStructures() {
    std::cout << "Testing Transformation data structures..." << std::endl;
    
    // Test basic structure creation
    TransformationPlan plan;
    plan.id = "TEST_PLAN_1";
    plan.kind = TransformationKind::SimplifyDoubleInversion;
    
    TransformationTarget target;
    target.subject_id = "COMP_1";
    target.subject_kind = "Component";
    plan.target = target;
    
    plan.guarantees.Add(PreservationLevel::BehaviorKindPreserved);
    plan.guarantees.Add(PreservationLevel::IOContractPreserved);
    
    TransformationStep step;
    step.description = "Test transformation step";
    plan.steps.Add(step);
    
    // Test JSON serialization
    Upp::ValueMap plan_map = JsonIO::TransformationPlanToValueMap(plan);
    Upp::ValueArray plans_array = JsonIO::TransformationPlansToValueArray(Upp::Vector<TransformationPlan>({plan}));
    
    assert(!plan_map.IsEmpty());
    assert(plans_array.GetCount() == 1);
    
    std::cout << "✓ Transformation data structures test passed" << std::endl;
}

void TestTransformationKindSerialization() {
    std::cout << "Testing TransformationKind serialization..." << std::endl;
    
    // Test each transformation kind
    assert(JsonIO::TransformationKindToJson(TransformationKind::SimplifyDoubleInversion).ToString() == "SimplifyDoubleInversion");
    assert(JsonIO::TransformationKindToJson(TransformationKind::SimplifyRedundantGate).ToString() == "SimplifyRedundantGate");
    assert(JsonIO::TransformationKindToJson(TransformationKind::ReplaceWithKnownBlock).ToString() == "ReplaceWithKnownBlock");
    assert(JsonIO::TransformationKindToJson(TransformationKind::RewireFanoutTree).ToString() == "RewireFanoutTree");
    assert(JsonIO::TransformationKindToJson(TransformationKind::MergeEquivalentBlocks).ToString() == "MergeEquivalentBlocks");
    
    std::cout << "✓ TransformationKind serialization test passed" << std::endl;
}

void TestPreservationLevelSerialization() {
    std::cout << "Testing PreservationLevel serialization..." << std::endl;
    
    // Test each preservation level
    assert(JsonIO::PreservationLevelToJson(PreservationLevel::BehaviorKindPreserved).ToString() == "BehaviorKindPreserved");
    assert(JsonIO::PreservationLevelToJson(PreservationLevel::IOContractPreserved).ToString() == "IOContractPreserved");
    assert(JsonIO::PreservationLevelToJson(PreservationLevel::DependencyPatternPreserved).ToString() == "DependencyPatternPreserved");
    
    std::cout << "✓ PreservationLevel serialization test passed" << std::endl;
}

// Simple test for the transformation engine (without full functionality)
void TestTransformationEngineCreation() {
    std::cout << "Testing TransformationEngine creation..." << std::endl;
    
    TransformationEngine engine;
    // Just test that we can create the object without errors
    
    std::cout << "✓ TransformationEngine creation test passed" << std::endl;
}

int main() {
    std::cout << "Running Transformations module tests..." << std::endl;
    
    TestTransformationDataStructures();
    TestTransformationKindSerialization();
    TestPreservationLevelSerialization();
    TestTransformationEngineCreation();
    
    std::cout << "All Transformations tests passed!" << std::endl;
    return 0;
}