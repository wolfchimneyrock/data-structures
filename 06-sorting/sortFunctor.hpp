//////////////////////////////////////////////////////////////////////////////////
// sortFuntor.hpp 
// Brooklyn College CISC3130 M. Lowenthal  - Assignment #6
// Robert Wagner
// 2016-05-18
//
// This is the class definition which contains the sorting routines and the
// statistical measuring and initial dataset generation.
// 
// the quicksort algorithm comes from Robert Sedgewick (Princeton) adapted from java:
// http://algs4.cs.princeton.edu/23quicksort/
//
// the heapsort algorithm comes from Doug Baldwin (SUNY Geneseo)
// http://www.geneseo.edu/~baldwin/csci240/spring2007/0427siftdown.html
//
// the random number algorithm comes from: 
// http://codereview.stackexchange.com/questions/109260/seed-stdmt19937-from-stdrandom-device
//
//////////////////////////////////////////////////////////////////////////////////
#ifndef __sortFunctor__
#define __sortFunctor__
#include <chrono>
#include <random>
#include <iostream>
#include <string>
#include <sstream>
#include <array>
#include <vector>
#include <algorithm>
#include <functional>

#define DELIMITER ','
#define ALMOST 10
typedef size_t sortType;
typedef size_t countType;
typedef size_t indexType;
struct sortFunctor;
typedef void (sortFunctor::*sortFunction)(sortType *, indexType);
typedef std::chrono::high_resolution_clock        Clock;
typedef std::chrono::time_point<Clock>            Timer;
typedef std::chrono::duration<double, std::milli> Duration;

enum class Sorts  : int { SELECTION, QUICK, HEAP};
const std::array<Sorts, 3> allSorts = {Sorts::SELECTION, Sorts::QUICK, Sorts::HEAP};
const std::array<std::string, 3> sortNames = {"selection", "quicksort", "heapsort"};
enum class Orders : int { FORWARD, ALMOSTFORWARD, UNIFORM, ALMOSTREVERSE, REVERSE};
const std::array<Orders, 5> allOrders = {Orders::FORWARD, Orders::ALMOSTFORWARD, 
                                         Orders::UNIFORM, Orders::ALMOSTREVERSE, Orders::REVERSE};
const std::array<std::string, 5> orderNames = {"forward_sorted", "almost_forward", 
                                               "shuffled_randm", "almost_reverse", "reverse_sorted"};
static constexpr int cast(Orders a) { return static_cast<int>(a); }
static constexpr int cast(Sorts  a) { return static_cast<int>(a); }

template<class T = std::mt19937, std::size_t N = T::state_size>
auto ProperlySeededRandomEngine () -> typename std::enable_if<!!N, T>::type {
        typename T::result_type random_data[N];
            std::random_device source;
                std::generate(std::begin(random_data), std::end(random_data), std::ref(source));
                    std::seed_seq seeds(std::begin(random_data), std::end(random_data));
                        T seededEngine (seeds);
                            return seededEngine;
}

struct sortFunctor {
    countType exchanges, comparisons;
    sortFunction sorter;
    Timer startTime, endTime;
    // Duration duration;
    indexType N;
    sortType *data;
    sortType *inputs[allOrders.size()];
    bool verbose;
    std::mt19937 rd;
    std::uniform_int_distribution<sortType> dist;

    ~sortFunctor() {
       for (Orders o : allOrders)
          delete inputs[cast(o)]; 
    }

    sortFunctor(indexType N = 100, bool verbose = false) {
        this->verbose = verbose;
        this->N = N;
        //inputs.reserve(allOrders.size());
        rd = ProperlySeededRandomEngine();
        dist = std::uniform_int_distribution<sortType>(0, N - 1);
        if(verbose) std::cout << "building input sets of size " << N << ":\n" ;
        for (Orders o : allOrders) {
            inputs[cast(o)] = new sortType[N];
            generateInput(inputs[cast(o)], N, o);
            //if (verbose) print(inputs[cast(o)], N);
        }
        data = new sortType[N];
        if(verbose) std::cout << "done." << std::endl;
    }

    void reset(Sorts S, Orders O) {
        exchanges   = 0;
        comparisons = 0;
        // select the sorting algorithm
        switch (S) {
            case Sorts::SELECTION: sorter = &sortFunctor::selectionSort; break;
            case Sorts::QUICK:     sorter = &sortFunctor::quickSort;     break;
            case Sorts::HEAP:      sorter = &sortFunctor::heapSort;      break;
        }
        // copy the pre-initialized starting data to the working data
        std::copy(inputs[cast(O)], inputs[cast(O)] + N, data); 
    }

