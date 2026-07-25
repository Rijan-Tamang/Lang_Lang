#include "parser.h"
#include <unordered_set>

using namespace std;

string nodeKindToString(NodeKind k) {
    switch (k) {
        case NodeKind::Program: return "Program";
        case NodeKind::VarDecl: return "VarDecl";
        case NodeKind::Block: return "Block";
        case NodeKind::Return: return "Return";
        case NodeKind::ExprStatement: return "ExprStatement";
        case NodeKind::Empty: return "Empty";
        case NodeKind::Print: return "Print";
        case NodeKind::Input: return "Input";
        case NodeKind::FunctionDecl: return "FunctionDecl";
        case NodeKind::FunctionDef: return "FunctionDef";
        case NodeKind::Param: return "Param";
        case NodeKind::If: return "If";
        case NodeKind::Switch: return "Switch";
        case NodeKind::Case: return "Case";
        case NodeKind::Default: return "Default";
        case NodeKind::While: return "While";
        case NodeKind::DoWhile: return "DoWhile";
        case NodeKind::For: return "For";
        case NodeKind::Break: return "Break";
        case NodeKind::Continue: return "Continue";
        case NodeKind::Assignment: return "Assignment";
        case NodeKind::LogicalOp: return "LogicalOp";
        case NodeKind::BinaryOp: return "BinaryOp";
        case NodeKind::UnaryOp: return "UnaryOp";
        case NodeKind::Call: return "Call";
        case NodeKind::IntLiteral: return "IntLiteral";
        case NodeKind::FloatLiteral: return "FloatLiteral";
        case NodeKind::StringLiteral: return "StringLiteral";
        case NodeKind::Identifier: return "Identifier";
    }
    return "Unknown";
}

Parser::Parser(vector<Token> toks) : tokens(move(toks)) {}

// ---------------- token stream helpers ----------------

const Token& Parser::peek(int offset) const {
    size_t idx = pos + offset;
    if (idx >= tokens.size()) return tokens.back(); // END
    return tokens[idx];
}

const Token& Parser::previous() const {
    return tokens[pos - 1];
}

bool Parser::isAtEnd() const {
    return peek().type == END;
}

const Token& Parser::advance() {
    if (!isAtEnd()) pos++;
    return previous();
}

bool Parser::check(TokenType type) const {
    return peek().type == type;
}

bool Parser::checkKeyword(const string& kw) const {
    return peek().type == KEYWORD && peek().value == kw;
}

bool Parser::match(TokenType type) {
    if (check(type)) { advance(); return true; }
    return false;
}

bool Parser::matchKeyword(const string& kw) {
    if (checkKeyword(kw)) { advance(); return true; }
    return false;
}

const Token& Parser::expect(TokenType type, const string& message) {
    if (check(type)) return advance();
    error(peek(), message);
}

const Token& Parser::expectKeyword(const string& kw, const string& message) {
    if (checkKeyword(kw)) return advance();
    error(peek(), message);
}

void Parser::error(const string& message) {
    error(peek(), message);
}

void Parser::error(const Token& at, const string& message) {
    string full = message + " (got '" + at.value + "')";
    errors.push_back({full, at.line, at.column});
    throw ParseError(full, at.line, at.column);
}

void Parser::synchronize() {
    static const unordered_set<string> statementStarters = {
        "yoho", "firta", "vayo", "aghibadh", "bhan", "sun",
        "yedi", "yochai", "jabasamma", "gar", "ferini",
        "yo", "abayeiho" // switch-body resync points
    };

    // Always consume at least one token before looking for a safe resume
    // point. Without this, an error thrown before any token was consumed
    // during this parse attempt (e.g. an unexpected '}' at the very start
    // of an expression) can leave previous() pointing at a stale token from
    // an earlier, unrelated statement - satisfying the "already safe" check
    // below with zero progress, which would make the caller retry from the
    // exact same position forever.
    if (!isAtEnd()) advance();

    while (!isAtEnd()) {
        // Just consumed a ';' -> the bad statement is behind us, safe to resume.
        if (previous().type == SEMICOLAN) return;

        // Don't consume a closing brace - let the enclosing block/switch
        // loop see it and terminate normally instead of over-skipping.
        if (check(RIGHT_BRECE)) return;

        // A keyword that starts a new statement is also a safe resume point.
        if (check(KEYWORD) && statementStarters.count(peek().value)) return;

        advance();
    }
}

