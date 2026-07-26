#include "semantic.h"

using namespace std;

// ---------------- scope helpers ----------------

void SemanticAnalyzer::pushScope() {
    scopes.emplace_back();
}

void SemanticAnalyzer::popScope() {
    scopes.pop_back();
}

bool SemanticAnalyzer::declareVar(const string& name, int line, int col) {
    auto& current = scopes.back();
    if (current.count(name)) {
        error(line, col, "redeclaration of variable '" + name + "' in the same scope");
        return false;
    }
    current[name] = VarSymbol{line, col};
    return true;
}

VarSymbol* SemanticAnalyzer::lookupVar(const string& name) {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) return &found->second;
    }
    return nullptr;
}

void SemanticAnalyzer::error(int line, int col, const string& msg) {
    diags.push_back({DiagSeverity::Error, msg, line, col});
}

void SemanticAnalyzer::warn(int line, int col, const string& msg) {
    diags.push_back({DiagSeverity::Warning, msg, line, col});
}

// ---------------- entry point ----------------

vector<Diagnostic> SemanticAnalyzer::analyze(Node* program) {
    diags.clear();
    scopes.clear();
    functions.clear();
    loopDepth = 0;
    functionDepth = 0;

    pushScope(); // global scope
    collectFunctionSignatures(program);
    for (auto& child : program->children) {
        visitStatement(child.get());
    }
    popScope();

    return diags;
}

// Pre-pass so functions can be called before their textual definition.
void SemanticAnalyzer::collectFunctionSignatures(Node* program) {
    for (auto& child : program->children) {
        if (child->kind == NodeKind::FunctionDef || child->kind == NodeKind::FunctionDecl) {
            int paramCount = 0;
            for (auto& c : child->children) {
                if (c->kind == NodeKind::Param) paramCount++;
            }
            bool isDef = child->kind == NodeKind::FunctionDef;

            auto it = functions.find(child->text);
            if (it == functions.end()) {
                functions[child->text] = FuncSymbol{paramCount, isDef, child->line, child->column};
            } else {
                if (it->second.paramCount != paramCount) {
                    warn(child->line, child->column,
                         "function '" + child->text + "' redeclared with a different parameter count");
                }
                if (isDef) {
                    if (it->second.defined) {
                        error(child->line, child->column,
                              "function '" + child->text + "' already defined");
                    }
                    it->second.defined = true;
                    it->second.line = child->line;
                    it->second.column = child->column;
                }
            }
        }
    }
}

// ---------------- statements ----------------

void SemanticAnalyzer::visitBlockScoped(Node* block) {
    pushScope();
    for (auto& stmt : block->children) visitStatement(stmt.get());
    popScope();
}

