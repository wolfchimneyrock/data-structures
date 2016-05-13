#include <iostream>
#include <string>
#include "definitions.hpp"
#include "parser.hpp"

using namespace FamilyTree;
using std::cout;
using std::endl;
int main(int argc, char *argv[]) {
    Parser p;
    p.debug = true;
    for (int i = 0; i < argc; i++) {
        std::string s(argv[i]);
        cout << s << ": ";
        p(s);
        cout << endl;
    }
    cout << "OUTCOME: " << endl;
    cout << p.toString();
}
