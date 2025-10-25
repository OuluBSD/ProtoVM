#include "PslParser.h"
#include <cctype>
#include <algorithm>

// Implementation of the PSL Tokenizer
PslTokenizer::PslTokenizer(String input) 
    : input(input), pos(0), line(1), column(1) {}

char PslTokenizer::Peek(int offset) const {
    int target_pos = pos + offset;
    if (target_pos >= 0 && target_pos < input.GetLength()) {
        return input[target_pos];
    }
    return '\0';
}

void PslTokenizer::Advance(int count) {
    for (int i = 0; i < count; i++) {
        if (IsAtEnd()) return;
        
        if (Current() == '\n') {
            line++;
            column = 1;
        } else {
            column++;
        }
        pos++;
    }
}

Token PslTokenizer::MakeToken(TokenType type, String value) {
    return Token(type, value, line, column);
}

TokenType PslTokenizer::GetKeywordType(String value) const {
    if (value == "component") return TokenType::KEYWORD_COMPONENT;
    if (value == "circuit") return TokenType::KEYWORD_CIRCUIT;
    if (value == "module") return TokenType::KEYWORD_MODULE;
    if (value == "test") return TokenType::KEYWORD_TEST;
    if (value == "connect") return TokenType::KEYWORD_CONNECT;
    if (value == "bus") return TokenType::KEYWORD_BUS;
    if (value == "stimulus") return TokenType::KEYWORD_STIMULUS;
    if (value == "assert") return TokenType::KEYWORD_ASSERT;
    if (value == "wait") return TokenType::KEYWORD_WAIT;
    return TokenType::IDENTIFIER;
}

Token PslTokenizer::ReadIdentifier() {
    int start_pos = pos;
    
    while (!IsAtEnd() && (std::isalnum(Current()) || Current() == '_')) {
        Advance();
    }
    
    String identifier = input.Mid(start_pos, pos - start_pos);
    TokenType type = GetKeywordType(identifier);
    
    return MakeToken(type, identifier);
}

Token PslTokenizer::ReadNumber() {
    int start_pos = pos;
    
    while (!IsAtEnd() && std::isdigit(Current())) {
        Advance();
    }
    
    String number = input.Mid(start_pos, pos - start_pos);
    return MakeToken(TokenType::NUMBER, number);
}

Token PslTokenizer::ReadString() {
    char quote = Current();
    Advance(); // skip opening quote
    
    int start_pos = pos;
    
    while (!IsAtEnd() && Current() != quote) {
        if (Current() == '\n') {
            // Error: unterminated string
            break;
        }
        Advance();
    }
    
    if (IsAtEnd()) {
        // Error: unterminated string
        return MakeToken(TokenType::UNKNOWN, "");
    }
    
    Advance(); // skip closing quote
    String str_content = input.Mid(start_pos, pos - start_pos - 1);
    return MakeToken(TokenType::STRING, str_content);
}

Token PslTokenizer::NextToken() {
    // Skip whitespace (but not newlines, which are significant)
    while (!IsAtEnd() && (Current() == ' ' || Current() == '\t')) {
        Advance();
    }
    
    if (IsAtEnd()) {
        return MakeToken(TokenType::END_OF_FILE, "");
    }
    
    char c = Current();
    
    switch (c) {
        case '\n':
            Advance();
            return MakeToken(TokenType::UNKNOWN, "\n");
            
        case ':':
            Advance();
            return MakeToken(TokenType::COLON, ":");
            
        case '-':
            if (Peek(1) == '-' && pos + 1 < input.GetLength()) {
                Advance(2);
                return MakeToken(TokenType::DASH_ARROW, "--");
            } else {
                Advance();
                return MakeToken(TokenType::UNKNOWN, "-");
            }
            
        case '=':
            Advance();
            return MakeToken(TokenType::EQUALS, "=");
            
        case ',':
            Advance();
            return MakeToken(TokenType::COMMA, ",");
            
        case '(':
            Advance();
            return MakeToken(TokenType::LPAREN, "(");
            
        case ')':
            Advance();
            return MakeToken(TokenType::RPAREN, ")");
            
        case '"':
        case '\'':
            return ReadString();
            
        default:
            if (std::isalpha(c) || c == '_') {
                return ReadIdentifier();
            } else if (std::isdigit(c)) {
                return ReadNumber();
            } else {
                Advance();
                return MakeToken(TokenType::UNKNOWN, String() << c);
            }
    }
}

