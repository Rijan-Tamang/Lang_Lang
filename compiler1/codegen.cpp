#include "codegen.h"
#include <stdexcept>
#include <functional>
//#include "error.h"

using namespace std;

static const int STR_BUF_SIZE = 256;

// ---------------- scope helpers ----------------

void CodeGenerator::pushScope() { scopes.emplace_back(); }
void CodeGenerator::popScope() { scopes.pop_back(); }

void CodeGenerator::declare(const string& name, const string& type) {
    scopes.back()[name] = type;
}

string CodeGenerator::lookupType(const string& name) const {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto f = it->find(name);
        if (f != it->end()) return f->second;
    }
    return "double"; // safe default; semantic analysis already guaranteed the name exists
}

// ---------------- string helpers ----------------

string CodeGenerator::escapeCString(const string& s) {
    string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += c;
        }
    }
    return out;
}

string CodeGenerator::escapePercent(const string& s) {
    string out;
    out.reserve(s.size());
    for (char c : s) {
        out += c;
        if (c == '%') out += '%';
    }
    return out;
}

string CodeGenerator::indentStr(int indent) const {
    return string(indent * 4, ' ');
}

// ---------------- function-signature discovery ----------------

bool CodeGenerator::literalLooksString(Node* expr, const FuncSig& sig) const {
    if (!expr) return false;
    if (expr->kind == NodeKind::StringLiteral) return true;
    if (expr->kind == NodeKind::Identifier) {
        // best-effort: only resolves if it's a name we already know about
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            auto f = it->find(expr->text);
            if (f != it->end()) return f->second == "string";
        }
    }
    (void)sig;
    return false;
}

void CodeGenerator::scanCallsForStringParams(Node* node) {
    if (!node) return;

    if (node->kind == NodeKind::Call && !node->children.empty() &&
        node->children[0]->kind == NodeKind::Identifier) {
        auto it = functions.find(node->children[0]->text);
        if (it != functions.end()) {
            for (size_t k = 1; k < node->children.size(); k++) {
                size_t paramIdx = k - 1;
                if (paramIdx < it->second.paramTypes.size()) {
                    if (node->children[k]->kind == NodeKind::StringLiteral) {
                        it->second.paramTypes[paramIdx] = "string";
                    }
                }
            }
        }
    }

    for (auto& c : node->children) scanCallsForStringParams(c.get());
    for (auto& c : node->elifConds) scanCallsForStringParams(c.get());
    for (auto& c : node->elifBodies) scanCallsForStringParams(c.get());
    if (node->elseBody) scanCallsForStringParams(node->elseBody.get());
}

void CodeGenerator::collectFunctionSignatures(Node* program) {
    // pass 1: register names + param count (default all "double")
    for (auto& child : program->children) {
        if (child->kind == NodeKind::FunctionDef || child->kind == NodeKind::FunctionDecl) {
            FuncSig sig;
            for (auto& c : child->children) {
                if (c->kind == NodeKind::Param) sig.paramTypes.push_back("double");
            }
            sig.returnType = "double";
            sig.hasBody = (child->kind == NodeKind::FunctionDef);

            auto existing = functions.find(child->text);
            if (existing == functions.end()) {
                functions[child->text] = sig;
            } else if (child->kind == NodeKind::FunctionDef) {
                existing->second.hasBody = true; // definition wins over a bare prototype
            }
        }
    }

    // pass 2: scan every call site in the whole program for string-literal
    // arguments, to upgrade the matching parameter's inferred type.
    for (auto& child : program->children) scanCallsForStringParams(child.get());

    // pass 3: infer return type per function definition by scanning its
    // direct return statements (shallow: literal or known-string param/local).
    for (auto& child : program->children) {
        if (child->kind != NodeKind::FunctionDef) continue;
        FuncSig& sig = functions[child->text];

        pushScope();
        vector<string> paramNames;
        size_t pi = 0;
        for (auto& c : child->children) {
            if (c->kind == NodeKind::Param) {
                declare(c->text, sig.paramTypes[pi]);
                pi++;
            }
        }

        bool sawReturn = false;
        bool stringReturn = false;

        function<void(Node*)> scan = [&](Node* n) {
            if (!n) return;
            if (n->kind == NodeKind::FunctionDef) return; // no nested functions in this grammar
            if (n->kind == NodeKind::Return) {
                if (!n->children.empty()) {
                    sawReturn = true;
                    if (typeOfExpr(n->children[0].get()) == "string") stringReturn = true;
                }
                return;
            }
            for (auto& c : n->children) scan(c.get());
            for (auto& c : n->elifConds) scan(c.get());
            for (auto& c : n->elifBodies) scan(c.get());
            if (n->elseBody) scan(n->elseBody.get());
        };
        for (auto& c : child->children) {
            if (c->kind == NodeKind::Block) scan(c.get());
        }
        popScope();

        sig.returnType = !sawReturn ? "void" : (stringReturn ? "string" : "double");
    }
}

