#include <iostream>
#include "lexer.h"
#include "token1.h"

using namespace std;

int main() {
    try {
        auto tokens = Lexer::fromFile("main.txt").scan();

        for (const Token& t : tokens) {
            cout << tokenTypeToString(t.type)
                 << " : " << t.value
                 << " (L" << t.line
                 << ", C" << t.column << ")\n"
                 <<t.errorMsg<<"\n";
        }

    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
    }

    return 0;
}