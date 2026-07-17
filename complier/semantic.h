#pragma once
#include "parser.h"
#include <string>
#include <vector>
#include <unordered_map>

enum class DiagSeverity { Error, Warning };

struct Diagnostic {
    DiagSeverity severity;
    std::string message;
    int line;
    int column;
};

struct VarSymbol {
    std::string type;   // "int" | "float" | "string" | "unknown"
    int line, column;
};

struct FuncSymbol {
    int paramCount;
    bool defined;        // true once a FunctionDef (with body) has been seen
    int line, column;
};

class SemanticAnalyzer {
public:
    // Walks the AST produced by Parser::parseProgram() and collects diagnostics.
    std::vector<Diagnostic> analyze(Node* program);

private:
    std::vector<std::unordered_map<std::string, VarSymbol>> scopes;
    std::unordered_map<std::string, FuncSymbol> functions;
    std::vector<Diagnostic> diags;

    int loopDepth = 0;
    int functionDepth = 0;

    void pushScope();
    void popScope();
    bool declareVar(const std::string& name, const std::string& type, int line, int col);
    VarSymbol* lookupVar(const std::string& name);

    void error(int line, int col, const std::string& msg);
    void warn(int line, int col, const std::string& msg);

    void collectFunctionSignatures(Node* program);
    void visitStatement(Node* node);
    void visitBlockScoped(Node* block); // pushes its own scope
    std::string visitExpression(Node* node);

    bool isNumeric(const std::string& type) const;
};