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

string file_name_with_extension = "";
string file_name_without_extension = "";
string file_extension = "";
string store_content = "";

class Msg{
    public:
    void Error(std::string errmsg){
        cerr<<BOLD<<"ERROR : "<<RESET<<RED<<errmsg<<RESET<<endl;
    }
    void Warn(std::string warnmsg){
        cerr<<BOLD<<"WARN : "<<RESET<<YELLOW<<warnmsg<<RESET<<endl;
    }
};


void check_format(int argc) {
    if (argc != 2) {
        cerr << BOLD << "ERROR:" << RESET << RED << " COMMAND THIK XAINA" << RESET << endl;

	cerr << "Usage: " << BOLD << "llc <filename.ll>" << RESET << endl;
        exit(1);  // Stop the program immediately
    }
}

void check_dots() {
    int count_dots = 0;
    for (char c : file_name_with_extension) {
        if (c == '.') count_dots++;
    }
    if (count_dots != 1) {
        cerr << BOLD << "ERROR: " << RESET << RED << "FILE EXTENSION MILENA HAI!!!!!" << RESET << endl;
        clog << BOLD << "WARNING:" << RESET << YELLOW << "FILE EXTENSION EG: namaste.ll HUNXA" << RESET << endl;
        exit(1);
    }
}

void extract_filename() {
    for (char c : file_name_with_extension) {
        if (c != '.')
            file_name_without_extension += c;
        else
            break;
    }
}

void extract_file_extension(Msg err) {
    unsigned int dot_pos = file_name_with_extension.rfind('.');
    if (dot_pos != string::npos) {
        file_extension = file_name_with_extension.substr(dot_pos + 1);
    } else {
        err.Error("EXTENSION NAI XAINA");
    }
}

void read_file() {
    string filename = file_name_without_extension + ".ll";
    fstream file(filename, ios::in);
    if (!file.is_open()) {
        cerr << BOLD << "ERROR:" << RESET << RED << " Could not open file " << filename << RESET << endl;
        exit(1);
    }
    string line;
    while (getline(file, line)) {
        store_content += line;
        store_content += '\n';  // preserve newlines
    }
    cout << "\nStore_content:\n" << store_content << endl;
}

int main(int argc, char* argv[]) {
    Msg err;
  // First, validate number of arguments
    check_format(argc);
    file_name_with_extension = argv[1];
    check_dots();
    // Now it's safe to use argv[1]
    extract_filename();
    extract_file_extension(err);

    if (file_extension != "ll") {
        cerr << BOLD << "ERROR:" << RESET << RED << " File extension must be .ll" << RESET << endl;
        return 1;
    }

    read_file();
    return 0;
}