// ---------------- type + expression emission ----------------

string CodeGenerator::typeOfExpr(Node* expr) const {
    switch (expr->kind) {
        case NodeKind::IntLiteral:
        case NodeKind::FloatLiteral:
            return "double";
        case NodeKind::StringLiteral:
            return "string";
        case NodeKind::Identifier:
            return lookupType(expr->text);
        case NodeKind::Assignment:
            return lookupType(expr->children[0]->text);
        case NodeKind::LogicalOp:
        case NodeKind::UnaryOp:
            return "double";
        case NodeKind::BinaryOp: {
            if (expr->text == "+") {
                string l = typeOfExpr(expr->children[0].get());
                string r = typeOfExpr(expr->children[1].get());
                if (l == "string" || r == "string") return "string";
            }
            return "double";
        }
        case NodeKind::Call: {
            if (expr->children[0]->kind == NodeKind::Identifier) {
                auto it = functions.find(expr->children[0]->text);
                if (it != functions.end()) return it->second.returnType == "string" ? "string" : "double";
            }
            return "double";
        }
        default:
            return "double";
    }
}

string CodeGenerator::emitExpr(Node* expr) {
    switch (expr->kind) {
        case NodeKind::IntLiteral:
            return expr->text + ".0"; // force a C `double` literal, not `int`

        case NodeKind::FloatLiteral:
            return expr->text;

        case NodeKind::StringLiteral:
            return "\"" + escapeCString(expr->text) + "\"";

        case NodeKind::Identifier:
            return expr->text;

        case NodeKind::Assignment: {
            string target = expr->children[0]->text;
            string valExpr = emitExpr(expr->children[1].get());
            string targetType = lookupType(target);
            const string& op = expr->text;

            if (targetType == "string") {
                if (op == "=") return "strcpy(" + target + ", " + valExpr + ")";
                // only "+=" survives semantic checking for strings
                return "strcpy(" + target + ", __concat(" + target + ", " + valExpr + "))";
            }
            if (op == "%=") return "(" + target + " = fmod(" + target + ", " + valExpr + "))";
            return "(" + target + " " + op + " " + valExpr + ")";
        }

        case NodeKind::LogicalOp: {
            string op = (expr->text == "||") ? "||" : "&&";
            return "(" + emitExpr(expr->children[0].get()) + " " + op + " " +
                   emitExpr(expr->children[1].get()) + ")";
        }

        case NodeKind::BinaryOp: {
            Node* lNode = expr->children[0].get();
            Node* rNode = expr->children[1].get();
            string lt = typeOfExpr(lNode);
            string rt = typeOfExpr(rNode);
            const string& op = expr->text;

            if (op == "+" && (lt == "string" || rt == "string")) {
                return "__concat(" + emitExpr(lNode) + ", " + emitExpr(rNode) + ")";
            }
            if ((op == "==" || op == "!=") && lt == "string" && rt == "string") {
                string cmp = "strcmp(" + emitExpr(lNode) + ", " + emitExpr(rNode) + ")";
                return op == "==" ? "(" + cmp + " == 0)" : "(" + cmp + " != 0)";
            }
            if (op == "%") {
                return "fmod(" + emitExpr(lNode) + ", " + emitExpr(rNode) + ")";
            }
            return "(" + emitExpr(lNode) + " " + op + " " + emitExpr(rNode) + ")";
        }

        case NodeKind::UnaryOp: {
            string inner = emitExpr(expr->children[0].get());
            if (expr->text == "!") return "(!(" + inner + "))";
            return "(" + expr->text + "(" + inner + "))";
        }

        case NodeKind::Call: {
            string callee = expr->children[0]->text;
            string args;
            for (size_t k = 1; k < expr->children.size(); k++) {
                if (k > 1) args += ", ";
                args += emitExpr(expr->children[k].get());
            }
            return callee + "(" + args + ")";
        }

        default:
            throw runtime_error("codegen: cannot emit expression node");
    }
}

