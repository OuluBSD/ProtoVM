#include "ProtoVM.h"
#include "PslParser.h"

#include <Core/Core.h>
using namespace UPP;

// Example usage of the PSL parser
void TestPslParser() {
    String psl_code = 
        "circuit simple_nand:\n"
        "    component nand1: nand\n"
        "    component vcc: vcc\n"
        "    component gnd: ground\n"
        "    \n"
        "    connect vcc -- nand1.input_a\n"
        "    connect gnd -- nand1.input_b\n";
    
    // Tokenize the PSL code
    PslTokenizer tokenizer(psl_code);
    std::vector<Token> tokens = tokenizer.Tokenize();
    
    // Parse the tokens into an AST
    PslParser parser;
    parser.SetTokens(tokens);
    std::vector<PslNode*> ast = parser.Parse();
    
    // Find the circuit node
    CircuitNode* circuit = nullptr;
    for (PslNode* node : ast) {
        if (CircuitNode* c = dynamic_cast<CircuitNode*>(node)) {
            circuit = c;
            break;
        }
    }
    
    if (circuit) {
        // Compile the circuit to C++ code
        PslCompiler compiler;
        String cpp_code = compiler.CompileCircuit(circuit);
        
        LOG("Generated C++ code:");
        LOG(AsString(cpp_code));
        
        // In a real application, you would save this to a file and
        // potentially compile it or process it further
    } else {
        LOG("No circuit found in PSL code");
    }
    
    // Clean up allocated nodes
    for (PslNode* node : ast) {
        delete node;
    }
}