bool Parser::isAssignOp(TokenType t) const {
    switch (t) {
        case ASSIGNMENT_OP:
        case ADD_ASSIGNMENT_OP:
        case SUB_ASSIGNMENT_OP:
        case MUL_ASSIGNMENT_OP:
        case DIV_ASSIGNMENT_OP:
        case MOD_ASSIGNMENT_OP:
            return true;
        default:
            return false;
    }
}

// ---------------- entry point ----------------

NodePtr Parser::parseProgram() {
    auto prog = make_unique<Node>(NodeKind::Program, "program", 0, 0);
    while (!isAtEnd()) {
        try {
            prog->children.push_back(statement());
        } catch (const ParseError&) {
            synchronize();
        }
    }
    return prog;
}

// ---------------- statements ----------------

NodePtr Parser::statement() {
    if (checkKeyword("yoho"))    return declaration();
    if (check(LEFT_BRECE))       return block();
    if (checkKeyword("firta"))   return returnStatement();
    if (checkKeyword("vayo"))    return breakStatement();
    if (checkKeyword("aghibadh")) return continueStatement();
    if (check(SEMICOLAN))        return emptyStatement();
    if (checkKeyword("bhan"))    return printStatement();
    if (checkKeyword("sun"))     return inputStatement();
    if (checkKeyword("yedi"))    return ifStatement();
    if (checkKeyword("yochai"))  return switchStatement();
    if (checkKeyword("jabasamma"))return whileStatement();
    if (checkKeyword("gar"))     return doWhileStatement();
    if (checkKeyword("ferini"))  return forStatement();

    // functiondec / functiondef share a prefix with a plain call expression
    // statement ("foo(x);"). Disambiguate by lookahead: a parameter list
    // beginning with "yoho" is unambiguous (a call's arguments never start
    // with that keyword). Empty parens "()" are genuinely ambiguous between
    // a zero-param function decl/def and a zero-argument call statement
    // ("greet();") - resolved by requiring a '{' body to treat it as a
    // definition; a bare "name();" with no body is a call.
    if (check(IDENTIFIER) && peek(1).type == LEFT_PAREN) {
        bool paramStart = peek(2).type == KEYWORD && peek(2).value == "yoho";
        if (paramStart) return functionDeclOrDef();

        bool emptyParams = peek(2).type == RIGHT_PAREN;
        if (emptyParams && peek(3).type == LEFT_BRECE) return functionDeclOrDef();
    }

    return exprStatement();
}

NodePtr Parser::declaration() {
    const Token& kw = expectKeyword("yoho", "expected 'yoho'");
    auto decl = make_unique<Node>(NodeKind::VarDecl, "yoho", kw.line, kw.column);

    do {
        const Token& name = expect(IDENTIFIER, "expected identifier after 'yoho'");
        auto idNode = make_unique<Node>(NodeKind::Identifier, name.value, name.line, name.column);
        if (match(ASSIGNMENT_OP)) {
            idNode->children.push_back(expression());
        }
        decl->children.push_back(move(idNode));
    } while (match(COMMA));

    expect(SEMICOLAN, "expected ';' after declaration");
    return decl;
}

