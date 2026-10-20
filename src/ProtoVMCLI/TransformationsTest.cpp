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

    plan.guarantees.push_back(PreservationLevel::BehaviorKindPreserved);
    plan.guarantees.push_back(PreservationLevel::IOContractPreserved);

    TransformationStep step;
    step.description = "Test transformation step";
    plan.steps.push_back(step);

    // Test JSON serialization
    Upp::ValueMap plan_map = ProtoVMCLI::JsonIO::TransformationPlanToValueMap(plan);
    Upp::Vector<TransformationPlan> plans_vector;
    plans_vector.Add(plan);
    Upp::ValueArray plans_array = ProtoVMCLI::JsonIO::TransformationPlansToValueArray(plans_vector);

    assert(!plan_map.IsEmpty());
    assert(plans_array.GetCount() == 1);

    std::cout << "✓ Transformation data structures test passed" << std::endl;
}

void TestTransformationKindSerialization() {
    std::cout << "Testing TransformationKind serialization..." << std::endl;

    // Test each transformation kind
    assert(ProtoVMCLI::JsonIO::TransformationKindToJson(TransformationKind::SimplifyDoubleInversion).ToString() == "SimplifyDoubleInversion");
    assert(ProtoVMCLI::JsonIO::TransformationKindToJson(TransformationKind::SimplifyRedundantGate).ToString() == "SimplifyRedundantGate");
    assert(ProtoVMCLI::JsonIO::TransformationKindToJson(TransformationKind::ReplaceWithKnownBlock).ToString() == "ReplaceWithKnownBlock");
    assert(ProtoVMCLI::JsonIO::TransformationKindToJson(TransformationKind::RewireFanoutTree).ToString() == "RewireFanoutTree");
    assert(ProtoVMCLI::JsonIO::TransformationKindToJson(TransformationKind::MergeEquivalentBlocks).ToString() == "MergeEquivalentBlocks");

    std::cout << "✓ TransformationKind serialization test passed" << std::endl;
}

void TestPreservationLevelSerialization() {
    std::cout << "Testing PreservationLevel serialization..." << std::endl;

    // Test each preservation level
    assert(ProtoVMCLI::JsonIO::PreservationLevelToJson(PreservationLevel::BehaviorKindPreserved).ToString() == "BehaviorKindPreserved");
    assert(ProtoVMCLI::JsonIO::PreservationLevelToJson(PreservationLevel::IOContractPreserved).ToString() == "IOContractPreserved");
    assert(ProtoVMCLI::JsonIO::PreservationLevelToJson(PreservationLevel::DependencyPatternPreserved).ToString() == "DependencyPatternPreserved");

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
