#include "function.h"

using namespace std;

enum TokenType{
    KEYWORD,//0
    IDENTIFIER,//1
    NUMBER,//2
    EQUAL,//3
    LEFTPAREN,//4
    RIGHTPAREN,//5
    LEFTBRACE,//6
    RIGHTBRACE,//7
    STRING_LITERAL,//8
    OPERATOR,//9
    COMMENT,//10
    SEMICOLON,//11
    UNDEFINED//12
};
struct Token{
    TokenType type;
    string value;
};
vector<Token> tokens;
void Output(){
        for(auto token:tokens){
        cout<< token.type << ": " << token.value << endl;
    }
}
int main() {

    string program = "int main() { rijan1 = 45; }";
    vector<string> src = splitString(program);

    while (!src.empty()) {
        string token = src.front();
        src.erase(src.begin());
        if(token == "int"||token == "main()"){
            tokens.push_back({TokenType::KEYWORD, token});
        } else if(token == "="){
            tokens.push_back({TokenType::EQUAL, token});
        } else if(token == "+" || token == "-" || token == "*" || token == "/"){
            tokens.push_back({TokenType::OPERATOR, token});
        } else if(token == "("){
            tokens.push_back({TokenType::LEFTPAREN, token});
        } else if(token == ")"){
            tokens.push_back({TokenType::RIGHTPAREN, token});
        } else if(token == "{"){
            tokens.push_back({TokenType::LEFTBRACE, token});
        } else if(token == "}"){
            tokens.push_back({TokenType::RIGHTBRACE, token});
        } else if(token == ";"){
            tokens.push_back({TokenType::SEMICOLON, token});
        } else if(isNumber(token)){
            tokens.push_back({TokenType::NUMBER, token});
        } else if(isIden(token)){
            tokens.push_back({TokenType::IDENTIFIER, token});
        }else{
            tokens.push_back({TokenType::UNDEFINED, token});
        }
    }
    
    // Output tokens
        Output();
    // Testing Code Here

    return 0;
}