NodePtr Parser::block() {
    const Token& open = expect(LEFT_BRECE, "expected '{'");
    auto blk = make_unique<Node>(NodeKind::Block, "{}", open.line, open.column);
    while (!check(RIGHT_BRECE) && !isAtEnd()) {
        try {
            blk->children.push_back(statement());
        } catch (const ParseError&) {
            synchronize();
        }
    }
    expect(RIGHT_BRECE, "expected '}' to close block");
    return blk;
}

NodePtr Parser::returnStatement() {
    const Token& kw = expectKeyword("firta", "expected 'firta'");
    auto node = make_unique<Node>(NodeKind::Return, "firta", kw.line, kw.column);
    if (!check(SEMICOLAN)) {
        node->children.push_back(expression());
    }
    expect(SEMICOLAN, "expected ';' after return statement");
    return node;
}

NodePtr Parser::exprStatement() {
    const Token& start = peek();
    auto expr = expression();

    // A bare expression statement only makes sense if it does something -
    // an assignment or a function call. Anything else (a stray literal, a
    // parenthesized value, a plain identifier, arithmetic with no
    // assignment, etc.) is computed and immediately discarded, which is
    // almost always a mistake (e.g. writing ("msg"); instead of bhan("msg");)
    // rather than intentional, so it's rejected here instead of silently
    // accepted.
    if (expr->kind != NodeKind::Assignment && expr->kind != NodeKind::Call) {
        error(start, "expression statement has no effect - only assignments and function calls are allowed as statements");
    }

    expect(SEMICOLAN, "expected ';' after expression");
    auto node = make_unique<Node>(NodeKind::ExprStatement, "", start.line, start.column);
    node->children.push_back(move(expr));
    return node;
}

NodePtr Parser::breakStatement() {
    const Token& kw = expectKeyword("vayo", "expected 'vayo'");
    expect(SEMICOLAN, "expected ';' after 'vayo'");
    return make_unique<Node>(NodeKind::Break, "vayo", kw.line, kw.column);
}

NodePtr Parser::continueStatement() {
    const Token& kw = expectKeyword("aghibadh", "expected 'aghibadh'");
    expect(SEMICOLAN, "expected ';' after 'aghibadh'");
    return make_unique<Node>(NodeKind::Continue, "aghibadh", kw.line, kw.column);
}

NodePtr Parser::emptyStatement() {
    const Token& t = expect(SEMICOLAN, "expected ';'");
    return make_unique<Node>(NodeKind::Empty, ";", t.line, t.column);
}

NodePtr Parser::functionDeclOrDef() {
    const Token& name = expect(IDENTIFIER, "expected function name");
    expect(LEFT_PAREN, "expected '(' after function name");

    vector<NodePtr> params;
    if (!check(RIGHT_PAREN)) {
        do {
            const Token& pkw = expectKeyword("yoho", "expected 'yoho' before parameter name");
            const Token& pname = expect(IDENTIFIER, "expected parameter name");
            params.push_back(make_unique<Node>(NodeKind::Param, pname.value, pkw.line, pkw.column));
        } while (match(COMMA));
    }
    expect(RIGHT_PAREN, "expected ')' after parameter list");

    if (check(LEFT_BRECE)) {
        auto def = make_unique<Node>(NodeKind::FunctionDef, name.value, name.line, name.column);
        for (auto& p : params) def->children.push_back(move(p));
        def->children.push_back(block());
        return def;
    }

    expect(SEMICOLAN, "expected ';' or '{' after function parameter list");
    auto decl = make_unique<Node>(NodeKind::FunctionDecl, name.value, name.line, name.column);
    for (auto& p : params) decl->children.push_back(move(p));
    return decl;
}

NodePtr Parser::printStatement() {
    const Token& kw = expectKeyword("bhan", "expected 'bhan'");
    expect(LEFT_PAREN, "expected '(' after 'bhan'");
    auto node = make_unique<Node>(NodeKind::Print, "", kw.line, kw.column);
    if (!check(RIGHT_PAREN)) {
        do {
            node->children.push_back(expression());
        } while (match(COMMA));
    }
    expect(RIGHT_PAREN, "expected ')' to close 'bhan'");
    expect(SEMICOLAN, "expected ';' after 'bhan(...)'");
    return node;
}

