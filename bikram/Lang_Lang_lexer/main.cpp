#include <iostream>
#include "lexer.h"
#include "token1.h"

using namespace std;

int main(int argc, char* argv[]) {
    main_read_from_file(argc, argv);
    try {
        auto tokens = Lexer::fromFile().scan();
        cout << "Tokens:"<< endl;
        for (const Token& t : tokens) {
            cout <<"\""<< t.value <<"\"" << "\t";
        }
        cout <<"\n=================================================================================="<<endl;
        cout <<"Token Details:\n"<< endl;
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