std::vector<Token> PslTokenizer::Tokenize() {
    std::vector<Token> tokens;
    
    Token token = NextToken();
    while (token.type != TokenType::END_OF_FILE) {
        tokens.push_back(token);
        if (token.type == TokenType::END_OF_FILE) {
            break;
        }
        token = NextToken();
    }
    
    tokens.push_back(MakeToken(TokenType::END_OF_FILE, ""));
    return tokens;
}

// Implementation of the PSL Parser
void PslParser::SetTokens(const std::vector<Token>& t) {
    tokens = t;
    current_pos = 0;
    if (!tokens.empty()) {
        current_token = tokens[0];
    }
}

void PslParser::Advance() {
    if (current_pos < tokens.size() - 1) {
        current_pos++;
        current_token = tokens[current_pos];
    }
}

bool PslParser::Check(TokenType type) {
    return current_token.type == type;
}

bool PslParser::Match(TokenType type) {
    if (Check(type)) {
        Advance();
        return true;
    }
    return false;
}

Token PslParser::Expect(TokenType type) {
    if (!Check(type)) {
        // For simplicity, we'll just return a dummy token
        // In a real implementation, this would throw an exception
        return Token(TokenType::UNKNOWN, "ERROR", -1, -1);
    }
    Token result = current_token;
    Advance();
    return result;
}

std::vector<PslNode*> PslParser::Parse() {
    std::vector<PslNode*> nodes;
    
    while (current_token.type != TokenType::END_OF_FILE) {
        PslNode* node = ParseStatement();
        if (node) {
            nodes.push_back(node);
        }
    }
    
    return nodes;
}

PslNode* PslParser::ParseStatement() {
    if (Check(TokenType::KEYWORD_COMPONENT)) {
        return ParseComponent();
    } else if (Check(TokenType::KEYWORD_CIRCUIT)) {
        return ParseCircuit();
    } else if (Check(TokenType::KEYWORD_MODULE)) {
        return ParseModule();
    } else if (Check(TokenType::KEYWORD_TEST)) {
        return ParseTest();
    } else if (Check(TokenType::KEYWORD_BUS)) {
        return ParseBus();
    } else if (Check(TokenType::KEYWORD_CONNECT)) {
        return ParseConnection();
    }
    
    // Skip unknown tokens for now
    Advance();
    return nullptr;
}

ComponentNode* PslParser::ParseComponent() {
    Expect(TokenType::KEYWORD_COMPONENT);
    
    String name = current_token.value;
    Expect(TokenType::IDENTIFIER);
    
    Expect(TokenType::COLON);
    
    String type_name = current_token.value;
    Expect(TokenType::IDENTIFIER);
    
    std::map<String, String> params;
    if (Check(TokenType::LPAREN)) {
        params = ParseParameters();
    }
    
    return new ComponentNode(name, type_name, params);
}

std::map<String, String> PslParser::ParseParameters() {
    std::map<String, String> params;
    
    Expect(TokenType::LPAREN);
    
    if (!Check(TokenType::RPAREN)) {
        while (true) {
            String param_name = current_token.value;
            Expect(TokenType::IDENTIFIER);
            
            Expect(TokenType::EQUALS);
            
            String param_value = ParseExpression();
            params[param_name] = param_value;
            
            if (Check(TokenType::RPAREN)) {
                break;
            }
            Expect(TokenType::COMMA);
        }
    }
    
    Expect(TokenType::RPAREN);
    return params;
}

String PslParser::ParseExpression() {
    if (current_token.type == TokenType::NUMBER || 
        current_token.type == TokenType::STRING || 
        current_token.type == TokenType::IDENTIFIER) {
        String value = current_token.value;
        Advance();
        return value;
    }
    
    // Return empty string for unrecognized expressions
    return "";
}

CircuitNode* PslParser::ParseCircuit() {
    Expect(TokenType::KEYWORD_CIRCUIT);
    
    String name = current_token.value;
    Expect(TokenType::IDENTIFIER);
    
    Expect(TokenType::COLON);
    
    // For now, parse a simple block of statements
    // In a full implementation, we'd handle indented blocks properly
    CircuitNode* circuit = new CircuitNode(name);
    
    // For simplicity in this implementation, just add a few sample nodes
    // The actual block parsing would be more complex to handle indents
    while (current_token.type != TokenType::END_OF_FILE && 
           current_token.type != TokenType::KEYWORD_CIRCUIT) {  // Simplified end condition
        PslNode* stmt = ParseStatement();
        if (stmt) {
            circuit->children.push_back(stmt);
        } else {
            // Skip the token if we couldn't parse it
            Advance();
        }
        
        // Break after a few iterations to avoid infinite loops in this simplified version
        // In a real implementation, there would be proper block structure detection
        break;
    }
    
    return circuit;
}