// ---------------- statement emission ----------------

string CodeGenerator::emitForInit(Node* init, ostringstream& preStatements, int indent) {
    if (init->kind == NodeKind::Empty) return "";

    if (init->kind == NodeKind::VarDecl) {
        // Common case: all declared vars share one C type -> inline "double i = 0, j = 1".
        // Mixed-type multi-decls fall back to hoisting extras before the loop.
        string first;
        bool firstDone = false;
        string firstType;

        for (auto& idNode : init->children) {
            string type = idNode->children.empty() ? "double" : typeOfExpr(idNode->children[0].get());
            declare(idNode->text, type);

            if (!firstDone) {
                firstType = type;
                if (type == "string") {
                    preStatements << indentStr(indent) << "static char " << idNode->text << "[" << STR_BUF_SIZE << "];\n";
                    string initExpr = idNode->children.empty() ? "\"\"" : emitExpr(idNode->children[0].get());
                    first = "(strcpy(" + idNode->text + ", " + initExpr + ") ? 1 : 1)"; // placeholder, unused as init text
                    // strings can't live in the for(...) header (array decl not allowed there),
                    // so hoist the declaration above the loop and leave the header slot empty.
                    preStatements << indentStr(indent) << "strcpy(" << idNode->text << ", "
                                  << (idNode->children.empty() ? "\"\"" : emitExpr(idNode->children[0].get())) << ");\n";
                    first.clear();
                } else {
                    first = "double " + idNode->text + " = " +
                            (idNode->children.empty() ? "0" : emitExpr(idNode->children[0].get()));
                }
                firstDone = true;
            } else if (type == firstType && type == "double") {
                first += ", " + idNode->text + " = " +
                         (idNode->children.empty() ? "0" : emitExpr(idNode->children[0].get()));
            } else {
                // different type than the header's declared type -> hoist above the loop
                if (type == "string") {
                    preStatements << indentStr(indent) << "static char " << idNode->text << "[" << STR_BUF_SIZE << "];\n";
                    preStatements << indentStr(indent) << "strcpy(" << idNode->text << ", "
                                  << (idNode->children.empty() ? "\"\"" : emitExpr(idNode->children[0].get())) << ");\n";
                } else {
                    preStatements << indentStr(indent) << "double " << idNode->text << " = "
                                  << (idNode->children.empty() ? "0" : emitExpr(idNode->children[0].get())) << ";\n";
                }
            }
        }
        return first;
    }

    if (init->kind == NodeKind::ExprStatement) {
        return emitExpr(init->children[0].get());
    }

    return "";
}

void CodeGenerator::emitBlockBody(Node* block, ostringstream& out, int indent) {
    for (auto& stmt : block->children) emitStatement(stmt.get(), out, indent);
}

