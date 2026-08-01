#include "code_generator.h"
#include <stdexcept>
#include <functional>

using namespace std;

static const int STR_BUF_SIZE = 256;

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

void CodeGenerator::collectFunctionSignatures(Node* program) {
    for (auto& child : program->children) {
        if (child->kind != NodeKind::FunctionDef && child->kind != NodeKind::FunctionDecl) continue;

        FuncSig sig;
        for (auto& c : child->children) {
            if (c->kind == NodeKind::Param) sig.paramCount++;
        }
        sig.hasBody = (child->kind == NodeKind::FunctionDef);

        if (child->kind == NodeKind::FunctionDef) {
            bool sawReturnValue = false;
            function<void(Node*)> scan = [&](Node* n) {
                if (!n || sawReturnValue) return;
                if (n->kind == NodeKind::FunctionDef) return; // no nested functions in this grammar
                if (n->kind == NodeKind::Return) {
                    if (!n->children.empty()) sawReturnValue = true;
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
            sig.returnsValue = sawReturnValue;
        }

        auto existing = functions.find(child->text);
        if (existing == functions.end()) {
            functions[child->text] = sig;
        } else if (child->kind == NodeKind::FunctionDef) {
            existing->second.hasBody = true;
            existing->second.returnsValue = sig.returnsValue;
        }
    }
}

// ---------------- expression emission ----------------

string CodeGenerator::emitExpr(Node* expr) {
    switch (expr->kind) {
        case NodeKind::IntLiteral:
        case NodeKind::FloatLiteral:
            return "mkNum(" + expr->text + ")";

        case NodeKind::StringLiteral:
            return "mkStr(\"" + escapeCString(expr->text) + "\")";

        case NodeKind::Identifier:
            return expr->text;

        case NodeKind::Assignment: {
            string target = expr->children[0]->text;
            string valExpr = emitExpr(expr->children[1].get());
            const string& op = expr->text;

            if (op == "=")  return "(" + target + " = " + valExpr + ")";
            if (op == "+=") return "(" + target + " = val_add(" + target + ", " + valExpr + "))";
            if (op == "-=") return "(" + target + " = val_sub(" + target + ", " + valExpr + "))";
            if (op == "*=") return "(" + target + " = val_mul(" + target + ", " + valExpr + "))";
            if (op == "/=") return "(" + target + " = val_div(" + target + ", " + valExpr + "))";
            /* "%=" */       return "(" + target + " = val_mod(" + target + ", " + valExpr + "))";
        }

        case NodeKind::LogicalOp: {
            string fn = (expr->text == "||") ? "val_or" : "val_and";
            return fn + "(" + emitExpr(expr->children[0].get()) + ", " + emitExpr(expr->children[1].get()) + ")";
        }

        case NodeKind::BinaryOp: {
            string l = emitExpr(expr->children[0].get());
            string r = emitExpr(expr->children[1].get());
            const string& op = expr->text;
            string fn;
            if (op == "+") fn = "val_add";
            else if (op == "-") fn = "val_sub";
            else if (op == "*") fn = "val_mul";
            else if (op == "/") fn = "val_div";
            else if (op == "%") fn = "val_mod";
            else if (op == "==") fn = "val_eq";
            else if (op == "!=") fn = "val_ne";
            else if (op == "<") fn = "val_lt";
            else if (op == "<=") fn = "val_le";
            else if (op == ">") fn = "val_gt";
            else /* ">=" */ fn = "val_ge";
            return fn + "(" + l + ", " + r + ")";
        }

        case NodeKind::UnaryOp: {
            string inner = emitExpr(expr->children[0].get());
            if (expr->text == "!") return "val_not(" + inner + ")";
            if (expr->text == "-") return "val_neg(" + inner + ")";
            return "val_pos(" + inner + ")";
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

string CodeGenerator::emitForInit(Node* init, int indent) {
    (void)indent;
    if (init->kind == NodeKind::Empty) return "";

    if (init->kind == NodeKind::VarDecl) {
        // Every declared variable is uniformly `Value`, so multiple
        // comma-separated declarations can always share one header slot.
        string text;
        bool first = true;
        for (auto& idNode : init->children) {
            if (!first) text += ", ";
            first = false;
            string initExpr = idNode->children.empty() ? "mkNum(0)" : emitExpr(idNode->children[0].get());
            if (text.empty() || text == ", ") {
                // unreachable guard, kept simple below instead
            }
            text += idNode->text + " = " + initExpr;
        }
        return "Value " + text;
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
                string initExpr = idNode->children.empty() ? "mkNum(0)" : emitExpr(idNode->children[0].get());
                out << pad << "Value " << idNode->text << " = " << initExpr << ";\n";
            }
            break;
        }

        case NodeKind::Block:
            out << pad << "{\n";
            emitBlockBody(node, out, indent + 1);
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
            bool first = true;
            for (auto& arg : node->children) {
                if (!first) out << pad << "printf(\" \");\n";
                first = false;
                out << pad << "print_value(" << emitExpr(arg.get()) << ");\n";
            }
            out << pad << "printf(\"\");\n"; // /n hatayo vane c ko jstai print func hunxa
            break;
        }

        case NodeKind::Input: {
            out << pad << "printf(\"%s\", \"" << escapeCString(escapePercent(node->text)) << "\");\n";
            for (auto& idNode : node->children) {
                out << pad << idNode->text << " = read_value();\n";
            }
            break;
        }

        case NodeKind::FunctionDecl:
            break; // signature already emitted as a prototype

        case NodeKind::FunctionDef:
            throw runtime_error("Codegen: arko function bhitra function define garna mildaina.");

        case NodeKind::If: {
            out << pad << "if (truthy(" << emitExpr(node->children[0].get()) << ")) {\n";
            if (node->children[1]->kind == NodeKind::Block) emitBlockBody(node->children[1].get(), out, indent + 1);
            else emitStatement(node->children[1].get(), out, indent + 1);
            out << pad << "}\n";

            for (size_t k = 0; k < node->elifConds.size(); k++) {
                out << pad << "else if (truthy(" << emitExpr(node->elifConds[k].get()) << ")) {\n";
                if (node->elifBodies[k]->kind == NodeKind::Block) emitBlockBody(node->elifBodies[k].get(), out, indent + 1);
                else emitStatement(node->elifBodies[k].get(), out, indent + 1);
                out << pad << "}\n";
            }

            if (node->elseBody) {
                out << pad << "else {\n";
                if (node->elseBody->kind == NodeKind::Block) emitBlockBody(node->elseBody.get(), out, indent + 1);
                else emitStatement(node->elseBody.get(), out, indent + 1);
                out << pad << "}\n";
            }
            break;
        }

        case NodeKind::Switch: {
            string subject = emitExpr(node->children[0].get());
            bool first = true;
            for (size_t idx = 1; idx < node->children.size(); idx++) {
                Node* clause = node->children[idx].get();
                if (clause->kind == NodeKind::Case) {
                    string label = emitExpr(clause->children[0].get());
                    out << pad << (first ? "if (" : "else if (") << "val_equals(" << subject << ", " << label << ")) {\n";
                    first = false;
                    for (size_t s = 1; s < clause->children.size(); s++) emitStatement(clause->children[s].get(), out, indent + 1);
                    out << pad << "}\n";
                } else { // Default
                    out << pad << (first ? "if (1) {\n" : "else {\n");
                    first = false;
                    for (auto& s : clause->children) emitStatement(s.get(), out, indent + 1);
                    out << pad << "}\n";
                }
            }
            break;
        }

        case NodeKind::While: {
            out << pad << "while (truthy(" << emitExpr(node->children[0].get()) << ")) {\n";
            if (node->children[1]->kind == NodeKind::Block) emitBlockBody(node->children[1].get(), out, indent + 1);
            else emitStatement(node->children[1].get(), out, indent + 1);
            out << pad << "}\n";
            break;
        }

        case NodeKind::DoWhile: {
            out << pad << "do {\n";
            emitBlockBody(node->children[0].get(), out, indent + 1);
            out << pad << "} while (truthy(" << emitExpr(node->children[1].get()) << "));\n";
            break;
        }

        case NodeKind::For: {
            string initText = emitForInit(node->children[0].get(), indent);
            string condText = (node->children[1]->kind == NodeKind::Empty) ? "" : ("truthy(" + emitExpr(node->children[1].get()) + ")");
            string postText = (node->children[2]->kind == NodeKind::Empty) ? "" : emitExpr(node->children[2].get());

            out << pad << "for (" << initText << "; " << condText << "; " << postText << ") {\n";
            if (node->children[3]->kind == NodeKind::Block) {
                emitBlockBody(node->children[3].get(), out, indent + 1);
            } else {
                emitStatement(node->children[3].get(), out, indent + 1);
            }
            out << pad << "}\n";
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

void CodeGenerator::emitGlobalVarDecl(Node* decl, ostringstream& out) {
    for (auto& idNode : decl->children) {
        out << "Value " << idNode->text << ";\n";
    }
}

void CodeGenerator::emitTopLevelVarDeclInit(Node* decl, ostringstream& out, int indent) {
    string pad = indentStr(indent);
    for (auto& idNode : decl->children) {
        string initExpr = idNode->children.empty() ? "mkNum(0)" : emitExpr(idNode->children[0].get());
        out << pad << idNode->text << " = " << initExpr << ";\n";
    }
}

void CodeGenerator::emitFunction(Node* fn, ostringstream& out) {
    FuncSig& sig = functions[fn->text];
    string cReturn = sig.returnsValue ? "Value" : "void";

    string paramList;
    for (auto& c : fn->children) {
        if (c->kind == NodeKind::Param) {
            if (!paramList.empty()) paramList += ", ";
            paramList += "Value " + c->text;
        }
    }
    if (paramList.empty()) paramList = "void";

    out << cReturn << " " << fn->text << "(" << paramList << ") {\n";
    for (auto& c : fn->children) {
        if (c->kind == NodeKind::Block) emitBlockBody(c.get(), out, 1);
    }
    out << "}\n\n";
}

string CodeGenerator::generate(Node* program) {
    functions.clear();
    globals.str("");
    protos.str("");
    funcDefs.str("");
    mainBody.str("");

    for (auto& child : program->children) {
        if (child->kind == NodeKind::VarDecl) {
            emitGlobalVarDecl(child.get(), globals);
        }
    }

    collectFunctionSignatures(program);

    for (auto& child : program->children) {
        if (child->kind == NodeKind::FunctionDef || child->kind == NodeKind::FunctionDecl) {
            FuncSig& sig = functions[child->text];
            string cReturn = sig.returnsValue ? "Value" : "void";
            string paramList;
            for (int k = 0; k < sig.paramCount; k++) {
                if (k > 0) paramList += ", ";
                paramList += "Value";
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
        if (child->kind == NodeKind::FunctionDef || child->kind == NodeKind::FunctionDecl) {
            continue;
        }
        if (child->kind == NodeKind::VarDecl) {
            emitTopLevelVarDeclInit(child.get(), mainBody, 1);
        } else {
            emitStatement(child.get(), mainBody, 1);
        }
    }
    mainBody << "    return 0;\n}\n";

    ostringstream final_;
    final_ <<
        "/* Auto-generated by the toy-language -> C transpiler. */\n"
        "/* The language is dynamically typed: every value is a tagged     */\n"
        "/* union struct `Value` (see below), passed/returned by value.    */\n"
        "/* Requires a C99-or-later compiler.                              */\n"
        "#include <stdio.h>\n"
        "#include <conio.h>\n"
        "#include <string.h>\n"
        "#include <stdlib.h>\n"
        "#include <math.h>\n\n"
        "typedef enum { VAL_NUM, VAL_STR } ValType;\n"
        "typedef struct {\n"
        "    ValType type;\n"
        "    double num;\n"
        "    char str[" << STR_BUF_SIZE << "];\n"
        "} Value;\n\n"
        "static Value mkNum(double n) {\n"
        "    Value v; v.type = VAL_NUM; v.num = n; v.str[0] = '\\0'; return v;\n"
        "}\n"
        "static Value mkStr(const char* s) {\n"
        "    Value v; v.type = VAL_STR; v.num = 0;\n"
        "    size_t i = 0;\n"
        "    for (; s[i] != '\\0' && i < sizeof(v.str) - 1; i++) v.str[i] = s[i];\n"
        "    v.str[i] = '\\0';\n"
        "    return v;\n"
        "}\n"
        "static int truthy(Value v) {\n"
        "    if (v.type == VAL_STR) return v.str[0] != '\\0';\n"
        "    return v.num != 0.0;\n"
        "}\n"
        "static void val_to_cstr(Value v, char* buf, size_t n) {\n"
        "    if (v.type == VAL_STR) { strncpy(buf, v.str, n - 1); buf[n - 1] = '\\0'; }\n"
        "    else { snprintf(buf, n, \"%g\", v.num); }\n"
        "}\n"
        "static void runtime_type_error(const char* op) {\n"
        "    fprintf(stderr, \"runtime error: invalid operand type(s) for '%s'\\n\", op);\n"
        "    exit(1);\n"
        "}\n"
        "static Value val_add(Value a, Value b) {\n"
        "    if (a.type == VAL_STR || b.type == VAL_STR) {\n"
        "        char sa[" << STR_BUF_SIZE << "], sb[" << STR_BUF_SIZE << "];\n"
        "        val_to_cstr(a, sa, sizeof(sa));\n"
        "        val_to_cstr(b, sb, sizeof(sb));\n"
        "        Value r; r.type = VAL_STR; r.num = 0;\n"
        "        snprintf(r.str, sizeof(r.str), \"%s%s\", sa, sb);\n"
        "        return r;\n"
        "    }\n"
        "    return mkNum(a.num + b.num);\n"
        "}\n"
        "static Value val_sub(Value a, Value b) { if (a.type==VAL_STR||b.type==VAL_STR) runtime_type_error(\"-\"); return mkNum(a.num - b.num); }\n"
        "static Value val_mul(Value a, Value b) { if (a.type==VAL_STR||b.type==VAL_STR) runtime_type_error(\"*\"); return mkNum(a.num * b.num); }\n"
        "static Value val_div(Value a, Value b) { if (a.type==VAL_STR||b.type==VAL_STR) runtime_type_error(\"/\"); return mkNum(a.num / b.num); }\n"
        "static Value val_mod(Value a, Value b) { if (a.type==VAL_STR||b.type==VAL_STR) runtime_type_error(\"%\"); return mkNum(fmod(a.num, b.num)); }\n"
        "static Value val_neg(Value a) { if (a.type == VAL_STR) runtime_type_error(\"unary -\"); return mkNum(-a.num); }\n"
        "static Value val_pos(Value a) { if (a.type == VAL_STR) runtime_type_error(\"unary +\"); return mkNum(+a.num); }\n"
        "static Value val_not(Value a) { return mkNum(truthy(a) ? 0 : 1); }\n"
        "static int val_equals(Value a, Value b) {\n"
        "    if (a.type != b.type) return 0;\n"
        "    if (a.type == VAL_STR) return strcmp(a.str, b.str) == 0;\n"
        "    return a.num == b.num;\n"
        "}\n"
        "static Value val_eq(Value a, Value b) { return mkNum(val_equals(a, b) ? 1 : 0); }\n"
        "static Value val_ne(Value a, Value b) { return mkNum(val_equals(a, b) ? 0 : 1); }\n"
        "static Value val_lt(Value a, Value b) { if (a.type==VAL_STR||b.type==VAL_STR) runtime_type_error(\"<\");  return mkNum(a.num <  b.num); }\n"
        "static Value val_le(Value a, Value b) { if (a.type==VAL_STR||b.type==VAL_STR) runtime_type_error(\"<=\"); return mkNum(a.num <= b.num); }\n"
        "static Value val_gt(Value a, Value b) { if (a.type==VAL_STR||b.type==VAL_STR) runtime_type_error(\">\");  return mkNum(a.num >  b.num); }\n"
        "static Value val_ge(Value a, Value b) { if (a.type==VAL_STR||b.type==VAL_STR) runtime_type_error(\">=\"); return mkNum(a.num >= b.num); }\n"
        "static Value val_and(Value a, Value b) { return mkNum((truthy(a) && truthy(b)) ? 1 : 0); }\n"
        "static Value val_or(Value a, Value b)  { return mkNum((truthy(a) || truthy(b)) ? 1 : 0); }\n"
        "static void print_value(Value v) {\n"
        "    if (v.type == VAL_STR) printf(\"%s\", v.str); else printf(\"%g\", v.num);\n"
        "}\n"
        "static Value read_value(void) {\n"
        "    char buf[" << STR_BUF_SIZE << "];\n"
        "    if (scanf(\"%" << (STR_BUF_SIZE - 1) << "s\", buf) != 1) buf[0] = '\\0';\n"
        "    char* endptr;\n"
        "    double n = strtod(buf, &endptr);\n"
        "    if (buf[0] != '\\0' && *endptr == '\\0') return mkNum(n);\n"
        "    return mkStr(buf);\n"
        "}\n\n"
        << globals.str() << "\n"
        << protos.str() << "\n"
        << funcDefs.str()
        << mainBody.str();

    return final_.str();
}