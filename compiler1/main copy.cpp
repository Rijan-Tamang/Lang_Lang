#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cstdio>
#include <algorithm>
#include <cctype>
#include "lexer.h"
#include "token1.h"
#include "parser.h"
#include "semantic.h"
#include "codegen.h"
#include "color.h"
 
using namespace std;
extern string file_name_with_extension;
extern string file_name_without_extension;
extern string store_content;

static void cleanStaleArtifacts(const string& cPath, const string& exeBase) {
    remove(cPath.c_str());
    remove(exeBase.c_str());
    //remove((exeBase + ".exe").c_str());
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
static void printDiagnostic(const string& sourceName, int line, int col, const string& severity, const string& msg, const vector<string>& sourceLines) {
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
    main_read_from_file(argc, argv);
    const vector<string> sourceLines = splitLines(store_content);
    const string outCPath = file_name_without_extension + ".c";
    const string outExeBase = file_name_without_extension;

    cleanStaleArtifacts(outCPath, outExeBase);

    // ---------- 2. Lexical analysis ----------
    vector<Token> tokens;
    try {
        tokens = Lexer::fromSource(store_content).scan();
    } catch (const exception& e) {
        cerr << file_name_with_extension << ": Gambhir Samasya : " << e.what() << "\n";
        cerr << "Compilation Rokkiyo\n";
        return 1;
    }

    bool lexError = false;
    for (const Token& t : tokens) {
        if (t.type == ERROR) {
            string m = t.errorMsg.empty() ? ("Token Milena '" + t.value + "'") : t.errorMsg;
            printDiagnostic(file_name_with_extension, t.line, t.column, string(BOLD)+RED+"SAMASYA"+RESET, m, sourceLines);
            lexError = true;
        }
    }
    if (lexError) {
        cout << "Lexical Error KO KARAN COMPILATION ROKKIYO\n";
        return 1;
    }

    // ---------- 3. Syntax analysis ----------
    Parser parser(tokens);
    NodePtr ast = parser.parseProgram();

    if (parser.hasErrors()) {
        for (auto& e : parser.getErrors()) {
            printDiagnostic(file_name_with_extension, e.line, e.column, string(BOLD)+RED+"SAMASYA"+RESET, e.message, sourceLines);
        }
        cout << parser.getErrors().size() << BOLD << " SYNTAX ERROR(s) VETTIYO.\n" << RESET;
        cout << RED << BOLD << "COMPILATION ROKKIYO.\n" << RESET;
        return 1;
    }

    // ---------- 4. Semantic analysis ----------
    SemanticAnalyzer sema;
    auto diagnostics = sema.analyze(ast.get());

    int errorCount = 0, warningCount = 0;
    for (auto& d : diagnostics) {
        printDiagnostic(file_name_with_extension, d.line, d.column, d.severity == DiagSeverity::Error ? string(BOLD)+RED+"SAMASYA"+RESET : string(BOLD)+YELLOW+"KHABARDAR"+RESET, d.message, sourceLines);
        if (d.severity == DiagSeverity::Error) errorCount++;
        else warningCount++;
    }

    if (errorCount > 0) {
        cout << errorCount << " SAMASYA(HARU)";
        if (warningCount > 0) cout << ", " << warningCount << RED<< BOLD <<" KHABARDAR"<< RESET;
        cout << BOLD << " VETIYO\n" << RESET;
        return 1;
    }
    if (warningCount > 0) cout << warningCount << " KHABARDAR VETIYO.\n";

    // ---------- 5. Code generation (transpile to C) ----------
    string cSource;
    try {
        CodeGenerator gen;
        cSource = gen.generate(ast.get());
    } catch (const exception& e) {
        cout << file_name_with_extension << ": SAMASYA: CODE GENERATION PURA VAYENA: " << e.what() << "\n";
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
    gccCmd << "gcc -std=c99 -x c - -o " << outExeBase <<".out -lm "
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
        cout << BOLD<<"COMPILATION PURA VAYENA.\n" << RESET;
        return 1;
    }

    // ---------- 7. Run the compiled program ----------
    cout.flush();
    ostringstream runCmd;
#ifdef _WIN32
    runCmd << outExeBase << ".exe";
#else
    runCmd << "./" << outExeBase;
#endif
    int runResult = system(runCmd.str().c_str());
    if (runResult != 0) cout << "(program exited with a non-zero status)\n";

    return 0;
}