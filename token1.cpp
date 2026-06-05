#include "token1.h"

std::string tokenTypeToString(TokenType t) {
    switch (t) {
        case INT_NO:return "INT_NO";
        case FLOAT_NO: return "FLOAT_NO";
        case STRING: return "STRING";
        case IDENTIFIER: return "IDENTIFIER";
        case KEYWORD: return "KEYWORD";
        case PLUS_OP: return "plus_op";
        case MINUS_OP: return "MINUS_OP";
        case DIVISION_OP: return "DIVISION_OP";
        case MULTIPLICATION_OP: return "MULTIPLICATION_OP";
        case MODULUS_OP: return "MODULUS_OP";
        case ASSIGNMENT_OP: return "ASSIGNMENT_OP";
        case ADD_ASSIGNMENT_OP: return "ADD_ASSINGMETN_OP";
        case SUB_ASSIGNMENT_OP: return "SUB_ASSINGMENT_OP";
        case DIV_ASSIGNMENT_OP: return "DIV_ASSINGMENT_OP";
        case MUL_ASSIGNMENT_OP: return "MUL_ASSINGNMENT_OP";
        case MOD_ASSIGNMENT_OP: return "MOD_ASSINGNMENT_OP";
        case LEFT_PAREN: return "LEFT_PAREN";
        case RIGHT_PAREN: return "rIGHT_pAREN";
        case LEFT_BRECE: return "LEFT_BRECE";
        case RIGHT_BRECE: return "RIGHT_BRECE";
        case LEFT_BRACKET: return "LEFT_BRCKET";
        case RIGHT_BRACKET: return "RIGHT_BRACKET";
        case COMMA: return "COMMA";
        case SEMICOLAN: return "SEMICOLAN";
        

        case ERROR: return "ERROR";
        case END: return "END";
        default: return "UNKNOWN";
    }
}