ModuleNode* PslParser::ParseModule() {
    Expect(TokenType::KEYWORD_MODULE);
    
    String name = current_token.value;
    Expect(TokenType::IDENTIFIER);
    
    Expect(TokenType::COLON);
    
    ModuleNode* module = new ModuleNode(name);
    
    // Similar simplification as circuit parsing
    while (current_token.type != TokenType::END_OF_FILE && 
           current_token.type != TokenType::KEYWORD_MODULE) {  // Simplified end condition
        PslNode* stmt = ParseStatement();
        if (stmt) {
            module->children.push_back(stmt);
        } else {
            // Skip the token if we couldn't parse it
            Advance();
        }
        
        // Break after a few iterations to avoid infinite loops
        break;
    }
    
    return module;
}

BusNode* PslParser::ParseBus() {
    Expect(TokenType::KEYWORD_BUS);
    
    String name = current_token.value;
    Expect(TokenType::IDENTIFIER);
    
    Expect(TokenType::COLON);
    Expect(TokenType::IDENTIFIER); // Expecting "width" keyword
    Expect(TokenType::EQUALS);
    
    String width_str = current_token.value;
    Expect(TokenType::NUMBER);
    
    int width = StrInt(width_str);
    return new BusNode(name, width);
}

TestNode* PslParser::ParseTest() {
    Expect(TokenType::KEYWORD_TEST);
    
    String name = current_token.value;
    Expect(TokenType::IDENTIFIER);
    
    Expect(TokenType::COLON);
    
    TestNode* test = new TestNode(name, "unknown_circuit");
    
    // For now, return the basic test node
    // The full implementation would parse stimulus and expected sections
    return test;
}

ConnectionNode* PslParser::ParseConnection() {
    Expect(TokenType::KEYWORD_CONNECT);
    
    // Handle both single connection and block forms
    if (Check(TokenType::COLON)) {
        // This is a block of connections - advanced parsing would handle this
        return nullptr; // Return null for now as block connections are complex
    }
    
    String source = current_token.value;
    Expect(TokenType::IDENTIFIER);
    
    Expect(TokenType::DASH_ARROW);
    
    String destination = current_token.value;
    Expect(TokenType::IDENTIFIER);
    
    return new ConnectionNode(source, destination);
}

// Implementation of the PSL Compiler
String PslCompiler::GetCppType(String psl_type) {
    auto it = component_type_map.find(psl_type);
    if (it != component_type_map.end()) {
        return it->second;
    }
    return "ElcBase"; // Default fallback
}

std::vector<String> PslCompiler::CompileComponent(ComponentNode* comp, String circuit_name) {
    std::vector<String> cpp_lines;
    
    String cpp_type = GetCppType(comp->type_name);
    
    // Handle special cases like vcc and ground
    if (comp->type_name == "vcc") {
        cpp_lines.push_back("    Pin& " + comp->name + " = b.Add<Pin>(\"" + comp->name + "\");  // VCC");
        cpp_lines.push_back("    " + comp->name + ".SetReference(1);");
    } else if (comp->type_name == "ground") {
        cpp_lines.push_back("    Pin& " + comp->name + " = b.Add<Pin>(\"" + comp->name + "\");  // GND");
        cpp_lines.push_back("    " + comp->name + ".SetReference(0);");
    } else if (comp->type_name == "register" && comp->parameters.count("size") && 
               comp->parameters["size"] == "8") {
        // Handle 8-bit register specially if needed
        cpp_lines.push_back("    Register4Bit& " + comp->name + "_low = b.Add<Register4Bit>(\"" + comp->name + "_low\");");
        cpp_lines.push_back("    Register4Bit& " + comp->name + "_high = b.Add<Register4Bit>(\"" + comp->name + "_high\");");
    } else {
        // Default component creation
        String paramString = "";
        for (const auto& param : comp->parameters) {
            paramString += param.first + "=" + param.second + ", ";
        }
        if (!paramString.IsEmpty()) {
            paramString.Remove(paramString.GetLength()-2); // Remove trailing ", "
        }
        cpp_lines.push_back("    " + cpp_type + "& " + comp->name + " = b.Add<" + cpp_type + ">(\"" + comp->name + "\");  // params: " + paramString);
    }
    
    return cpp_lines;
}

