///////////////////////////////////////////////////////////////////////////////////
// tree.hpp
// Brooklyn College CISC3130 M. Lowenthal  - Assignment #4
// Robert Wagner
// 2016-03-25
//
// this the driver program for the first tree assignment.
// here I implement a printing functor to print the tree contents,
// and also I have made a rudimentary token parsing system for the input
//
///////////////////////////////////////////////////////////////////////////////////
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <stdlib.h>
#include <map>
#include <boost/format.hpp>
#include "DynamicBinaryTree.hpp"

typedef DynamicBinaryTree::Node<int> Node;
typedef DynamicBinaryTree::DynamicBinaryTree<int> Tree;
 
using boost::format;
using std::ostringstream;
using std::endl;
using std::string;
using DynamicBinaryTree::Traversals;
std::ostream& output = std::cout;

// I could put all this token parsing stuff into its own class, but I don't
// think this is the point of the assignment, and its not too heavy anyway
enum class Tokens : int {
    NOTHING    = 0,
    NEWSET     = 1,
    INSERT     = 2,
    DELETE     = 4,
    INTEGER    = 8,
    EOL        = 16,
    OPERATIONS = NEWSET | INSERT | DELETE,
    OPERANDS   = INTEGER | EOL
};

inline constexpr Tokens operator&(Tokens a, Tokens b) { 
    return static_cast<Tokens>(static_cast<int>(a) & static_cast<int>(b));
}

std::map<string, Tokens> TOKENKEY = {{"set",    Tokens::NEWSET }, 
                                     {"insert", Tokens::INSERT }, 
                                     {"delete", Tokens::DELETE },
                                     {"end",    Tokens::EOL    }};
// boost::format strings for output
format fmtInvalidToken ("[%4i] Invalid token '%s', skipping.\n");
format fmtExpectedInt  ("[%4i] Expected an int but got '%s' instead.\n");
format fmtUnexpected   ("[%4i] Unexpected token '%s', skipping.\n");
format fmtParseError   ("[%4i] Parse error.\n");
format fmtInitial      ("(Set #%i) Initial state:\n");
format fmtModified     ("(Set #%i) After %i operations:\n");
format fmtInOrder      ("(Set #%i) InOrder:   %s\n");
format fmtPreOrder     ("(Set #%i) PreOrder:  %s\n");
format fmtPostOrder    ("(Set #%i) PostOrder: %s\n");
format fmtCount        ("(Set #%i) Tree Size: %i\n");
format fmtChildren     ("(Set #%i) Preorder w/ Children:  %s\n");
format fmtNoOperations ("(Set #%i) No further operations performed.\n\n");

// this is a functor that creates a string of the traversal of the tree.
// using enableChildCount() and disableChildCount() it can print either with
// or without the # of children for each node in []
struct PrintFunctor {
    private:
        string separator, prefix, suffix;
        string results;
        bool started;
        bool countChildren;
    public:
        PrintFunctor(const string& s, const string& pre, const string& suf) :
            separator(s), prefix(pre), suffix(suf)
            { started = false; countChildren = false; results = prefix; }

          // default init = typical set notation
        PrintFunctor() :
            PrintFunctor(", ", "{", "}") {}
        
          // empty the functor for re-use
        void reset() {
            started = false;
            results = prefix;
            countChildren = false;
        }
        void enableChildCount()  { countChildren = true; }
        void disableChildCount() { countChildren = false; }

          // add an item to the functor
        void operator()(Node *node) {
            if (!started) {
                results += std::to_string(node->getData());
                started = true;
            } else { 
                results += separator;
                results += std::to_string(node->getData());
            }
            if (this->countChildren == true) {
                int count = 0;
                if (node->getLeft())  count++;
                if (node->getRight()) count++;
                results += '[';
                results += std::to_string(count);
                results += ']';
            }

        }
          // return the output of the current state of the functor
        string result() const { return results + suffix; }
}; // PrintSetFunctor

void printTree(Tree *tree, int current) {
    PrintFunctor Print;

    tree->traverse(Traversals::INORDER, Print);
    output << fmtInOrder % current % Print.result();
    Print.reset();

    tree->traverse(Traversals::PREORDER, Print);
    output << fmtPreOrder % current % Print.result();
    Print.reset();

    tree->traverse(Traversals::POSTORDER, Print);
    output << fmtPostOrder % current % Print.result();
    Print.reset();

    output << fmtCount % current % tree->count();

    Print.enableChildCount();
    tree->traverse(Traversals::PREORDER, Print);
    output << fmtChildren % current % Print.result();
    output << endl;
}

