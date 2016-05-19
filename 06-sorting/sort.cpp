//////////////////////////////////////////////////////////////////////////////////
// sort.cpp 
// Brooklyn College CISC3130 M. Lowenthal  - Assignment #6
// Robert Wagner
// 2016-05-18
//
// This is the main routine which loops through the dataset sizes and generates
// CSV data based time elapsed, exchanges, and comparisons of each sort.
// if no filename is passed, it outputs the data to console
//
//////////////////////////////////////////////////////////////////////////////////
#include <iostream>
#include <fstream>
#include "sortFunctor.hpp"

int main(int argc, char *argv[]) {
    // parameter list: <start> <end> <step> <output>
    // starting  - starting value
    // ending    - ending value
    // count     - iteration count
    // output    - file to send results to
    
    indexType starting, ending, count, step;
    if (argc < 4) {
        std::cout << "usage: sortstats [start] [stop] [step] <output>\n";
        return -1;
    }

    try {
        starting = atol(argv[1]);
        ending   = atol(argv[2]);
        count    = atol(argv[3]);
    } catch(std::exception &e) {
        std::cout << "error: invalid parameter.\n";
        return -1;
    }
    if (ending < starting) {
        std::cout << "error: ending value must be greater than starting value.\n";
        return -1;
    }
    if (count < 1) {
        std::cout << "error: count must be a natural number.\n";
        return -1;
    }
    std::ofstream outFile;
    std::ostream* output = &std::cout; // default to cout if no file specified
    if (argc > 4) {
        outFile.open(argv[4]);
        if (outFile.good()) { output = &outFile; }
        else outFile.close();
    }
    bool verbose = true;
    // write CSV column names first, used by the R script
    (*output) << "size,sort,initial_order,ms_elapsed,exchanges,compares" << std::endl;
    step = (ending - starting) / count;
    for (indexType i = starting; i <= ending; i += step) {
        sortFunctor sort(i, verbose);
        for (Sorts s : allSorts)
            for (Orders o : allOrders)
                (*output) << sort(s, o) << std::endl;
    }
    if (outFile) outFile.close();
} // main



