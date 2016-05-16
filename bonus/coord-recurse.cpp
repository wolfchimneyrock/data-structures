////////////////////////////////////////////////////////////////////////////////////
// coord-recurse.cpp
// Brooklyn College CISC3130 M. Lowenthal  - Bonus Assignment
// Robert Wagner
// 2016-03-08
// recursive version of the bonus assignment
// builds a global variable finalString and recurses through the possible permutations
// of the dimension sizes, and only adds coordinates if it hasn't been added already
//
// to compile: g++ coordinates.cpp -o coordinates
//     to run: ./coordinates <# of dims> <size1> <size2> ...
//         eg: ./coordinates 3 4 5 6
////////////////////////////////////////////////////////////////////////////////////
#include <iostream>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <sstream>
using namespace std;

string finalString = "";

int ind = 0;
int maxim;

void recurse(int N, int values[]) {
    if (ind > N) return;
    ostringstream SS;
    SS << "(";
    for (int i = 0; i < N; i++) {
        SS << values[i];
        if (i < N - 1) SS << ", ";
        if (i == ind) {
            if (values[ind] > 1) {
                values[ind] -= 1;
                recurse(N, values);
                //values[ind] += 1;
            } else { 
                values[ind] = maxim; 
                ind++; 
                maxim = values[ind];
                values[ind] -= 1;
                recurse(N, values);
            }
        }
    }        
    SS << "), ";
    const string& tmp = SS.str();
    const char* s = tmp.c_str();
    
    // only add it if its not already added
    if  (strstr(finalString.c_str(), s) == NULL) {
         finalString.append(s);
         cout << "x";
    } else cout << ".";
}

int main(int argc, char* argv[]) {
    if (argc < 3) { cout << "must pass at least two integers." << endl; return -1; }
    int vals[argc-2];
    // first, intialize what we know we can initialize
    int dim = atoi(argv[1]);
    if (argc - 2 != dim) { cout << "invalid number of parameters." << endl; return -1; }

    // this loop only acts to fill the input array with the passed parameter values
    for (int i = 0; i < dim; i++) vals[i] = atoi(argv[i + 2]);

    maxim = vals[0];
    // do the work
    recurse(dim, vals);

    // now global string finalString contains the final output with a trailing ',' to remove
    finalString = finalString.substr(0, finalString.length() - 2);
    cout << finalString << endl;
}
