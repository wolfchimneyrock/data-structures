#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <vector>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/constants.hpp>
#include "definitions.hpp"
#include "parser.hpp"
#include "FamilyTree.hpp"

using namespace FamilyTree;
using std::cout;
using std::string;
using std::endl;
using std::getline;
using std::vector;
using boost::split;

std::ostream& output = std::cout;

const string ws = "\t, ?.:;'-";

int main(int argc, char *argv[]) {
    Family family;
    //NameQueue  parentQ;

    // first, either open the first parameter as a file or stdin if not valid or missing
    bool showPrompt = true;
    std::ifstream  inFile;
    std::istream* inFileP = &std::cin;
    if (argc > 1) {
        inFile.open(argv[1]);
        if (inFile.good()) { inFileP = &inFile; showPrompt = false; }
        else inFile.close();
    }

    string       line = "";
    string     parent = "";
    vector<string> tokens;
    Parser parser;
        //parser.debug = true;
    int         value = 0;
    int        lineNo = 0;
    if (showPrompt) output << fmtPrompt;
    while (getline(*inFileP, line)) {
        ++lineNo;
        if (!showPrompt) output << fmtEcho % lineNo % fmtPrompt % line;
        parser.reset();
        tokens.clear();
        if (line.size()  ==  0 ) { 
            if (showPrompt) output << fmtPrompt;
            continue;  // skip blank lines
        }
        if (line.front() == '#') {
            if (showPrompt) output << fmtPrompt;
            continue;  // skip comments
        }
        split(tokens, line, boost::is_any_of(ws), boost::token_compress_on);
        for (auto token : tokens) parser(token);
        parser.finalize();
        if (parser.debug) output << parser.toString();
        family.resolve(parser);
        family.execute(parser);
        output << parser.message;
        if (showPrompt) output << fmtPrompt;
    }
    if (showPrompt) output << endl;
}
