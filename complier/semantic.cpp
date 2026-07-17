#include "semantic.h"

using namespace std;

// ---------------- scope helpers ----------------

void SemanticAnalyzer::pushScope() {
    scopes.emplace_back();
}

void SemanticAnalyzer::popScope() {
    scopes.pop_back();
}

bool SemanticAnalyzer::declareVar(const string& name, const string& type, int line, int col) {
    auto& current = scopes.back();
    if (current.count(name)) {
        error(line, col, "redeclaration of variable '" + name + "' in the same scope");
        return false;
    }
    current[name] = VarSymbol{type, line, col};
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

bool SemanticAnalyzer::isNumeric(const string& type) const {
    return type == "int" || type == "float";
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
                string type = "unknown";
                if (!idNode->children.empty()) {
                    type = visitExpression(idNode->children[0].get());
                }
                declareVar(idNode->text, type, idNode->line, idNode->column);
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
                    declareVar(child->text, "unknown", child->line, child->column);
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

string SemanticAnalyzer::visitExpression(Node* node) {
    if (!node) return "unknown";

    switch (node->kind) {
        case NodeKind::IntLiteral: return "int";
        case NodeKind::FloatLiteral: return "float";
        case NodeKind::StringLiteral: return "string";

        case NodeKind::Identifier: {
            VarSymbol* sym = lookupVar(node->text);
            if (!sym) {
                error(node->line, node->column, "use of undeclared identifier '" + node->text + "'");
                return "unknown";
            }
            return sym->type;
        }

        case NodeKind::Assignment: {
            Node* target = node->children[0].get();
            string valType = visitExpression(node->children[1].get());

            VarSymbol* sym = lookupVar(target->text);
            if (!sym) {
                error(target->line, target->column,
                      "assignment to undeclared variable '" + target->text + "'");
                return valType;
            }

            if (node->text != "=") { // compound assignment: +=, -=, *=, /=, %=
                if (sym->type != "unknown" && valType != "unknown") {
                    bool okString = (sym->type == "string" && valType == "string" && node->text == "+=");
                    if (!okString && (!isNumeric(sym->type) || !isNumeric(valType))) {
                        error(node->line, node->column,
                              "invalid operand types for '" + node->text + "': " +
                              sym->type + " and " + valType);
                    }
                }
            } else if (sym->type == "unknown") {
                sym->type = valType; // refine type on first real assignment
            } else if (valType != "unknown" && sym->type != valType) {
                warn(node->line, node->column,
                     "assigning " + valType + " to variable '" + target->text +
                     "' previously inferred as " + sym->type);
            }
            return sym->type;
        }

        case NodeKind::LogicalOp: {
            string l = visitExpression(node->children[0].get());
            string r = visitExpression(node->children[1].get());
            if (l == "string" || r == "string") {
                warn(node->line, node->column,
                     "'" + node->text + "' used with a string operand");
            }
            return "int";
        }

        case NodeKind::BinaryOp: {
            string l = visitExpression(node->children[0].get());
            string r = visitExpression(node->children[1].get());
            const string& op = node->text;

            bool comparison = (op == "==" || op == "!=" || op == "<" || op == "<=" ||
                                op == ">" || op == ">=");

            if (l == "unknown" || r == "unknown") return comparison ? "int" : "unknown";

            if (op == "+") {
                if (l == "string" || r == "string") {
                    if (l != "string" || r != "string") {
                        error(node->line, node->column,
                              "cannot use '+' between string and " + (l == "string" ? r : l) +
                              " without explicit conversion");
                    }
                    return "string";
                }
                return (l == "float" || r == "float") ? "float" : "int";
            }

            bool equalityOp = (op == "==" || op == "!=");
            if (equalityOp && l == "string" && r == "string") {
                return "int"; // string equality is valid (compares contents)
            }

            if (l == "string" || r == "string") {
                error(node->line, node->column,
                      "invalid operand types for '" + op + "': " + l + " and " + r);
                return comparison ? "int" : "unknown";
            }

            if (comparison) return "int";
            return (l == "float" || r == "float") ? "float" : "int";
        }

        case NodeKind::UnaryOp: {
            string t = visitExpression(node->children[0].get());
            if (node->text == "!") return "int";
            if (t != "unknown" && !isNumeric(t)) {
                error(node->line, node->column,
                      "invalid operand type for unary '" + node->text + "': " + t);
            }
            return t;
        }

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
            return "unknown";
        }

        default:
            return "unknown";
    }
}