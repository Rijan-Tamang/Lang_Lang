#pragma once
#include <string>

// Prints colored ERROR/WARNING messages to stderr.
class Message {
public:
    void Error(const std::string& errmsg) const;
    void Warn(const std::string& warnmsg) const;
};

// Holds the parsed pieces of the filename given on the command line.
struct SourceFile {
    std::string nameWithExtension;   // "namaste.ll"
    std::string nameWithoutExtension; // "namaste"
    std::string extension;           // "ll"
    std::string content;             // full file contents
};

// Validates argc/argv, the filename's extension, opens and reads the file.
// Prints an Error/Warn pair and calls exit(1) on any failure - never returns
// an invalid SourceFile.
SourceFile loadSourceFile(int argc, char* argv[]);