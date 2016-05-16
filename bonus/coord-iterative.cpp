////////////////////////////////////////////////////////////////////////////////////
// coord-iterative.cpp
// Brooklyn College CISC3130 M. Lowenthal  - Bonus Assignment
// Robert Wagner
// 2016-02-25
// 
// iterative verion of the bonus assignment
// takes advantage of the fact that a multivariate function A(a1,a2,a3, ... a_n) is
// bijective with a single dimensional B(b) with b = { 0 ... product(a1,a2,...a_n) }
// using modular aritmatic.  this works as long as the product(a1..a_n) does not
// overflow the variable data type used. 
//
// to compile: g++ coord-iterative.cpp -o coord-iterative
//     to run: ./coord-iterative <# of dims> <size1> <size2> ...
//         eg: ./coord-iterative 3 4 5 6
////////////////////////////////////////////////////////////////////////////////////
#include <iostream>
#include <stdlib.h>
using namespace std;
int main(int argc, char* argv[]) {
    if (argc < 3) { cout << "must pass at least two integers." << endl; return -1; }
    int mods[argc-2];
    int divs[argc-2];
    int max, mod, div, index;
    // first, intialize what we know we can initialize
    int dim = atoi(argv[1]);
    if (argc - 2 != dim) { cout << "invalid number of parameters." << endl; return -1; }
    mods[0] = atoi(argv[2]);
    divs[0] = dim;
    max = mods[0] * dim;
    for (int n = 0; n < max; n++) {  // the only loop
        // if we are still within our parameter list, continue to initialize
        if (n < argc - 3) {
            mods[n + 1] = atoi(argv[n + 3]);
            divs[n + 1] = divs[n] * mods[n];
            max = max * mods[n + 1];    
        }
        index = n % dim;
        // if there should be an opening parenthesis, print it
        if (index == 0) cout << "(";
        // print the actual number
        mod = mods[index];
        div = divs[index];
        cout << 1 + (n / div) % mod;
        // print closing parenthesis or just a comma or both
        if      (n+1 >= max)       cout << ")" << endl;
        else if ((n+1) % dim != 0) cout << ", ";
        else                       cout << "), ";
    }
}
