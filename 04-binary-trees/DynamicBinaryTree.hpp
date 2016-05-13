///////////////////////////////////////////////////////////////////////////////////
// DynamicBinaryTree.hpp
// Brooklyn College CISC3130 M. Lowenthal  - Assignment #4
// Robert Wagner
// 2016-03-22
//
// this is a templated tree class with the following api:
//     DynamicBinaryTree<T>()  create and returns a new empty queue containing type T
//     bool    isEmpty()       returns true if the queue is empty, false otherwise
//     size_t  count()         returns number of items in the queue
//     size_t  order()         returns the highest # of levels of the tree
//             insert(d)       adds item d of type T to tree
//             remove(d)       deletes item d from tree
//             clear()         erases contents of tree for re-use
//     bool    contains(d)     returns true if item d is present in the tree
//             traverse(t,f)   performs functor f with specified traversal
//
// insert(), remove(), and traverse() can be chained 
//
// the traversal function requires a functor to be passed.  this may contain state
// to do things such as sum, average, etc over all of the tree elements.
// a prototypical functor:
//
//   struct MeanFunctor {
//       double sum, count;
//     public:
//       MeanFunctor() { sum = 0; count = 0; }
//       void operator()(const Node& node) const {
//           sum = sum + node->getData(); 
//           count++;
//       }
//       double result() const { return sum / count; }
//   }
//
// another example functor, this one builds a string of element representations:
//
//   template <typename T>
//   struct PrintFunctor {
//     string separator;
//     stringstream ss;
//     public:
//       PrintFunctor(const string& s) { separator = s; }
//       void operator()(const Node& node) const {
//           ss << node->getData() << separator;
//       }
//       string result() const { return ss.str(); }
//   }
//
// then to traverse, you do the following (assuming "Tree" is an already made tree):
//
//   PrintFunctor Print(" ");
//   MeanFunctor Mean;
//   Tree.traverse(inOrder, Print);
//   Tree.traverse(inOrder, Mean);
//   cout << "Tree: " << Print.result() << endl;
//   cout << "Mean: " << Mean.result() << endl;
//
///////////////////////////////////////////////////////////////////////////////////

#ifndef __DYNAMIC_BINARY_TREE
#define __DYNAMIC_BINARY_TREE
#include <memory>
namespace DynamicBinaryTree {
enum class Traversals {
    INORDER,
    PREORDER,
    POSTORDER
};

template <typename T>
    struct Node {
        private:
        T data;
        Node *left, *right, *parent;
        public:

        // implement rule-of-three - constructor, destructor, copy constructor
        Node() { left = nullptr; right = nullptr; parent = nullptr; }

        // notice that the destructor is recursive and will delete all subtree
        // nodes before letting itself be destroyed.  if you want to save the 
        // contents of a subtree, you need to set the parent link referring to 
        // the subtree as nullptr before you destroy it
        ~Node() { 
            if (this->left) delete left;
            if (this->right) delete right; 
        }
        Node &operator=(const Node &other) {
            left = other.left;
            right = other.right;
            parent = other.parent;
            data = other.data;
            return this;
        }

        // static factory method for getNode
        static Node *getNode(const T& d) {
            Node *temp = new Node();
            temp->data = d;
            return temp;
        }

        Node  *getLeft()   const   { return left;  }
        Node  *getRight()  const   { return right; }
        Node  *getParent() const   { return parent;}
        const T& getData() const   { return data;  }
        void   setData(const T& d) { data = d;     }
        void   setLeft(Node *n)    { left = n;  if (n) n->setParent(this); }
        void   setRight(Node *n)   { right = n; if (n) n->setParent(this); }
        // caution - setParent() doesn't go back and fix the old parent's links
        void   setParent(Node *n)  { parent = n; }

        // traversals, passed a functor
        template <typename F>
        void traverseInOrder( F& f ) {
            if (left)  left->traverseInOrder(f);
                       f(this);
            if (right) right->traverseInOrder(f);
        }
        template <typename F>
        void traversePreOrder( F& f ) {
                       f(this);
            if (left)  left->traversePreOrder(f);
            if (right) right->traversePreOrder(f);
        }
        template <typename F>
        void traversePostOrder( F& f ) {
            if (left)  left->traversePostOrder(f);
            if (right) right->traversePostOrder(f);
                       f(this);
        }