std::vector<String> PslCompiler::CompileBus(BusNode* bus) {
    std::vector<String> cpp_lines;
    cpp_lines.push_back("    // Bus " + bus->name + " with width " + IntStr(bus->width));
    // For now, just add a comment
    return cpp_lines;
}

std::vector<String> PslCompiler::CompileConnection(ConnectionNode* conn) {
    std::vector<String> cpp_lines;
    cpp_lines.push_back("    " + conn->source + " >> " + conn->destination + ";");
    return cpp_lines;
}

String PslCompiler::CompileCircuit(CircuitNode* circuit) {
    std::vector<String> cpp_code;
    
    // Include headers
    cpp_code.push_back("#include \"ProtoVM.h\"");
    cpp_code.push_back("");
    
    // Function declaration
    String func_name = "Setup" + circuit->name;
    // Manually remove underscores by building a new string
    String new_func_name;
    for(int i = 0; i < func_name.GetLength(); i++) {
        if(func_name[i] != '_') {
            new_func_name.Cat(func_name[i]);
        }
    }
    func_name = new_func_name; // Remove underscores from function name
    cpp_code.push_back("void " + func_name + "(Machine& mach) {");
    cpp_code.push_back("    Pcb& b = mach.AddPcb();");
    cpp_code.push_back("");
    
    // Add components, buses, and connections
    for (PslNode* node : circuit->children) {
        if (ComponentNode* comp_node = dynamic_cast<ComponentNode*>(node)) {
            std::vector<String> lines = CompileComponent(comp_node, circuit->name);
            for (const String& line : lines) {
                cpp_code.push_back(line);
            }
        } else if (BusNode* bus_node = dynamic_cast<BusNode*>(node)) {
            std::vector<String> lines = CompileBus(bus_node);
            for (const String& line : lines) {
                cpp_code.push_back(line);
            }
        } else if (ConnectionNode* conn_node = dynamic_cast<ConnectionNode*>(node)) {
            std::vector<String> lines = CompileConnection(conn_node);
            for (const String& line : lines) {
                cpp_code.push_back(line);
            }
        }
    }
    
    cpp_code.push_back("}");
    cpp_code.push_back("");
    
    String result;
    for (const String& line : cpp_code) {
        result += line + "\n";
    }
    
    return result;
}

String PslCompiler::CompileModule(ModuleNode* module) {
    std::vector<String> cpp_code;
    cpp_code.push_back("// Module " + module->name + " definition");
    cpp_code.push_back("// This would typically generate a reusable component or function");
    
    String func_name = "Create" + module->name;
    // Manually remove underscores by building a new string
    String new_func_name;
    for(int i = 0; i < func_name.GetLength(); i++) {
        if(func_name[i] != '_') {
            new_func_name.Cat(func_name[i]);
        }
    }
    func_name = new_func_name; // Remove underscores
    cpp_code.push_back("void " + func_name + "(Pcb& b) {  // Module as a function");
    
    for (PslNode* node : module->children) {
        if (ComponentNode* comp_node = dynamic_cast<ComponentNode*>(node)) {
            String cpp_type = GetCppType(comp_node->type_name);
            if (comp_node->type_name == "vcc" || comp_node->type_name == "ground") {
                cpp_code.push_back("    Pin& " + comp_node->name + " = b.Add<Pin>(\"" + comp_node->name + "\");");
                if (comp_node->type_name == "vcc") {
                    cpp_code.push_back("    " + comp_node->name + ".SetReference(1);  // VCC");
                } else {
                    cpp_code.push_back("    " + comp_node->name + ".SetReference(0);  // Ground");
                }
            } else {
                cpp_code.push_back("    " + cpp_type + "& " + comp_node->name + " = b.Add<" + cpp_type + ">(\"" + comp_node->name + "\");");
            }
        } else if (ConnectionNode* conn_node = dynamic_cast<ConnectionNode*>(node)) {
            cpp_code.push_back("    " + conn_node->source + " >> " + conn_node->destination + ";");
        }
    }
    
    cpp_code.push_back("}");
    cpp_code.push_back("");
    
    String result;
    for (const String& line : cpp_code) {
        result += line + "\n";
    }
    
    return result;
}

String PslCompiler::CompileTest(TestNode* test) {
    std::vector<String> test_code;
    test_code.push_back("// Test " + test->name + " for circuit " + test->circuit_under_test);
    test_code.push_back("// This would contain test validation code");
    
    String result;
    for (const String& line : test_code) {
        result += line + "\n";
    }
    
    return result;
}