NodePtr Parser::inputStatement() {
    const Token& kw = expectKeyword("sun", "expected 'sun'");
    expect(LEFT_PAREN, "expected '(' after 'sun'");
    const Token& prompt = expect(STRING, "expected prompt string in 'sun'");
    auto node = make_unique<Node>(NodeKind::Input, prompt.value, kw.line, kw.column);
    while (match(COMMA)) {
        const Token& id = expect(IDENTIFIER, "expected identifier in 'sun'");
        node->children.push_back(make_unique<Node>(NodeKind::Identifier, id.value, id.line, id.column));
    }
    expect(RIGHT_PAREN, "expected ')' to close 'sun'");
    expect(SEMICOLAN, "expected ';' after 'sun(...)'");
    return node;
}

NodePtr Parser::ifStatement() {
    const Token& kw = expectKeyword("yedi", "expected 'yedi'");
    expect(LEFT_PAREN, "expected '(' after 'yedi'");
    auto cond = expression();
    expect(RIGHT_PAREN, "expected ')' after condition");
    auto thenStmt = statement();

    auto node = make_unique<Node>(NodeKind::If, "yedi", kw.line, kw.column);
    node->children.push_back(move(cond));
    node->children.push_back(move(thenStmt));

    while (checkKeyword("tesovaye")) {
        advance();
        expect(LEFT_PAREN, "expected '(' after 'tesovaye'");
        auto elifCond = expression();
        expect(RIGHT_PAREN, "expected ')' after 'tesovaye' condition");
        auto elifBody = statement();
        node->elifConds.push_back(move(elifCond));
        node->elifBodies.push_back(move(elifBody));
    }

    if (matchKeyword("natra")) {
        node->elseBody = statement();
    }

    return node;
}

NodePtr Parser::switchStatement() {
    const Token& kw = expectKeyword("yochai", "expected 'yochai'");
    expect(LEFT_PAREN, "expected '(' after 'yochai'");
    auto subject = expression();
    expect(RIGHT_PAREN, "expected ')' after switch expression");
    expect(LEFT_BRECE, "expected '{' to start switch body");

    auto node = make_unique<Node>(NodeKind::Switch, "yochai", kw.line, kw.column);
    node->children.push_back(move(subject));

    while (checkKeyword("yo")) {
        const Token& ckw = advance();
        auto caseNode = make_unique<Node>(NodeKind::Case, "yo", ckw.line, ckw.column);
        const Token& label = peek();
        if (check(INT_NO)) {
            caseNode->children.push_back(make_unique<Node>(NodeKind::IntLiteral, advance().value, label.line, label.column));
        } else if (check(FLOAT_NO)) {
            caseNode->children.push_back(make_unique<Node>(NodeKind::FloatLiteral, advance().value, label.line, label.column));
        } else if (check(IDENTIFIER)) {
            caseNode->children.push_back(make_unique<Node>(NodeKind::Identifier, advance().value, label.line, label.column));
        } else {
            error("expected INT_NO, FLOAT_NO, or IDENTIFIER as case label");
        }
        expect(COLAN, "expected ':' after case label");
        while (!checkKeyword("yo") && !checkKeyword("abayeiho") && !check(RIGHT_BRECE) && !isAtEnd()) {
            try {
                caseNode->children.push_back(statement());
            } catch (const ParseError&) {
                synchronize();
            }
        }
        node->children.push_back(move(caseNode));
    }

    if (matchKeyword("abayeiho")) {
        const Token& dkw = previous();
        auto defNode = make_unique<Node>(NodeKind::Default, "abayeiho", dkw.line, dkw.column);
        expect(COLAN, "expected ':' after 'abayeiho'");
        while (!check(RIGHT_BRECE) && !isAtEnd()) {
            try {
                defNode->children.push_back(statement());
            } catch (const ParseError&) {
                synchronize();
            }
        }
        node->children.push_back(move(defNode));
    }

    expect(RIGHT_BRECE, "expected '}' to close switch body");
    return node;
}

