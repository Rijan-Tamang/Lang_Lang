#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
using namespace std;

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define YELLOW  "\033[33m"
#define BOLD    "\033[1m"

int main(){
    // string name = BOLD<<"ERROR : "<<RESET<<RED<<"Hello";
    string name = string(BOLD) + "ERROR : " + RESET + RED + "Hello" + RESET;
    cerr<<BOLD<<"ERROR : "<<RESET<<RED<<"ho rw"<<RESET<<endl;
    cout<<name;
    
    return 0;
}