        // find the descendant node containing d, or nullptr if not found
        Node *find(const T& d) { 
            if (data == d)           return this;
            if (left && (d < data))  return left->find(d);
            if (right && (d > data)) return right->find(d);
                                     return nullptr; 
        }
        
        // insert a new node with d and return a pointer to the new node
        // if we stumble on a node already containing d, return nullptr
        Node *insert(const T& d) {
            if (data == d) return nullptr;
            Node *current = (Node *)this; // cast to get rid of 'const'
            Node *found;
            while (current) {
                found = current;
                     if (d < found->data) current = current->getLeft();
                else if (d > found->data) current = current->getRight();
                else                      return nullptr;
            }
            Node *newNode = Node::getNode(d);
            // "found" should have a nullptr in the apropos direction
            if (d < found->data) found->setLeft(newNode);
            else                 found->setRight(newNode);
            return newNode;
        }

          // the way the delete works is to recursively swap the deleted
          // node's value with that of its descendent predecessor or successor
          // until it gets to a child with no children, then simply deletes that
        void remove() {
            Node<T> *parent = this->getParent();
            Node<T> *temp;
            int children = 0;
            if (this->left) children += 1;
            if (this->right) children += 2;
            switch (children) {
                case 0: // no children
                    if(parent) {
                        if (this == parent->getLeft()) parent->setLeft(nullptr);
                        else parent->setRight(nullptr);
                    }
                    delete this;
                    break;
                case 1: // only left child
                    temp = predecessor();
                    setData(temp->getData());
                    temp->remove();
                    break;
                case 2: // only right child
                    temp = successor();
                    setData(temp->getData());
                    temp->remove();
                    break;
                default: // both children - could use either successor or pred.
                    temp = successor();
                    setData(temp->getData());
                    temp->remove();
                    break;
            }
        }

        // find the most immediate predecessor in descendants
        // used to delete a node
        Node *predecessor() {
            Node *current = this->left;
            Node *found = current;
            while (current) {
                found = current;
                current = current->right;
            }
            return found;
        }

        // find the most immediate successor in descendants
        // used to delete a node
        Node *successor() {
            Node *current = this->right;
            Node *found = current;
            while (current) {
                found = current;
                current = current->left;
            }
            return found;
        }

}; // struct Node

template <typename T>
class DynamicBinaryTree {
    private:
    Node<T> *head;
    size_t _count;

    public:
    // empty constructor
    DynamicBinaryTree() { _count = 0; head = nullptr; }

    // to destroy the tree, run the recursive node delete on the head
    ~DynamicBinaryTree() {
        if(!isEmpty()) delete head;
    }
    // copy constructor - NOTE : shallow copy
    DynamicBinaryTree<T>& operator=(const DynamicBinaryTree<T> &other) {
        _count = other.count();
        head = other.head;
        return *this;
    }

    // empty condition is easy
    bool isEmpty() const { return _count == 0; }

    size_t count() const { return _count; };

    bool contains(const T& data) const {
        if (isEmpty()) return false;
        return (head->find(data) != nullptr);
    }
    
    // clear wipes the contents to an empty tree
    DynamicBinaryTree<T>& clear() {
        // since the node's destructor calls its childrens destructors,
        // this is equivalent to recursively delete the tree bottom-up.
        if (!isEmpty()) delete head;
        _count = 0;
        return *this;
    }

    // insert returns itself to allow chaining
    DynamicBinaryTree<T>& insert(T& data) {
             if (isEmpty())          { head = Node<T>::getNode(data); _count++; }
        else if (head->insert(data)) _count++;
        return *this; 
    }
    
    // delete returns itself to allow chaining
    DynamicBinaryTree<T>& remove(const T& data) {
        if (isEmpty()) return *this;
        Node<T> *found = head->find(data);
        if (found == nullptr) return *this;
        found->remove();
        _count--;
        if (isEmpty()) head = nullptr;
        return *this;
    }

    template <typename F>
    const DynamicBinaryTree<T>& traverse(Traversals type, F& f ) {
        if (!isEmpty()) {
            switch(type) {
                case Traversals::INORDER:
                    head->traverseInOrder(f);
                    break;
                case Traversals::PREORDER:
                    head->traversePreOrder(f);
                    break;
                case Traversals::POSTORDER:
                    head->traversePostOrder(f);
                    break;
            }
        }
        return *this;
    }


}; // class DynamicBinaryTree
}  // namespace DynamicBinaryTree
#endif
