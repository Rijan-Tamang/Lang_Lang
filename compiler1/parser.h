#pragma once
#include "token1.h"
#include "lexer.h"
#include <vector>
#include <string>
#include <memory>
#include <stdexcept>

enum class NodeKind {
    Program,

    // statements
    VarDecl,           // "yoho" a = expr, b = expr, ...   (children = each initializer's expr, or none)
    Block,              // children = statements
    Return,             // child[0] = expr (optional)
    ExprStatement,       // child[0] = expr
    Empty,
    Print,              // text = format string, children = args
    Input,              // text = prompt string, children = identifier nodes
    FunctionDecl,       // text = name, children = params (Identifier nodes)
    FunctionDef,        // text = name, children = params..., last child = Block
    Param,              // text = name
    If,                 // see elifConds/elifBodies/elseBody fields on Node
    Switch,             // child[0] = expr, rest = Case/Default nodes
    Case,               // child[0] = label literal, rest = statements
    Default,            // children = statements
    While,              // child[0] = cond, child[1] = body
    DoWhile,            // child[0] = body(block), child[1] = cond
    For,                // child[0]=init(or Empty), child[1]=cond(or Empty), child[2]=post(or Empty), child[3]=body
    Break,
    Continue,

    // expressions
    Assignment,         // text = op ("=","+=","-=","*=","/=","%="), child[0]=target(Identifier), child[1]=value
    LogicalOp,          // text = "||" | "&&"
    BinaryOp,           // text = op
    UnaryOp,            // text = op, child[0] = operand
    Call,               // child[0] = callee, rest = args
    IntLiteral,
    FloatLiteral,
    StringLiteral,
    Identifier
};

struct Node {
    NodeKind kind;
    std::string text;
    int line = 0;
    int column = 0;
    std::vector<std::unique_ptr<Node>> children;

    // extra fields used by If, to keep an elif chain readable without
    // overloading the generic children vector's ordering.
    std::vector<std::unique_ptr<Node>> elifConds;
    std::vector<std::unique_ptr<Node>> elifBodies;
    std::unique_ptr<Node> elseBody; // nullable

    Node(NodeKind k, std::string t = "", int l = 0, int c = 0)
        : kind(k), text(std::move(t)), line(l), column(c) {}
};

using NodePtr = std::unique_ptr<Node>;

std::string nodeKindToString(NodeKind k);

struct ParseError : public std::runtime_error {
    int line, column;
    ParseError(const std::string& msg, int line, int column)
        : std::runtime_error(msg), line(line), column(column) {}
};

// A recorded syntax error. Unlike ParseError (which is only used internally
// to unwind the recursive descent back to a resynchronization point),
// SyntaxError is the durable record the caller reads after parsing finishes.
struct SyntaxError {
    std::string message;
    int line;
    int column;
};

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);

    // Parses the whole token stream. Uses panic-mode error recovery: on a
    // syntax error, the error is recorded (see getErrors()) and the parser
    // skips ahead to the next likely statement boundary instead of stopping,
    // so a single call can report every syntax error in the file. The
    // returned AST may be partial/incomplete if any errors were recorded -
    // always check hasErrors() before using it for semantic analysis.
    NodePtr parseProgram();

    bool hasErrors() const { return !errors.empty(); }
    const std::vector<SyntaxError>& getErrors() const { return errors; }

private:
    std::vector<Token> tokens;
    size_t pos = 0;
    std::vector<SyntaxError> errors;

    // --- token stream helpers ---
    const Token& peek(int offset = 0) const;
    const Token& previous() const;
    bool isAtEnd() const;
    const Token& advance();
    bool check(TokenType type) const;
    bool checkKeyword(const std::string& kw) const;
    bool match(TokenType type);
    bool matchKeyword(const std::string& kw);
    const Token& expect(TokenType type, const std::string& message);
    const Token& expectKeyword(const std::string& kw, const std::string& message);
    [[noreturn]] void error(const std::string& message);
    [[noreturn]] void error(const Token& at, const std::string& message);

    // Skips tokens until a plausible statement boundary is reached (just
    // after a ';', or right before a token that starts a new statement, or
    // right before a closing '}' so the enclosing block/program loop can
    // consume it itself). Called after catching a ParseError.
    void synchronize();

    // --- statements ---
    NodePtr statement();
    NodePtr declaration();
    NodePtr block();
    NodePtr returnStatement();
    NodePtr exprStatement();
    NodePtr breakStatement();
    NodePtr continueStatement();
    NodePtr emptyStatement();
    NodePtr functionDeclOrDef();      // handles both functiondec and functiondef (shared prefix)
    NodePtr printStatement();
    NodePtr inputStatement();
    NodePtr ifStatement();
    NodePtr switchStatement();
    NodePtr whileStatement();
    NodePtr doWhileStatement();
    NodePtr forStatement();

    // --- expressions (precedence climbing) ---
    NodePtr expression();
    NodePtr assignment();
    NodePtr logicalOr();
    NodePtr logicalAnd();
    NodePtr equality();
    NodePtr relational();
    NodePtr additive();
    NodePtr multiplicative();
    NodePtr unary();
    NodePtr call();
    NodePtr primary();

    bool isAssignOp(TokenType t) const;
};