NodePtr Parser::whileStatement() {
    const Token& kw = expectKeyword("jabasamma", "expected 'jabasamma'");
    expect(LEFT_PAREN, "expected '(' after 'jabasamma'");
    auto cond = expression();
    expect(RIGHT_PAREN, "expected ')' after while condition");

    if (!check(LEFT_BRECE)) {
        error(peek(), "expected '{' to start 'jabasamma' body");
    }
    auto body = block();

    auto node = make_unique<Node>(NodeKind::While, "jabasamma", kw.line, kw.column);
    node->children.push_back(move(cond));
    node->children.push_back(move(body));
    return node;
}

NodePtr Parser::doWhileStatement() {
    const Token& kw = expectKeyword("gar", "expected 'gar'");
    auto body = block();
    expectKeyword("jabasamma", "expected 'jabasamma' after do-block");
    expect(LEFT_PAREN, "expected '(' after 'jabasamma'");
    auto cond = expression();
    expect(RIGHT_PAREN, "expected ')' after do-while condition");
    expect(SEMICOLAN, "expected ';' after do-while");

    auto node = make_unique<Node>(NodeKind::DoWhile, "gar", kw.line, kw.column);
    node->children.push_back(move(body));
    node->children.push_back(move(cond));
    return node;
}

NodePtr Parser::forStatement() {
    const Token& kw = expectKeyword("ferini", "expected 'ferini'");
    expect(LEFT_PAREN, "expected '(' after 'ferini'");

    NodePtr init;
    if (check(SEMICOLAN)) {
        init = emptyStatement();
    } else if (checkKeyword("yoho")) {
        init = declaration();
    } else {
        init = exprStatement();
    }

    NodePtr cond;
    if (!check(SEMICOLAN)) cond = expression();
    else cond = make_unique<Node>(NodeKind::Empty, ";", peek().line, peek().column);
    expect(SEMICOLAN, "expected ';' after for-loop condition");

    NodePtr post;
    if (!check(RIGHT_PAREN)) post = expression();
    else post = make_unique<Node>(NodeKind::Empty, ";", peek().line, peek().column);
    expect(RIGHT_PAREN, "expected ')' after for-loop clauses");

    if (!check(LEFT_BRECE)) {
        error(peek(), "expected '{' to start 'ferini' body");
    }
    auto body = block();

    auto node = make_unique<Node>(NodeKind::For, "ferini", kw.line, kw.column);
    node->children.push_back(move(init));
    node->children.push_back(move(cond));
    node->children.push_back(move(post));
    node->children.push_back(move(body));
    return node;
}

// ---------------- expressions ----------------

NodePtr Parser::expression() {
    return assignment();
}

NodePtr Parser::assignment() {
    auto left = logicalOr();

    if (left->kind == NodeKind::Identifier && isAssignOp(peek().type)) {
        const Token& opTok = advance();
        auto value = assignment();
        auto node = make_unique<Node>(NodeKind::Assignment, opTok.value, opTok.line, opTok.column);
        node->children.push_back(move(left));
        node->children.push_back(move(value));
        return node;
    }
    return left;
}

NodePtr Parser::logicalOr() {
    auto left = logicalAnd();
    while (check(OR_OP)) {
        const Token& op = advance();
        auto right = logicalAnd();
        auto node = make_unique<Node>(NodeKind::LogicalOp, op.value, op.line, op.column);
        node->children.push_back(move(left));
        node->children.push_back(move(right));
        left = move(node);
    }
    return left;
}