void SemanticAnalyzer::visitStatement(Node* node) {
    if (!node) return;

    switch (node->kind) {
        case NodeKind::VarDecl: {
            for (auto& idNode : node->children) {
                if (!idNode->children.empty()) {
                    visitExpression(idNode->children[0].get());
                }
                declareVar(idNode->text, idNode->line, idNode->column);
            }
            break;
        }

        case NodeKind::Block:
            visitBlockScoped(node);
            break;

        case NodeKind::Return:
            if (functionDepth == 0) {
                error(node->line, node->column, "'firta' (return) used outside a function");
            }
            if (!node->children.empty()) visitExpression(node->children[0].get());
            break;

        case NodeKind::ExprStatement:
            visitExpression(node->children[0].get());
            break;

        case NodeKind::Empty:
            break;

        case NodeKind::Print:
            for (auto& arg : node->children) visitExpression(arg.get());
            break;

        case NodeKind::Input:
            for (auto& idNode : node->children) {
                if (!lookupVar(idNode->text)) {
                    error(idNode->line, idNode->column,
                          "'sun' target '" + idNode->text + "' is not declared");
                }
            }
            break;

        case NodeKind::FunctionDecl:
            if (functionDepth > 0 || scopes.size() > 1) {
                error(node->line, node->column,
                      "nested function declarations are not supported - '" + node->text +
                      "' must be declared at the top level");
            }
            // signature already recorded in the pre-pass; nothing else to check here.
            break;

        case NodeKind::FunctionDef: {
            if (functionDepth > 0 || scopes.size() > 1) {
                error(node->line, node->column,
                      "nested function definitions are not supported - '" + node->text +
                      "' must be defined at the top level");
                break; // don't also process its body as if it were valid
            }
            functionDepth++;
            pushScope();
            for (auto& child : node->children) {
                if (child->kind == NodeKind::Param) {
                    declareVar(child->text, child->line, child->column);
                } else if (child->kind == NodeKind::Block) {
                    for (auto& stmt : child->children) visitStatement(stmt.get());
                }
            }
            popScope();
            functionDepth--;
            break;
        }

        case NodeKind::If: {
            visitExpression(node->children[0].get());
            visitStatement(node->children[1].get());
            for (size_t k = 0; k < node->elifConds.size(); k++) {
                visitExpression(node->elifConds[k].get());
                visitStatement(node->elifBodies[k].get());
            }
            if (node->elseBody) visitStatement(node->elseBody.get());
            break;
        }

        case NodeKind::Switch: {
            visitExpression(node->children[0].get());
            for (size_t idx = 1; idx < node->children.size(); idx++) {
                Node* clause = node->children[idx].get();
                pushScope();
                if (clause->kind == NodeKind::Case) {
                    Node* label = clause->children[0].get();
                    if (label->kind == NodeKind::Identifier && !lookupVar(label->text)) {
                        error(label->line, label->column,
                              "case label '" + label->text + "' is not a declared identifier");
                    }
                    for (size_t s = 1; s < clause->children.size(); s++) {
                        visitStatement(clause->children[s].get());
                    }
                } else { // Default
                    for (auto& stmt : clause->children) visitStatement(stmt.get());
                }
                popScope();
            }
            break;
        }

        case NodeKind::While:
            visitExpression(node->children[0].get());
            loopDepth++;
            visitStatement(node->children[1].get());
            loopDepth--;
            break;

        case NodeKind::DoWhile:
            loopDepth++;
            visitStatement(node->children[0].get());
            loopDepth--;
            visitExpression(node->children[1].get());
            break;

        case NodeKind::For: {
            pushScope();
            visitStatement(node->children[0].get()); // init
            if (node->children[1]->kind != NodeKind::Empty) visitExpression(node->children[1].get());
            if (node->children[2]->kind != NodeKind::Empty) visitExpression(node->children[2].get());
            loopDepth++;
            visitStatement(node->children[3].get());
            loopDepth--;
            popScope();
            break;
        }

        case NodeKind::Break:
            if (loopDepth == 0) error(node->line, node->column, "'voyo' (break) used outside a loop");
            break;

        case NodeKind::Continue:
            if (loopDepth == 0) error(node->line, node->column, "'aghibad' (continue) used outside a loop");
            break;

        default:
            // Any other node reached as a "statement" is an expression used standalone.
            visitExpression(node);
            break;
    }
}

// ---------------- expressions ----------------
// Dynamic typing: these checks are purely about validity (is the name
// declared? does the call have the right arity?), never about type
// compatibility - a variable's type is only known at runtime now.

void SemanticAnalyzer::visitExpression(Node* node) {
    if (!node) return;

    switch (node->kind) {
        case NodeKind::IntLiteral:
        case NodeKind::FloatLiteral:
        case NodeKind::StringLiteral:
            break;

        case NodeKind::Identifier:
            if (!lookupVar(node->text)) {
                error(node->line, node->column, "use of undeclared identifier '" + node->text + "'");
            }
            break;

        case NodeKind::Assignment: {
            Node* target = node->children[0].get();
            visitExpression(node->children[1].get());
            if (!lookupVar(target->text)) {
                error(target->line, target->column,
                      "assignment to undeclared variable '" + target->text + "'");
            }
            break;
        }

        case NodeKind::LogicalOp:
        case NodeKind::BinaryOp:
            visitExpression(node->children[0].get());
            visitExpression(node->children[1].get());
            break;

        case NodeKind::UnaryOp:
            visitExpression(node->children[0].get());
            break;

        case NodeKind::Call: {
            Node* callee = node->children[0].get();
            size_t argCount = node->children.size() - 1;

            if (callee->kind == NodeKind::Identifier) {
                auto it = functions.find(callee->text);
                if (it == functions.end()) {
                    error(callee->line, callee->column,
                          "call to undeclared function '" + callee->text + "'");
                } else if ((size_t)it->second.paramCount != argCount) {
                    error(node->line, node->column,
                          "function '" + callee->text + "' expects " +
                          to_string(it->second.paramCount) + " argument(s), got " +
                          to_string(argCount));
                }
            } else {
                visitExpression(callee);
            }

            for (size_t k = 1; k < node->children.size(); k++) {
                visitExpression(node->children[k].get());
            }
            break;
        }

        default:
            break;
    }
}