int main(int argc, char *argv[]) {
    Tree tree; 
    
    // open a file for input or use standard input if no good file
    std::ifstream inFile;
    std::istream* inFileP = &std::cin;
    if (argc > 1) {
        inFile.open(argv[1]);
        if (inFile.good()) inFileP = &inFile;
        else inFile.close();
    }

    string    line = "";       // an individual line from input file
    string   token = "";       // an individual token from line
    int      value = 0;        // the most recently read integer
    int operations = 0;        // # of operations encountered
    int     lineNo = 0;        // input file current line #
    int    current = 0;        // the current set in process

      // the first thing we expect to encounter is a NewSet token
    Tokens expected = Tokens::NEWSET;
    Tokens found, operation = Tokens::NOTHING;
      // loop through lines of the input
    while (std::getline(*inFileP, line)) {
       ++lineNo;
         // skip empty lines
       if (line.size() == 0) continue;
         // skip comments
       if (line.front() == '#') continue;
       std::stringstream ss(line);
         // loop through tokens in this line
       while (ss >> token) {
           found = Tokens::NOTHING;
           auto t = TOKENKEY.find(token);
           if (t == TOKENKEY.end()) {
                 // there are two cases where token isn't found:
                 // 1. an expected integer
                 // 2. an actual invalid token
               if ((expected & Tokens::INTEGER) == Tokens::INTEGER) {
                   found = Tokens::INTEGER;
                   try { value = stoi(token); }
                   catch (std::exception &e) {
                       output << fmtExpectedInt % lineNo % token;
                       found = Tokens::NOTHING;
                   }
               } else output << fmtInvalidToken % lineNo % token;
           } else found = t->second;
             // was it missing? skip it
           if (found == Tokens::NOTHING) continue;
             // was it unexpected? skip it
           if ((expected & found) != found) {
               output << fmtUnexpected % lineNo % token;
               continue;
           }
           switch (found) {
           case Tokens::NEWSET:  // start a new set.  
                                 // if there was a prior set modified,
                                 // print it.
               if ((current > 0) && (operations > 0)) { 
                   output << fmtModified % current % operations;
                   printTree(&tree, current);
               } else if (current > 0) output << fmtNoOperations % current;
               ++current;
               operations = 0;
               operation = Tokens::NEWSET;
               expected  = Tokens::OPERANDS;
               value = 0;
               tree.clear();
               break;
           case Tokens::INSERT:  // run the insert operation
               operation = Tokens::INSERT;
               expected  = Tokens::OPERANDS;
               operations++;
               value = 0;
               break;
           case Tokens::DELETE:  // run the delete operation
               operation = Tokens::DELETE;
               expected  = Tokens::OPERANDS;
               operations++;
               value = 0;
               break;
           case Tokens::EOL:  // end the set, print the initial contents
               operation = Tokens::NOTHING;
               expected  = Tokens::OPERATIONS;
               value = 0;
               output << fmtInitial % current;
               printTree(&tree, current);
               break;
           case Tokens::INTEGER:  // what we do with integers depends
               switch (operation) {
                   case Tokens::NEWSET: tree.insert(value); 
                                        expected = Tokens::OPERANDS;
                                break;
                   case Tokens::INSERT: tree.insert(value); 
                                        expected = Tokens::OPERATIONS;
                                        operation = Tokens::NOTHING;
                                break;
                   case Tokens::DELETE: tree.remove(value); 
                                        expected = Tokens::OPERATIONS;
                                        operation = Tokens::NOTHING;
                                break;
                   default:
                       output << fmtParseError % lineNo;
                       break;
               }
               break;
           default: 
               output << fmtParseError % lineNo;
               break;
           }
       }
        // if there were operations performed on the last set, print results
        if (operations > 0) {
            output << fmtModified % current % operations;
            printTree(&tree, current);
            operations = 0;
        } 
    }
    if (current > 0) output << fmtNoOperations % current;
    if (inFile) inFile.close();
}
