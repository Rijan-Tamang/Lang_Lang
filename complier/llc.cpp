#include "llc.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdlib>

using namespace std;

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define YELLOW  "\033[33m"
#define BOLD    "\033[1m"

void Message::Error(const string& errmsg) const {
    cerr << BOLD << "ERROR : " << RESET << RED << errmsg << RESET << endl;
}

void Message::Warn(const string& warnmsg) const {
    cerr << BOLD << "WARNING : " << RESET << YELLOW << warnmsg << RESET << endl;
}

static void checkFormat(int argc, const Message& msg) {
    if (argc != 2) {
        msg.Error("USAGE: llc <filename>.ll");
        exit(1);
    }
}

static void checkDots(const string& nameWithExtension, const Message& msg) {
    int countDots = 0;
    for (char c : nameWithExtension) {
        if (c == '.') countDots++;
    }
    if (countDots != 1) {
        msg.Error("INVALID FILE NAME");
        msg.Warn("EXPECTED FORMAT EG: 'namaste.ll'");
        exit(1);
    }
}

static string extractNameWithoutExtension(const string& nameWithExtension) {
    string result;
    for (char c : nameWithExtension) {
        if (c == '.') break;
        result += c;
    }
    return result;
}

static string extractExtension(const string& nameWithExtension, const Message& msg) {
    string ext;
    for (int i = (int)nameWithExtension.length() - 1; i >= 0; i--) {
        if (nameWithExtension[i] == '.') break;
        ext += nameWithExtension[i];
    }
    reverse(ext.begin(), ext.end());

    if (ext != "ll") {
        msg.Error("UNSUPPORTED FILE EXTENSION");
        msg.Warn("EXPECTED FORMAT EG: 'namaste.ll'");
        exit(1);
    }
    return ext;
}

static string readFileContent(const string& nameWithoutExtension,
                               const string& nameWithExtension, const Message& msg) {
    ifstream file(nameWithoutExtension + ".ll");
    if (!file.is_open()) {
        msg.Error("COULD NOT OPEN FILE: " + nameWithExtension);
        msg.Warn("FILE DOES NOT EXIST: " + nameWithExtension);
        exit(1);
    }
    stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

SourceFile loadSourceFile(int argc, char* argv[]) {
    Message msg;
    checkFormat(argc, msg);

    SourceFile sf;
    sf.nameWithExtension = argv[1];
    checkDots(sf.nameWithExtension, msg);
    sf.nameWithoutExtension = extractNameWithoutExtension(sf.nameWithExtension);
    sf.extension = extractExtension(sf.nameWithExtension, msg);
    sf.content = readFileContent(sf.nameWithoutExtension, sf.nameWithExtension, msg);

    return sf;
}