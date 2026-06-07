#include "function.h"

using namespace std;


void flushWord(string& word, vector<string>& result){
    if(!word.empty())
    {
        result.push_back(word);
        word.clear();
    }
}

    bool isNumber(string& token){
        if(token.empty()) return false;
        for(char c : token){
            if(!isdigit(c)) return false;
        }
        return true;
    }
    
    bool isIden(string& token){
        if(token.empty()) return false;
        // identifier cannot start with digit
        if(isdigit(token[0])) return false;
        // identifier cannot contain any special characters 
        for(char ch:token){
            if(!isalnum(ch) && ch != '_') return false;
        }
        //identifiers must start with a letter or underscore
        if(!isalpha(token[0]) && token[0] != '_') return false;
        
        
        return true;
    }

    // vector<string> splitString(string& str){
    //     vector<string> result;
    //     string word;

    //     for(char ch: str){
    //         if(isalnum(ch) || ch == '_'){
    //             word += ch;
    //         } 
    //         // else if(!word.empty()){
    //         //     flushWord(word, result);
    //         // }
    //         else{

    //             flushWord(word, result);
                
    //             // while(ch=='"'){
    //             //     word += ch;

    //             //     if(ch == '"'){
                        
    //             //     }
    //             // }
    //             if(!isspace(ch) && ch != '"'){
    //                 result.push_back(string(1, ch));
    //             }
    //         }
    //     }   
    //     flushWord(word, result);
    //     return result;
    // }

    vector<string> splitString(string& str){
        vector<string> result;
        string word;

        for(int i = 0; i < str.size(); i++){

            char ch = str[i];

            if(isalnum(ch) || ch == '_'){
                word += ch;
            }

            else if(ch == '"'){

                flushWord(word, result);

                string literal;
                literal += ch; // opening quote

                i++;

                while(i < str.size() && str[i] != '"'){
                    literal += str[i];
                    i++;
                }

                if(i < str.size()){
                    literal += '"'; // closing quote
                }

                result.push_back(literal);
            }

            else{
                flushWord(word, result);
                if(!isspace(ch))
                    
                    result.push_back(string(1, ch));
            }
        }
        flushWord(word, result);
        return result;
    }

    bool isStringLiteral(string& token){
        return token.size() >= 2 && token.front() == '"' && token.back() == '"';
    }