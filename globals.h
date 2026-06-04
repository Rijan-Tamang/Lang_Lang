#ifndef function_h
#define function_h

#include<iostream>
#include<sstream>
#include<cctype>
#include<vector>
#include<string>


#define RESET   "\033[0m"
#define RED     "\033[31m"
#define YELLOW  "\033[33m"
#define BOLD    "\033[1m"


using namespace std;
string store_content;


class message{
    public:
    void Error(std::string errmsg){
        cerr<<BOLD<<"ERROR : "<<RESET<<RED<<errmsg<<RESET<<endl;
    }
    void Warn(std::string warnmsg){
        cerr<<BOLD<<"WARNING : "<<RESET<<YELLOW<<warnmsg<<RESET<<endl;
    }
};


bool isNumber(string& token);


bool isIden(string& token);


vector<string> splitString(string& str);


void main_read_from_file();


#endif /* function_h */

