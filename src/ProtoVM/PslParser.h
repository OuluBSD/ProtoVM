#ifndef _ProtoVM_PslParser_h_
#define _ProtoVM_PslParser_h_

#include "ProtoVM.h"
#include <vector>
#include <string>
#include <map>

// Forward declarations
class PslNode;
class ComponentNode;
class ConnectionNode;
class BusNode;
class ModuleNode;
class CircuitNode;
class TestNode;

// Enum for token types
enum class TokenType {
    UNKNOWN,
    END_OF_FILE,
    IDENTIFIER,
    NUMBER,
    STRING,
    KEYWORD_COMPONENT,
    KEYWORD_CIRCUIT,
    KEYWORD_MODULE,
    KEYWORD_TEST,
    KEYWORD_CONNECT,
    KEYWORD_BUS,
    KEYWORD_STIMULUS,
    KEYWORD_ASSERT,
    KEYWORD_WAIT,
    COLON,
    DASH_ARROW,
    EQUALS,
    COMMA,
    LPAREN,
    RPAREN,
    INDENT,
    DEDENT
};

// Token structure
struct Token {
    TokenType type;
    String value;
    int line;
    int column;
    
    Token() : type(TokenType::UNKNOWN), line(0), column(0) {}  // Default constructor
    Token(TokenType t, String v, int l, int c) : type(t), value(v), line(l), column(c) {}
};

// Base class for all PSL AST nodes
class PslNode {
public:
    virtual ~PslNode() {}
    virtual String GetNodeType() const = 0;
};

// Represents a component definition in the schematic
class ComponentNode : public PslNode {
public:
    String name;
    String type_name;
    std::map<String, String> parameters;
    
    ComponentNode(String n, String t, std::map<String, String> p = {}) 
        : name(n), type_name(t), parameters(p) {}
    
    String GetNodeType() const override { return "ComponentNode"; }
};

// Represents a connection between components
class ConnectionNode : public PslNode {
public:
    String source;
    String destination;
    
    ConnectionNode(String s, String d) : source(s), destination(d) {}
    
    String GetNodeType() const override { return "ConnectionNode"; }
};

// Represents a bus definition
class BusNode : public PslNode {
public:
    String name;
    int width;
    
    BusNode(String n, int w) : name(n), width(w) {}
    
    String GetNodeType() const override { return "BusNode"; }
};

// Represents a module definition
class ModuleNode : public PslNode {
public:
    String name;
    std::vector<PslNode*> children;
    
    ModuleNode(String n) : name(n) {}
    
    String GetNodeType() const override { return "ModuleNode"; }
};

// Represents a circuit definition
class CircuitNode : public PslNode {
public:
    String name;
    std::vector<PslNode*> children;
    
    CircuitNode(String n) : name(n) {}
    
    String GetNodeType() const override { return "CircuitNode"; }
};

// Represents a test definition
class TestNode : public PslNode {
public:
    String name;
    String circuit_under_test;
    std::vector<std::map<String, String>> stimulus; // Each step is a map of signal->value
    std::vector<std::map<String, String>> expected; // Expected values for validation
    
    TestNode(String n, String c) : name(n), circuit_under_test(c) {}
    
    String GetNodeType() const override { return "TestNode"; }
};

// Tokenizer for PSL
class PslTokenizer {
private:
    String input;
    int pos;
    int line;
    int column;
    
public:
    PslTokenizer(String input);
    Token NextToken();
    std::vector<Token> Tokenize();
    
private:
    char Peek(int offset = 0) const;
    char Current() const { return Peek(0); }
    void Advance(int count = 1);
    bool IsAtEnd() const { return pos >= input.GetLength(); }
    Token MakeToken(TokenType type, String value);
    
    // Helper functions for token recognition
    Token ReadIdentifier();
    Token ReadNumber();
    Token ReadString();
    TokenType GetKeywordType(String value) const;
};

// Parser for PSL
class PslParser {
private:
    std::vector<Token> tokens;
    int current_pos;
    Token current_token;
    
public:
    PslParser() : current_pos(0), current_token() {}
    
    void SetTokens(const std::vector<Token>& tokens);
    std::vector<PslNode*> Parse();
    
private:
    void Advance();
    bool Check(TokenType type);
    bool Match(TokenType type);
    Token Expect(TokenType type);
    void Consume(TokenType expected_type, String expected_value = "");
    
    // Parsing functions for different constructs
    PslNode* ParseStatement();
    ComponentNode* ParseComponent();
    CircuitNode* ParseCircuit();
    ModuleNode* ParseModule();
    BusNode* ParseBus();
    TestNode* ParseTest();
    ConnectionNode* ParseConnection();
    
    std::map<String, String> ParseParameters();
    String ParseExpression();
    std::vector<PslNode*> ParseBlock();
};

// Compiler to convert PSL AST to ProtoVM C++ components
class PslCompiler {
public:
    String CompileCircuit(CircuitNode* circuit);
    String CompileModule(ModuleNode* module);
    String CompileTest(TestNode* test);
    
    // Helper functions
    String GetCppType(String psl_type);
    std::vector<String> CompileComponent(ComponentNode* comp, String circuit_name);
    std::vector<String> CompileBus(BusNode* bus);
    std::vector<String> CompileConnection(ConnectionNode* conn);
    
private:
    // Component type mapping
    std::map<String, String> component_type_map = {
        {"nand", "ElcNand"},
        {"nor", "ElcNor"},
        {"xor", "ElcXor"},
        {"xnor", "ElcXnor"},
        {"not", "ElcNot"},
        {"vcc", "Pin"},
        {"ground", "Pin"},
        {"d_flip_flop", "FlipFlopD"},
        {"register", "Register4Bit"},
        {"ram", "ICRamRom"},
        {"cpu6502", "IC6502"}
    };
};

#endif