void CodeGenerator::emitStatement(Node* node, ostringstream& out, int indent) {
    string pad = indentStr(indent);

    switch (node->kind) {
        case NodeKind::VarDecl: {
            for (auto& idNode : node->children) {
                string type = idNode->children.empty() ? "double" : typeOfExpr(idNode->children[0].get());
                declare(idNode->text, type);
                if (type == "string") {
                    out << pad << "static char " << idNode->text << "[" << STR_BUF_SIZE << "];\n";
                    string initExpr = idNode->children.empty() ? "\"\"" : emitExpr(idNode->children[0].get());
                    out << pad << "strcpy(" << idNode->text << ", " << initExpr << ");\n";
                } else {
                    string initExpr = idNode->children.empty() ? "0" : emitExpr(idNode->children[0].get());
                    out << pad << "double " << idNode->text << " = " << initExpr << ";\n";
                }
            }
            break;
        }

        case NodeKind::Block:
            out << pad << "{\n";
            pushScope();
            emitBlockBody(node, out, indent + 1);
            popScope();
            out << pad << "}\n";
            break;

        case NodeKind::Return:
            if (node->children.empty()) out << pad << "return;\n";
            else out << pad << "return " << emitExpr(node->children[0].get()) << ";\n";
            break;

        case NodeKind::ExprStatement:
            out << pad << emitExpr(node->children[0].get()) << ";\n";
            break;

        case NodeKind::Empty:
            break;

        case NodeKind::Print: {
            string fmt;
            string args;
            bool first = true;
            for (auto& arg : node->children) {
                if (!first) fmt += " ";
                first = false;
                if (arg->kind == NodeKind::StringLiteral) {
                    fmt += escapeCString(escapePercent(arg->text));
                } else {
                    fmt += (typeOfExpr(arg.get()) == "string") ? "%s" : "%g";
                    args += ", " + emitExpr(arg.get());
                }
            }
            fmt += "\\n";
            out << pad << "printf(\"" << fmt << "\"" << args << ");\n";
            break;
        }

        case NodeKind::Input: {
            out << pad << "printf(\"%s\", \"" << escapeCString(escapePercent(node->text)) << "\");\n";
            for (auto& idNode : node->children) {
                string type = lookupType(idNode->text);
                if (type == "string") {
                    out << pad << "scanf(\"%" << (STR_BUF_SIZE - 1) << "s\", " << idNode->text << ");\n";
                } else {
                    out << pad << "scanf(\"%lf\", &" << idNode->text << ");\n";
                }
            }
            break;
        }

        case NodeKind::FunctionDecl:
            break; // signature already emitted as a prototype

        case NodeKind::FunctionDef:
            throw runtime_error("codegen: nested function definitions are not supported");

        case NodeKind::If: {
            out << pad << "if (" << emitExpr(node->children[0].get()) << ") {\n";
            pushScope();
            if (node->children[1]->kind == NodeKind::Block) emitBlockBody(node->children[1].get(), out, indent + 1);
            else emitStatement(node->children[1].get(), out, indent + 1);
            popScope();
            out << pad << "}\n";

            for (size_t k = 0; k < node->elifConds.size(); k++) {
                out << pad << "else if (" << emitExpr(node->elifConds[k].get()) << ") {\n";
                pushScope();
                if (node->elifBodies[k]->kind == NodeKind::Block) emitBlockBody(node->elifBodies[k].get(), out, indent + 1);
                else emitStatement(node->elifBodies[k].get(), out, indent + 1);
                popScope();
                out << pad << "}\n";
            }

            if (node->elseBody) {
                out << pad << "else {\n";
                pushScope();
                if (node->elseBody->kind == NodeKind::Block) emitBlockBody(node->elseBody.get(), out, indent + 1);
                else emitStatement(node->elseBody.get(), out, indent + 1);
                popScope();
                out << pad << "}\n";
            }
            break;
        }

        case NodeKind::Switch: {
            // Translated to if/else-if (no fallthrough) rather than a native C
            // switch, since case labels here aren't restricted to integer
            // constants the way C requires.
            string subject = emitExpr(node->children[0].get());
            bool first = true;
            for (size_t idx = 1; idx < node->children.size(); idx++) {
                Node* clause = node->children[idx].get();
                if (clause->kind == NodeKind::Case) {
                    string label = emitExpr(clause->children[0].get());
                    out << pad << (first ? "if (" : "else if (") << "(" << subject << ") == (" << label << ")) {\n";
                    first = false;
                    pushScope();
                    for (size_t s = 1; s < clause->children.size(); s++) emitStatement(clause->children[s].get(), out, indent + 1);
                    popScope();
                    out << pad << "}\n";
                } else { // Default
                    out << pad << (first ? "if (1) {\n" : "else {\n");
                    first = false;
                    pushScope();
                    for (auto& s : clause->children) emitStatement(s.get(), out, indent + 1);
                    popScope();
                    out << pad << "}\n";
                }
            }
            break;
        }

        case NodeKind::While: {
            out << pad << "while (" << emitExpr(node->children[0].get()) << ") {\n";
            pushScope();
            if (node->children[1]->kind == NodeKind::Block) emitBlockBody(node->children[1].get(), out, indent + 1);
            else emitStatement(node->children[1].get(), out, indent + 1);
            popScope();
            out << pad << "}\n";
            break;
        }

        case NodeKind::DoWhile: {
            out << pad << "do {\n";
            pushScope();
            emitBlockBody(node->children[0].get(), out, indent + 1);
            popScope();
            out << pad << "} while (" << emitExpr(node->children[1].get()) << ");\n";
            break;
        }

        case NodeKind::For: {
            pushScope();
            ostringstream pre;
            string initText = emitForInit(node->children[0].get(), pre, indent);
            out << pre.str();

            string condText = (node->children[1]->kind == NodeKind::Empty) ? "" : emitExpr(node->children[1].get());
            string postText = (node->children[2]->kind == NodeKind::Empty) ? "" : emitExpr(node->children[2].get());

            out << pad << "for (" << initText << "; " << condText << "; " << postText << ") {\n";
            if (node->children[3]->kind == NodeKind::Block) {
                pushScope();
                emitBlockBody(node->children[3].get(), out, indent + 1);
                popScope();
            } else {
                emitStatement(node->children[3].get(), out, indent + 1);
            }
            out << pad << "}\n";
            popScope();
            break;
        }

        case NodeKind::Break:
            out << pad << "break;\n";
            break;

        case NodeKind::Continue:
            out << pad << "continue;\n";
            break;

        default:
            out << pad << emitExpr(node) << ";\n";
            break;
    }
}

