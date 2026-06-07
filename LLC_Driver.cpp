#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>  // for exit()
#include"./lexer/function.h"

using namespace std;

// ANSI codes
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define YELLOW  "\033[33m"
#define BOLD    "\033[1m"

string file_name_with_extension = ""; //this string stores the file name written in commandline without the extension
string file_name_without_extension = ""; //this string stores the file
string file_extension = "";
string store_content="";


class message{
    public:
    void Error(std::string errmsg){
        cerr<<BOLD<<"ERROR : "<<RESET<<RED<<errmsg<<RESET<<endl;
    }
    void Warn(std::string warnmsg){
        cerr<<BOLD<<"WARNING : "<<RESET<<YELLOW<<warnmsg<<RESET<<endl;
    }
};

void check_format(int argc, message msg){ /*to check if the command in the terminal is correct eg: "llc program.ll" is correct "llc" and "llc program.ll -o hello"
                        is incorrect */
        if (argc != 2){
                //cerr<< BOLD << "ERROR:" << RESET << RED << "COMMAND THIK XAINA" << RESET << endl;
                msg.Error("COMMAND THIK XAINA"); //using oop instead
                exit(1);
        }
}


void check_dots(message msg){
        int count_dots = 0;
        for (int i = 0; i < file_name_with_extension.length(); i++){
                if (file_name_with_extension[i] == '.')
                        count_dots += 1;
                }
                //More than 1 dots in file name shows errors
        if (count_dots != 1){
            //cerr << BOLD << "ERROR: "  << RESET << RED << "FILE EXTENSION MILENA HAI!!!!!" << RESET<< endl;
	    msg.Error("FILE EXTENSION MILENA HAI!!!!");
            //clog << BOLD << "WARNING:" << RESET << YELLOW << "FILE EXTENSION EG: namaste.ll HUNXA" << RESET << endl;
	    msg.Warn("FILE EXTENSION EG: 'namaste.ll' HUNXA");
	    exit(1);
        }


}


void extract_filename(){
        for (int i = 0; i < file_name_with_extension.length(); i++){
                if (file_name_with_extension[i] != '.'){
                        file_name_without_extension += file_name_with_extension[i];
                }
                else
                        break;
        }
}


void extract_file_extension(message msg){

        for (int i = file_name_with_extension.length()-1; i >= 0; i--){
                if (file_name_with_extension[i] != '.'){
                        file_extension += file_name_with_extension[i];
                }
                else
                        break;
        }
	if (file_extension != "ll"){
            //cerr << BOLD << "ERROR: "  << RESET << RED << "FILE EXTENSION MILENA HAI!!!!!" << RESET<< endl;
	    msg.Error("FILE EXTENSION MILENA HAI!!!!");
            //clog << BOLD << "WARNING:" << RESET << YELLOW << "FILE EXTENSION EG: namaste.ll HUNXA" << RESET << endl;
	    msg.Warn("FILE EXTENSION EG: 'namaste.ll' HUNXA");
	    exit(1);
        }
//More than 1 dots in file name shows errors

}





void read_file(message msg) {

        fstream file( file_name_without_extension + ".ll", ios::in);

        if (!file.is_open()) {

                //cerr << "\nError: Could not open file " << file_name_with_extension << ".ll" << endl;
		msg.Error( file_name_with_extension + " FILE KHOLNA SAKINA ");
		msg.Warn( file_name_with_extension + " KO ASTITWA XAINA ");
                exit(1);
        }
	store_content.clear();
        string line;
        while(getline(file, line)) {
                store_content += line + "\n";
        }
}







void driver_main(int argc, char* argv[]){
	message msg;
        check_format(argc, msg);
	file_name_with_extension = argv[1];
        extract_filename();
        check_dots(msg);
        extract_file_extension(msg);
        //cout << "\t" << file_name_without_extension << endl;

        read_file(msg);
 //       return 0;
}