NodePtr Parser::logicalAnd() {
    auto left = equality();
    while (check(AND_OP)) {
        const Token& op = advance();
        auto right = equality();
        auto node = make_unique<Node>(NodeKind::LogicalOp, op.value, op.line, op.column);
        node->children.push_back(move(left));
        node->children.push_back(move(right));
        left = move(node);
    }
    return left;
}

NodePtr Parser::equality() {
    auto left = relational();
    while (check(EQUAL_OP) || check(NOT_EQUAL_OP)) {
        const Token& op = advance();
        auto right = relational();
        auto node = make_unique<Node>(NodeKind::BinaryOp, op.value, op.line, op.column);
        node->children.push_back(move(left));
        node->children.push_back(move(right));
        left = move(node);
    }
    return left;
}

NodePtr Parser::relational() {
    auto left = additive();
    while (check(LESS_OP) || check(LESS_EQUAL_OP) || check(GREATER_OP) || check(GREATER_EQUAL_OP)) {
        const Token& op = advance();
        auto right = additive();
        auto node = make_unique<Node>(NodeKind::BinaryOp, op.value, op.line, op.column);
        node->children.push_back(move(left));
        node->children.push_back(move(right));
        left = move(node);
    }
    return left;
}

NodePtr Parser::additive() {
    auto left = multiplicative();
    while (check(PLUS_OP) || check(MINUS_OP)) {
        const Token& op = advance();
        auto right = multiplicative();
        auto node = make_unique<Node>(NodeKind::BinaryOp, op.value, op.line, op.column);
        node->children.push_back(move(left));
        node->children.push_back(move(right));
        left = move(node);
    }
    return left;
}

NodePtr Parser::multiplicative() {
    auto left = unary();
    while (check(MULTIPLICATION_OP) || check(DIVISION_OP) || check(MODULUS_OP)) {
        const Token& op = advance();
        auto right = unary();
        auto node = make_unique<Node>(NodeKind::BinaryOp, op.value, op.line, op.column);
        node->children.push_back(move(left));
        node->children.push_back(move(right));
        left = move(node);
    }
    return left;
}

NodePtr Parser::unary() {
    if (check(MINUS_OP) || check(PLUS_OP) || check(NOT_OP)) {
        const Token& op = advance();
        auto operand = unary();
        auto node = make_unique<Node>(NodeKind::UnaryOp, op.value, op.line, op.column);
        node->children.push_back(move(operand));
        return node;
    }
    return call();
}

NodePtr Parser::call() {
    auto expr = primary();
    while (check(LEFT_PAREN)) {
        const Token& paren = advance();
        auto node = make_unique<Node>(NodeKind::Call, "", paren.line, paren.column);
        node->children.push_back(move(expr));
        if (!check(RIGHT_PAREN)) {
            do {
                node->children.push_back(expression());
            } while (match(COMMA));
        }
        expect(RIGHT_PAREN, "expected ')' after call arguments");
        expr = move(node);
    }
    return expr;
}

NodePtr Parser::primary() {
    const Token& t = peek();

    if (check(INT_NO)) { advance(); return make_unique<Node>(NodeKind::IntLiteral, t.value, t.line, t.column); }
    if (check(FLOAT_NO)) { advance(); return make_unique<Node>(NodeKind::FloatLiteral, t.value, t.line, t.column); }
    if (check(STRING)) { advance(); return make_unique<Node>(NodeKind::StringLiteral, t.value, t.line, t.column); }
    if (check(IDENTIFIER)) { advance(); return make_unique<Node>(NodeKind::Identifier, t.value, t.line, t.column); }

    if (match(LEFT_PAREN)) {
        auto expr = expression();
        expect(RIGHT_PAREN, "expected ')' after expression");
        return expr;
    }

    if (check(ERROR)) {
        error(t, "lexical error reached parser: " + (t.errorMsg.empty() ? t.value : t.errorMsg));
    }

    error(t, "expected an expression");
}