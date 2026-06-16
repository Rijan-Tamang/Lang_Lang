#include<iostream>
using namespace std;
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define YELLOW  "\033[33m"
#define BOLD    "\033[1m"

class Msg{
    public:
    void Error(std::string errmsg){
        cerr<<BOLD<<"ERROR : "<<RESET<<RED<<errmsg<<RESET<<endl;
    }
    void Warn(std::string warnmsg){
        cerr<<BOLD<<"WARN : "<<RESET<<YELLOW<<warnmsg<<RESET<<endl;
    }
};
int main(){
    Msg err;
    err.Error("Nothing Error");
 	return 1;
    err.Warn("Nothing Warning");
	return 0;
}
