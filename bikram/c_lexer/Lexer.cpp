#include "lexer.h"
#include "token1.h"

using namespace std;

extern string store_content;

static unordered_set<string> keywords = {
    "yoho", "yedi", "natra","tesovaye" ,"setgar", "yochai", "yo", "feriferi"/*for loop*/, "jabasamma", "firta", "bhan", "sun", "vayo", "jaujau",  "gar", "vayenavane"
};

Lexer Lexer::fromFile() {
    return Lexer(store_content);
}

Lexer Lexer::fromSource(const string& source) {
    return Lexer(source);
}

Lexer::Lexer(string source) : src(move(source)) {}

vector<Token> Lexer::scan() {
    vector<Token> tokens;

    while (i < src.size()) {
        skipSpace();
        if (i >= src.size()) break;

        char c = see();
        int startLine = line;
        int startCol = col + 1;

        // comment
        if (c == '/' && see(1) == '/') {
            skipLineComment();
            continue;
        }

        // number
        if (isdigit(c)) {
            tokens.push_back(readNumber());
        }

        // string
        else if (c == '"') {
            tokens.push_back(readString());
        }

        // identifier / keyword
        else if (isalpha(c) || c == '_') {
            tokens.push_back(readWord());
        }

        // operator
        else {
            tokens.push_back(readOperator());
        }
    }

    tokens.push_back({END, "EOF", line, col});
    return tokens;
}

char Lexer::see(int offset) const {
    size_t idx = i + offset;
    return idx < src.size() ? src[idx] : '\0';
}

char Lexer::moveonce() {
    if (i >= src.size()) return '\0';

    char c = src[i++];

    if (c == '\n') {
        line++;
        col = 0;
    } else {
        col++;
    }

    return c;
}

void Lexer::skipSpace() {
    while (isspace(see()))
        moveonce();
}

void Lexer::skipLineComment() {
    while (see() != '\n' && see() != '\0')
        moveonce();
}

void Lexer::drainAlnum(string& out) {
    while (isalnum(see()) || see() == '_')
        out += moveonce();
}

Token Lexer::readWord() {
    string word;
    int startLine = line;
    int startCol = col + 1;

    drainAlnum(word);

    if (keywords.count(word))
        return {KEYWORD, word, startLine, startCol};

    return {IDENTIFIER, word, startLine, startCol};
}

Token Lexer::readNumber() {
    string num;
    int startLine = line;
    int startCol = col + 1;
    bool hasDot = false;
    bool hasError = false;
    string errorMsg;

    while (isdigit(see()) || see() == '.' || isalpha(see())) {
        char c = see();

        if (c == '.') {
            if (hasDot) {
                // e.g. 11.11.11 — second dot is malformed
                hasError = true;
                errorMsg = "malformed number: multiple decimal points";
                num += moveonce();
                continue;
            }
            hasDot = true;
        } else if (isalpha(c)) {
            // e.g. 21a — letter suffix after digits
            hasError = true;
            errorMsg = "malformed number: unexpected character '" + string(1, c) + "'";
        }

        num += moveonce();
    }

    if (hasError) {
        return {ERROR, num, startLine, startCol, errorMsg};
    }
    if(hasDot) return {FLOAT_NO, num, startLine, startCol};

    return {INT_NO, num, startLine, startCol};
}

Token Lexer::readString() {
    string str;
    int startLine = line;
    int startCol = col + 1;

    moveonce(); // skip "

    while (see() != '"' && see() != '\0') {
        str += moveonce();
    }

    if (see() == '"') moveonce();
    else return {ERROR, "Unterminated string", startLine, startCol};

    return {STRING, str, startLine, startCol};
}

Token Lexer::readOperator() {
    string op;
    int startLine = line;
    int startCol = col + 1;
    

    op += moveonce();

    char b = see();
        if(op=="+" && b=='='){
            op+=moveonce();
            return {ADD_ASSIGNMENT_OP,op,startLine,startCol};}
        else if(op=="-"&& b=='='){
            op+=moveonce();
            return {SUB_ASSIGNMENT_OP,op,startLine,startCol};}
        else if(op=="="&& b=='='){
            op+=moveonce();
            return {SUB_ASSIGNMENT_OP,op,startLine,startCol};}
        else if(op=="*" && b=='='){
            op+=moveonce();
            return {MUL_ASSIGNMENT_OP, op,startLine, startCol};}

        else if(op=="/" && b=='='){
            op+=moveonce();
            return {DIV_ASSIGNMENT_OP, op, startLine,startCol};}
        else if(op=="%" && b=='='){
        op+=moveonce();
        return {MOD_ASSIGNMENT_OP,op , startLine, startCol};}
        else if(op=="+") return {PLUS_OP,op,startLine,startCol };
        else if(op=="-") return {MINUS_OP,op,startLine,startCol};
        else if(op=="/") return {DIVISION_OP,op,startLine,startCol};
        else if(op=="*") return {MULTIPLICATION_OP,op,startLine,startCol};
        else if(op=="=") return {ASSIGNMENT_OP,op,startLine,startCol};






        else if (op=="(") return {LEFT_PAREN, op,startLine,startCol};
        else if(op==")") return {RIGHT_PAREN, op, startLine, startCol};
        else if (op=="{") return {LEFT_BRECE, op , startLine , startCol};
        else if(op=="}") return {RIGHT_BRECE, op, startLine, startCol};
        else if (op=="[") return {LEFT_BRACKET, op, startLine , startCol};
        else if (op=="]") return { RIGHT_BRACKET , op, startLine, startCol};
        else if (op==",") return {COMMA,op ,startLine, startCol};
        else if (op==";") return {SEMICOLAN,op, startLine, startCol};
        else return {ERROR, op, startLine, startCol};

}