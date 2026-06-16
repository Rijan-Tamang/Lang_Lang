#pragma once
#include <string>
#include <vector>
#include "token1.h"
#include <fstream>
#include <sstream>
#include <cctype>
#include <stdexcept>
#include <unordered_set>


class Lexer {
public:
    static Lexer fromFile();
    static Lexer fromSource(const std::string& source);

    std::vector<Token> scan();

private:
    explicit Lexer(std::string source);

    std::string src;
    size_t i = 0;
    int line = 1;
    int col = 0;

    char see(int offset = 0) const;
    char moveonce();

    void skipSpace();
    void skipLineComment();

    void drainAlnum(std::string& out);

    Token readNumber();
    Token readString();
    Token readWord();
    Token readOperator();
};

void main_read_from_file(int argc, char* argv[]);