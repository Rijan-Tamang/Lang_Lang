#pragma once
#include "parser.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>

// Translates an AST that has already passed SemanticAnalyzer::analyze()
// (with zero errors) into standalone C source code.
//
// Type model: the toy language has no explicit types, so C needs one.
// This generator unifies every number (INT_NO/FLOAT_NO) to C `double`,
// and every string to a fixed `static char[STR_BUF_SIZE]` buffer. See
// the comments emitted at the top of the generated file for the exact
// runtime tradeoffs (string buffers are static -> not safe for
// recursive functions; concatenation uses a small round-robin pool).
class CodeGenerator {
public:
    // Throws std::runtime_error if it hits a construct it cannot translate.
    std::string generate(Node* program);

private:
    struct FuncSig {
        std::vector<std::string> paramTypes; // each "double" or "string"
        std::string returnType;              // "double" | "string" | "void"
        bool hasBody = false;
    };

    std::vector<std::unordered_map<std::string, std::string>> scopes; // name -> "double"/"string"
    std::unordered_map<std::string, FuncSig> functions;

    // ---- scope helpers (mirror the shape used by SemanticAnalyzer) ----
    void pushScope();
    void popScope();
    void declare(const std::string& name, const std::string& type);
    std::string lookupType(const std::string& name) const; // defaults to "double" if unseen

    // ---- signature discovery pre-pass ----
    void collectFunctionSignatures(Node* program);
    void scanCallsForStringParams(Node* node);
    bool literalLooksString(Node* expr, const FuncSig& sig) const;

    // ---- type + expression emission ----
    std::string typeOfExpr(Node* expr) const;
    std::string emitExpr(Node* expr);
    static std::string escapeCString(const std::string& s);
    static std::string escapePercent(const std::string& s);

    // ---- statement emission ----
    void emitStatement(Node* stmt, std::ostringstream& out, int indent);
    void emitBlockBody(Node* block, std::ostringstream& out, int indent); // no extra scope push
    std::string emitForInit(Node* init, std::ostringstream& preStatements, int indent);
    std::string indentStr(int indent) const;

    // ---- top level ----
    void emitFunction(Node* fn, std::ostringstream& out);

    std::ostringstream protos;
    std::ostringstream funcDefs;
    std::ostringstream mainBody;
};