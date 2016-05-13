///////////////////////////////////////////////////////////////////////////////////
// LinearQueue.hpp
// Brooklyn College CISC3130 M. Lowenthal  - Assignment #3
// Robert Wagner
// 2016-03-04
//
// this is a templated queue class with the following api:
//     LinearQueue<T>()      create and returns a new empty queue containing type T
//     bool    isEmpty()     returns true if the queue is empty, false otherwise
//     size_t  size()        returns number of items in the queue
//             enQueue(d)    adds item d of type T to end of queue
//             deQueue()     deletes the front of the queue
//     T&      peek()        returns reference to the item at the front of the queue
//                           you have to explictly copy of you want a copy
//                           i.e. string s = new string(LQ.peek());
//             replace()     replace the data in the head item with new value
//
//     *** peek() and make a copy before deQueue() !!! ***
//
// enQueue(), deQueue(), replace() can be chained i.e.Q.enQueue(5).enQueue(3).deQueue();
//
// this is implemented with a circular linked list with one sentinel node. As items
// are enqueued, the previous sentinel becomes the new item's node, and a new sentinel
// is inserted.  this allows access to both the front and back of the list with one
// pointer into the list, and without having to worry about special enqueue condition
// into an empty list.
///////////////////////////////////////////////////////////////////////////////////

#ifndef __LINEAR_QUEUE
#define __LINEAR_QUEUE
#include <memory>

template <class T>
class LinearQueue {
    private:
    // this is an individual node in the queue.  It is hidden from the API
    struct Node {
        private:
        T data;
        Node *next;
        public:

        // implement rule-of-three - constructor, destructor, copy constructor
        Node() {}
        ~Node() { next = nullptr; }
        Node &operator=(const Node &other) { next = other.next; data = other.data; }

        Node *getNext()           { return next; }
        T&    getData()           { return data; }
        void  setData(const T& d) { data = d; }
        void  setNext(Node *n)    { next = n; }
    }; // inner struct Node

    Node *sentinel;
    size_t count;

    public:
    // in an empty queue, the sentinel points to itself like ouroborus
    LinearQueue() { count = 0; sentinel = new Node(); sentinel->setNext(sentinel); }

    // to destroy the queue, loop through and dequeue all items, then delete the sentinel
    ~LinearQueue() {
        while(!isEmpty()) deQueue();
        delete sentinel;
    }
    // copy constructor
    LinearQueue (const LinearQueue &other) {
        count = 0;
        sentinel = new Node();
        sentinel->setNext(sentinel);
        Node *currentOther = (other.sentinel)->getNext();
        while (currentOther != other.sentinel) {
            T& data = currentOther->getData();
            enQueue(data);
            currentOther = currentOther->getNext();
        }
    }

    // empty condition is easy - sentinel points to itself
    bool isEmpty() const { return sentinel == sentinel->getNext(); }

    size_t size() const { return count; };

    // enQueue returns itself to allow chaining
    LinearQueue& enQueue(const T& data)  {
        Node *newSentinel = new Node();
        newSentinel->setNext(sentinel->getNext());
        sentinel->setData(data);
        sentinel->setNext(newSentinel);
        sentinel = newSentinel;
        count++;
        return *this;
    }
    
    // deQueue returns itself to allow chaining
    LinearQueue& deQueue() {
        if (isEmpty()) throw std::runtime_error("No element to deQueue.");
        count--;
        Node *removed = sentinel->getNext();
        sentinel->setNext(removed->getNext());
        delete removed;
        return *this;
    }

    T& peek() const {
        if (isEmpty()) throw std::runtime_error("No element to peek.");
        return (sentinel->getNext())->getData();
    }

    LinearQueue& replace(const T& data)  {
        if (isEmpty()) throw std::runtime_error("No element to replace.");
        (sentinel->getNext())->setData(data);
        return *this;
    }
}; // class LinearQueue

#endif
