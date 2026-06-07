#ifndef function_h
#define function_h

#include<iostream>
#include<sstream>
#include<cctype>
#include<vector>
#include<string>
#include<algorithm>

using namespace std;

extern string store_content;

bool isNumber(string& token);
bool isIden(string& token);
vector<string> splitString(string& str);
bool isStringLiteral(string& token);
void driver_main(int argc, char* argv[]);

#endif /* function_h */