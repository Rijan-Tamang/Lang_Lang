#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include "color.h"

using namespace std;

class message{
    public:
    void Error(std::string errmsg){
        cerr<<BOLD<<RED<<"ERROR : "<<RESET<<errmsg<<RESET<<endl;
    }
    
    void Warn(std::string warnmsg){
        cerr<<BOLD<<YELLOW<<"WARNING : "<<RESET<<warnmsg<<RESET<<endl;
    }
};

message msg;
