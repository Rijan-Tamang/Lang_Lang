#pragma once
#include "parser.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>

// Translates an AST that has already passed SemanticAnalyzer::analyze()
// (with zero errors) into standalone C source code.
//
// Type model: the toy language is dynamically typed - a `yoho` variable can
// hold a number at one point and a string at another (see the runtime
// preamble emitted at the top of every generated file). Every value is
// represented in the generated C as a tagged union struct `Value { type,
// num, str[256] }`, passed and returned *by value*. Every operator (+, -,
// ==, etc.), print, and input statement is lowered to a call into a small
// runtime helper (val_add, val_sub, print_value, read_value, ...) that
// switches on the tag at runtime. Because Value is a plain struct returned
// by value (not a pointer into shared static storage), this also removes
// the earlier static-string-buffer/recursion-safety limitation entirely.
class CodeGenerator {
public:
    // Throws std::runtime_error if it hits a construct it cannot translate.
    std::string generate(Node* program);

private:
    struct FuncSig {
        int paramCount = 0;
        bool returnsValue = false; // false only if it has no `firta expr;` anywhere -> C void
        bool hasBody = false;
    };

    std::unordered_map<std::string, FuncSig> functions;

    // ---- signature discovery pre-pass ----
    void collectFunctionSignatures(Node* program);

    // ---- expression emission ----
    std::string emitExpr(Node* expr);
    static std::string escapeCString(const std::string& s);
    static std::string escapePercent(const std::string& s);

    // ---- statement emission ----
    void emitStatement(Node* stmt, std::ostringstream& out, int indent);
    void emitBlockBody(Node* block, std::ostringstream& out, int indent);
    std::string emitForInit(Node* init, int indent);
    std::string indentStr(int indent) const;

    // ---- top level ----
    void emitFunction(Node* fn, std::ostringstream& out);
    // Top-level `yoho` declarations become real C global `Value` variables
    // (declared at file scope, before any function) so every function can
    // see them, matching the semantic analyzer's global-visibility model.
    void emitGlobalVarDecl(Node* decl, std::ostringstream& out);
    void emitTopLevelVarDeclInit(Node* decl, std::ostringstream& out, int indent);

    std::ostringstream globals;
    std::ostringstream protos;
    std::ostringstream funcDefs;
    std::ostringstream mainBody;
};