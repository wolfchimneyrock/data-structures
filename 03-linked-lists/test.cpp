#include <iostream>
#include <string>
#include <stdlib.h>
#include "LinearQueue.hpp"

using namespace std;

int main(int argc, char *argv[]) {
    LinearQueue<int> Q;
    for (int i = 1; i < argc; i++) {
        int s = atoi(argv[i]);
        Q.enQueue(s);
    }
    LinearQueue<int> R = Q;
    cout << Q.size() << " items enqueued." << endl;
    while (!Q.isEmpty()) {
        auto item = Q.peek();
        cout << "item:    " << item << endl;
        Q.deQueue();
        
    }
    cout << " copy: " << endl;
    while (!R.isEmpty()) {
        auto item = R.peek();
        cout << "item:    " << item << endl;
        R.deQueue();
    }
    
}
