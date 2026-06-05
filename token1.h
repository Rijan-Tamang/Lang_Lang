#pragma once
#include <string>

enum TokenType {
    INT_NO,
    FLOAT_NO,
    STRING,
    IDENTIFIER,
    KEYWORD,
    
    PLUS_OP,
    MINUS_OP,
    DIVISION_OP,
    MULTIPLICATION_OP,
    MODULUS_OP,

    ASSIGNMENT_OP,
    ADD_ASSIGNMENT_OP,
    SUB_ASSIGNMENT_OP,
    DIV_ASSIGNMENT_OP,
    MUL_ASSIGNMENT_OP,
    MOD_ASSIGNMENT_OP,

    LEFT_PAREN,
    RIGHT_PAREN,
    LEFT_BRECE,
    RIGHT_BRECE,
    LEFT_BRACKET,
    RIGHT_BRACKET,
    COMMA,
    SEMICOLAN,

    ERROR,
    END
};

struct Token {
    TokenType type;
    std::string value;
    int line;
    int column;
    std::string errorMsg;
};

std::string tokenTypeToString(TokenType t);