// ---------------- functions & program ----------------

void CodeGenerator::emitFunction(Node* fn, ostringstream& out) {
    FuncSig& sig = functions[fn->text];
    string cReturn = sig.returnType == "string" ? "char*" : (sig.returnType == "void" ? "void" : "double");

    string paramList;
    pushScope();
    size_t pi = 0;
    for (auto& c : fn->children) {
        if (c->kind == NodeKind::Param) {
            string ptype = sig.paramTypes[pi];
            declare(c->text, ptype);
            if (!paramList.empty()) paramList += ", ";
            paramList += (ptype == "string" ? "char* " : "double ") + c->text;
            pi++;
        }
    }
    if (paramList.empty()) paramList = "void";

    out << cReturn << " " << fn->text << "(" << paramList << ") {\n";
    for (auto& c : fn->children) {
        if (c->kind == NodeKind::Block) emitBlockBody(c.get(), out, 1);
    }
    out << "}\n\n";
    popScope();
}

string CodeGenerator::generate(Node* program) {
    scopes.clear();
    functions.clear();
    protos.str("");
    funcDefs.str("");
    mainBody.str("");

    pushScope(); // global scope (top-level vars visible to main)
    collectFunctionSignatures(program);

    for (auto& child : program->children) {
        if (child->kind == NodeKind::FunctionDef || child->kind == NodeKind::FunctionDecl) {
            FuncSig& sig = functions[child->text];
            string cReturn = sig.returnType == "string" ? "char*" : (sig.returnType == "void" ? "void" : "double");
            string paramList;
            for (size_t k = 0; k < sig.paramTypes.size(); k++) {
                if (k > 0) paramList += ", ";
                paramList += (sig.paramTypes[k] == "string" ? "char*" : "double");
            }
            if (paramList.empty()) paramList = "void";
            protos << cReturn << " " << child->text << "(" << paramList << ");\n";
        }
    }

    for (auto& child : program->children) {
        if (child->kind == NodeKind::FunctionDef) {
            emitFunction(child.get(), funcDefs);
        }
    }

    mainBody << "int main(void) {\n";
    for (auto& child : program->children) {
        if (child->kind != NodeKind::FunctionDef && child->kind != NodeKind::FunctionDecl) {
            emitStatement(child.get(), mainBody, 1);
        }
    }
    mainBody << "    return 0;\n}\n";
    popScope();

    ostringstream final_;
    final_ <<
        "/* Auto-generated by the toy-language -> C transpiler. */\n"
        "/* Numbers are unified to `double`; strings use fixed static buffers   */\n"
        "/* (STR_BUF_SIZE bytes). Because string buffers are `static`, this     */\n"
        "/* code is NOT safe for recursive functions that use string locals    */\n"
        "/* or return strings. Requires a C99-or-later compiler.               */\n"
        "#include <stdio.h>\n"
        "#include <string.h>\n"
        "#include <math.h>\n\n"
        "#define __STR_POOL 16\n"
        "static char __strbuf[__STR_POOL][" << STR_BUF_SIZE << "];\n"
        "static int __strbuf_idx = 0;\n"
        "static char* __concat(const char* a, const char* b) {\n"
        "    char* dst = __strbuf[__strbuf_idx];\n"
        "    __strbuf_idx = (__strbuf_idx + 1) % __STR_POOL;\n"
        "    snprintf(dst, " << STR_BUF_SIZE << ", \"%s%s\", a, b);\n"
        "    return dst;\n"
        "}\n\n"
        << protos.str() << "\n"
        << funcDefs.str()
        << mainBody.str();

    return final_.str();
}