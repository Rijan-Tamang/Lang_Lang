#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cstdio>
#include <algorithm>
#include <cctype>
#include "llc.h"
#include "lexer.h"
#include "token1.h"
#include "parser.h"
#include "semantic.h"
#include "codegen.h"

using namespace std;

static void cleanStaleArtifacts(const string& cPath, const string& exeBase) {
    remove(cPath.c_str());
    remove(exeBase.c_str());
    remove((exeBase + ".exe").c_str());
    // remove("gcc_errors.txt");
}

static vector<string> splitLines(const string& src) {
    vector<string> lines;
    string cur;
    for (char c : src) {
        if (c == '\n') {
            if (!cur.empty() && cur.back() == '\r') cur.pop_back();
            lines.push_back(cur);
            cur.clear();
        } else {
            cur += c;
        }
    }
    if (!cur.empty() || lines.empty()) lines.push_back(cur);
    return lines;
}

// gcc-style diagnostic:
//   namaste.ll:3:1: error: expected ';' before 'bhan'
//       3 | bhan("You entered: ", x)
//         | ^
static void printDiagnostic(const string& sourceName, int line, int col, const string& severity,
                             const string& msg, const vector<string>& sourceLines) {
    if (!sourceLines.empty()) {
        if (line < 1) line = 1;
        if ((size_t)line > sourceLines.size()) line = (int)sourceLines.size();
        if (col < 1) col = (int)sourceLines[line - 1].size() + 1;
    }

    cout << sourceName << ":" << line << ":" << col << ": " << severity << ": " << msg << "\n";

    if (line >= 1 && (size_t)line <= sourceLines.size()) {
        string lineNumStr = to_string(line);
        string gutter(lineNumStr.size(), ' ');
        cout << " " << lineNumStr << " | " << sourceLines[line - 1] << "\n";
        int caretPos = max(1, col) - 1;
        cout << " " << gutter << " | " << string(caretPos, ' ') << "^\n";
    }
}

int main(int argc, char* argv[]) {
    // ---------- 1. CLI validation + file loading ----------
    SourceFile src = loadSourceFile(argc, argv);

    const vector<string> sourceLines = splitLines(src.content);
    const string outCPath = src.nameWithoutExtension + ".c";
    const string outExeBase = src.nameWithoutExtension ;
     {
        string lower = outExeBase;
        for (char& c : lower) c = (char)tolower((unsigned char)c);
        if (lower == "llc") {
            cerr << "error: source filename '" << src.nameWithExtension
                 << "' would produce an output executable named 'llc', which "
                 << "collides with the compiler itself. Please rename the file.\n";
            return 1;
        }
    }

    cleanStaleArtifacts(outCPath, outExeBase);

    // ---------- 2. Lexical analysis ----------
    vector<Token> tokens;
    try {
        tokens = Lexer::fromSource(src.content).scan();
    } catch (const exception& e) {
        cerr << src.nameWithExtension << ": fatal error: " << e.what() << "\n";
        cerr << "compilation terminated.\n";
        return 1;
    }

    bool lexError = false;
    for (const Token& t : tokens) {
        if (t.type == ERROR) {
            string m = t.errorMsg.empty() ? ("unexpected token '" + t.value + "'") : t.errorMsg;
            printDiagnostic(src.nameWithExtension, t.line, t.column, "error", m, sourceLines);
            lexError = true;
        }
    }
    if (lexError) {
        cout << "compilation terminated due to lexical errors.\n";
        return 1;
    }

    // ---------- 3. Syntax analysis ----------
    Parser parser(tokens);
    NodePtr ast = parser.parseProgram();

    if (parser.hasErrors()) {
        for (auto& e : parser.getErrors()) {
            printDiagnostic(src.nameWithExtension, e.line, e.column, "error", e.message, sourceLines);
        }
        cout << parser.getErrors().size() << " syntax error(s) generated.\n";
        cout << "compilation terminated.\n";
        return 1;
    }

    // ---------- 4. Semantic analysis ----------
    SemanticAnalyzer sema;
    auto diagnostics = sema.analyze(ast.get());

    int errorCount = 0, warningCount = 0;
    for (auto& d : diagnostics) {
        printDiagnostic(src.nameWithExtension, d.line, d.column,
                         d.severity == DiagSeverity::Error ? "error" : "warning", d.message, sourceLines);
        if (d.severity == DiagSeverity::Error) errorCount++;
        else warningCount++;
    }

    if (errorCount > 0) {
        cout << errorCount << " error(s)";
        if (warningCount > 0) cout << ", " << warningCount << " warning(s)";
        cout << " generated.\n";
        return 1;
    }
    if (warningCount > 0) cout << warningCount << " warning(s) generated.\n";

    // ---------- 5. Code generation (transpile to C) ----------
    string cSource;
    try {
        CodeGenerator gen;
        cSource = gen.generate(ast.get());
    } catch (const exception& e) {
        cout << src.nameWithExtension << ": error: code generation failed: " << e.what() << "\n";
        return 1;
    }

    // ofstream outFile(outCPath);
    // if (!outFile) {
    //     cerr << "error: could not open " << outCPath << " for writing\n";
    //     return 1;
    // }
    // outFile << cSource;
    // outFile.close();

    // ---------- 6. Compile the generated C with gcc ----------
    cout.flush();
    ostringstream gccCmd;
#ifdef _WIN32
    gccCmd << "gcc -std=c99 -x c - -o " << outExeBase << ".exe -lm ";
    // gccCmd << "gcc -std=c99 -x c - -o " << outExeBase << ".exe -lm 2> gcc_errors.txt";
    // gccCmd << "gcc -std=c99 -o " << outExeBase << ".exe " << outCPath << " -lm 2> gcc_errors.txt";
#else
    gccCmd << "gcc -std=c99 -x c - -o " << outExeBase <<" -lm "
    // gccCmd << "gcc -std=c99 -x c - -o " << outExeBase << " -lm 2> gcc_errors.txt";
    // gccCmd << "gcc -std=c99 -o " << outExeBase << " " << outCPath << " -lm 2> gcc_errors.txt";
#endif

#ifdef _WIN32
    FILE* gccPipe = _popen(gccCmd.str().c_str(), "w");
#else
    FILE* gccPipe = popen(gccCmd.str().c_str(), "w");
#endif
   if (!gccPipe) {
        cerr << "error: could not start gcc\n";
        return 1;
    }
    fwrite(cSource.data(), 1, cSource.size(), gccPipe);
#ifdef _WIN32
    int compileResult = _pclose(gccPipe);
#else
    int compileResult = pclose(gccPipe);
#endif
    // int compileResult = system(gccCmd.str().c_str());
    if (compileResult != 0) {
        cout << "Compilation failed.\n";
        return 1;
    }

    // ---------- 7. Run the compiled program ----------
    cout << "----- program output -----\n";
    cout.flush();
    ostringstream runCmd;
#ifdef _WIN32
    runCmd << outExeBase << ".exe";
#else
    runCmd << "./" << outExeBase;
#endif
    int runResult = system(runCmd.str().c_str());
    cout << "---------------------------\n";
    if (runResult != 0) cout << "(program exited with a non-zero status)\n";

    return 0;
}