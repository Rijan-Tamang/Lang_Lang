#include "function.h"

using namespace std;

    bool isNumber(string& token){
        if(token.empty()) return false;
        for(char c : token){
            if(!isdigit(c)) return false;
        }
        return true;
    }
    
    bool isIden(string& token){
        if(token.empty()) return false;
        if(isdigit(token[0])) return false;
        if(!isalpha(token[0]) && token[0] != '_') return false;
        
        
        return true;
    }

    vector<string> splitString(string& str){
        vector<string> result;
        stringstream ss(str);
        string word;
        while(ss >> word){
            result.push_back(word);
        }
        return result;
    }