    std::string operator()(Sorts S, Orders O) {
        std::ostringstream buffer;
        reset(S, O);
        if (verbose) std::cout << "sort: N=" << N << ", " << orderNames[cast(O)] << ", " << sortNames[cast(S)] << "... ";
        std::cout.flush();
        // perform the sort and measure time elapsed
        startTime = std::chrono::system_clock::now();
        (*this.*sorter)(data, N);
        endTime   = std::chrono::system_clock::now();
        auto duration  = endTime - startTime;
        // verify the list is now sorted
        if (!equal(data, inputs[cast(Orders::FORWARD)], N)) { 
            std::cout << "[error: sort didn't sort] ";
            print(data, N); 
        }
        // output CSV
        buffer << N << DELIMITER << sortNames[cast(S)] << DELIMITER << orderNames[cast(O)] << DELIMITER
               << duration.count() << DELIMITER << exchanges << DELIMITER << comparisons;
        if (verbose) std::cout << "done: " << exchanges << " exch, " << comparisons << " cmps, " << duration.count() << " ms.\n";
        return buffer.str();
    }

    // print a list
    void print(sortType *a, indexType N) {
        std::cout << "{ ";
        for (int i = 0; i < N; i++) std::cout << a[i] << " ";
        std::cout << "}\n";
    }

    // test that two lists contains the same elements
    bool equal(sortType *a, sortType *b, indexType N) {
        bool result = true;
        for (indexType i = 0; result && i < N; i++) if(a[i] != b[i]) result = false;
        return result;
    }
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// here are the sorts exposed to the API
// they should all share the same parameter list, 
// and have their subroutines as private methods below
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void quickSort(sortType *arr, indexType N) {
        quickSplit(arr, 0, N-1);
    }

    void selectionSort(sortType *arr, indexType N) {
        indexType i, j, minIndex;    
        for (i = 0; i < N - 1; i++) {
            minIndex = i;
            for (j = i + 1; j < N; j++)
                if (compare(arr, j, minIndex) < 0)
                    minIndex = j;
            if (minIndex != i) 
                exchange(arr, i, minIndex);
        }
    }

    void heapSort(sortType *arr, indexType N) {       
        for (long k = N >> 1; k >= 0; k--) {
            heapSiftDown(arr, k, N);    
        }
        while (N - 1 > 0) {  
            exchange(arr, N - 1, 0); 
            heapSiftDown(arr, 0, N - 1);  
            N--;
        }
    }

    private:
    void exchange(sortType *arr, const indexType a, const indexType b) {
        exchanges++;
        sortType temp = arr[a];
        arr[a] = arr[b];
        arr[b] = temp;
    }

    // compare two elements of same array
    int compare(sortType *arr, const indexType a, const indexType b) {
        comparisons++;
        if (arr[a] > arr[b]) return 1;
        else if (arr[a] < arr[b]) return -1;
        else return 0;
    }
    // mimics java's compare
    int compareTo(const sortType &a, const sortType &b) {
        comparisons++;
        if      (a < b) return -1;
        else if (b < a) return 1;
        else            return 0;
    }

    bool lessThan(sortType a, sortType b) { comparisons++; return a < b; }
    bool greaterThan(sortType a, sortType b) {comparisons++; return a > b; }

    void heapSiftDown(sortType *arr, long k, long N) {
        indexType right = 2 * (k + 1);
        indexType left = right - 1;
        indexType largest = k;
        if (left < N && greaterThan(arr[left], arr[largest])) largest = left;
        if (right < N && greaterThan(arr[right], arr[largest])) largest = right;
        if (largest != k) {
            exchange(arr, k, largest);
            heapSiftDown(arr, largest, N);
        }
    }

    void quickSplit(sortType *arr, long lo, long hi) {
        if (hi <= lo) return;
        int lt = lo, gt = hi;
        indexType mid = lo + (hi - lo) / 2;
        sortType v = arr[mid];
        indexType i = lo;
        while (i <= gt) {
            int cmp = compareTo(arr[i], v);
            if      (cmp < 0) exchange(arr, lt++, i++);
            else if (cmp > 0) exchange(arr, i, gt--);
            else i++;

        }
        quickSplit(arr, lo, lt - 1);
        quickSplit(arr, gt + 1, hi);
    } // void quickSplit

    // generate the initial sets
    // forward and reverse are trivial.
    // the "almost" routines generate the initial, and then swap a certain number
    // of random places
    // the uniform random uses c++11's list shuffle routine
    void generateInput(sortType *arr, indexType N, Orders order) {
        if (verbose) std::cout << orderNames[cast(order)] << "... ";
        switch(order) {
            case Orders::FORWARD:
                for (indexType i = 0; i < N; i++) arr[i] = i + 1;
                break;
            case Orders::ALMOSTFORWARD:
                for (indexType i = 0; i < N; i++) arr[i] = i + 1;
                for (int i = N / ALMOST; i >= 0; i--)
                    exchange(arr, dist(rd), dist(rd));
                break;
            case Orders::REVERSE:
                for (indexType i = 0; i < N; i++) arr[i] = N - i;
                break;
            case Orders::ALMOSTREVERSE:
                for (indexType i = 0; i < N; i++) arr[i] = N - i;
                for (int i = N / ALMOST; i >= 0; i--)
                    exchange(arr, dist(rd), dist(rd));
                break;
            case Orders::UNIFORM:
                for (indexType i = 0; i < N; i++) arr[i] = N - i;
                std::random_shuffle(arr, arr + N); 
                break;
        }
    } // void generateInput
}; // struct sortFunctor
#endif
