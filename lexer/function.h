#ifndef function_h
#define function_h

#include<iostream>
#include<sstream>
#include<cctype>
#include<vector>
#include<string>

using namespace std;

bool isNumber(string& token);
bool isIden(string& token);
vector<string> splitString(string& str);
#endif /* function_h */