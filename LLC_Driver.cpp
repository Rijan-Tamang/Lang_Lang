#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>  // for exit()

using namespace std;

// ANSI codes
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define YELLOW  "\033[33m"
#define BOLD    "\033[1m"

string file_name_with_extension = ""; //this string stores the file name written in commandline without the extension
string file_name_without_extension = ""; //this string stores the file
string file_extension = "";
string store_content = "";


void check_format(int argc){ /*to check if the command in the terminal is correct eg: "llc program.ll" is correct "llc" and "llc program.ll -o hello"
                        is incorrect */
        if (argc != 2){
                cerr<< BOLD << "ERROR:" << RESET << RED << "COMMAND THIK XAINA" << RESET << endl;
                //msg.Error("COMMAND THIK XAINA");
                exit(1);
        }
}


void check_dots(){
        int count_dots = 0;
        for (int i = 0; i < file_name_with_extension.length(); i++){
                if (file_name_with_extension[i] == '.')
                        count_dots += 1;
        }
//More than 1 dots in file name shows errors
        if (count_dots != 1){
            cerr << BOLD << "ERROR: "  << RESET << RED << "FILE EXTENSION MILENA HAI!!!!!" << RESET<< endl;
            clog << BOLD << "WARNING:" << RESET << YELLOW << "FILE EXTENSION EG: namaste.ll HUNXA" << RESET << endl;
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
void extract_file_extension(){
        for (int i = file_name_with_extension.length()-1; i >= 0; i--){
                if (file_name_with_extension[i] != '.'){
                        file_extension += file_name_with_extension[i];
                }
                else
                        break;
        }
	if (file_extension != "ll")
		cerr << BOLD << "ERROR: "  << RESET << RED << "FILE EXTENSION MILENA HAI!!!!!" << RESET<< endl;
                clog << BOLD << "WARNING:" << RESET << YELLOW << "FILE EXTENSION EG: namaste.ll HUNXA" << RESET << endl;
//More than 1 dots in file name shows errors
}

void read_file() {
        cout << "\n" << file_name_without_extension;
        fstream file( file_name_without_extension + ".ll", ios::in);

        if (!file.is_open()) {
                cerr << "\nError: Could not open file " << file_name_with_extension << ".ll" << endl;
                exit(1);
        }

        string line;
        while(getline(file, line)) {
                store_content += line + "\n";
        }
        cout << "\n Store_content " << store_content;
}


int main(int argc, char* argv[]){

        check_format(argc);
	file_name_with_extension = argv[1];
        extract_filename();
        check_dots();
        extract_file_extension();
        if (file_extension != "ll")
                return 0;
        //cout << "\t" << file_name_without_extension << endl;
        read_file();
        cout << store_content << endl;
        return 0;
}
