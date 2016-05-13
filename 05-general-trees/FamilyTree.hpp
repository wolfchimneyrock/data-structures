//////////////////////////////////////////////////////////////////////////////////
// FamilyTree.hpp
// Brooklyn College CISC3130 M. Lowenthal  - Assignment #5
// Robert Wagner
// 2016-05-11
//
// this file contains the FamilyTree class which is organized into two major components:
//     Node   - each person in the tree
//     Family - contains the tree, and a lookup map of names to Nodes
// 
// the resolve() method uses a map-reduce paradigm to convert an ordered list of
// relations into a set of names.  all of the storage happens in each Parser object
// for cache coherency and potential threadsafe operation.
// 
// for each stage in the resolution pipeline, results are matched from a previously
// given or resolved name and a relation / modifier to a set of new names, i.e.:
//
//     'Bob' ->  { siblings } -> { older than 'Bob' } -> { youngest } -> 'Julie'
//
// if we had searched: who is Bob's youngest older sibling?
//
///////////////////////////////////////////////////////////////////////////////////
#ifndef __FAMILY_TREE
#define __FAMILY_TREE
#include <memory>
#include <string>
#include <map>
#include <set>
#include <deque>
#include "definitions.hpp"
#include "parser.hpp"

namespace FamilyTree {

    struct Node;
    typedef std::map<string, Node *> NameMap;
    typedef std::deque<Node *>       NodeQueue;
    struct Node {
        private:
        Node *child, *sibling, *parent;
        string name;
        int date;

        public:
        Node()  { child = nullptr; sibling = nullptr; parent = nullptr; }
        Node(const string& name_, int date_) : Node() { name.assign(name_); date = date_; }
        const string &getName() const { return name; }
        // destructor is recursive and will delete its subtree before itself
        ~Node() { 
            if(child) delete child; if(sibling) delete sibling; 
        }
        Node &operator=(const Node& other) {
            child   = other.child;
            sibling = other.sibling;
            parent  = other.parent;
            name    = other.name;
            date    = other.date;
            return *this;
        }
        static bool dateCompare(Node*& a, Node*& b) {
            return a->date < b->date;
        }

        Node *addChild(const string& name, int date) {
            Node *child = new Node(name, date);
            if (this->child == nullptr) this->child = child;
            else {
                
                Node *p = this->child;
                Node *q = p;
                while(p) { q = p; p = p->sibling; }
                q->sibling = child;
            }
            child->parent = this;
            return child;
        }

        void filter(NodeQueue& q, ModifiedRelation mr) {
            Modifiers m = mr.modifier;
            if ((m & (Modifiers::AGE)) != Modifiers::NONE) {
                std::sort(q.begin(), q.end(), &Node::dateCompare);
                if ( (m & Modifiers::OLDER) == Modifiers::OLDER)
                    while (!q.empty() && this->date < q.back()->date) q.pop_back();
                else if ( (m & Modifiers::YOUNGER) == Modifiers::YOUNGER)
                    while (!q.empty() && this->date > q.front()->date) q.pop_front();
                if ( (m & Modifiers::OLDEST) == Modifiers::OLDEST)
                    while (q.size() > 1) q.pop_back();
                else if ( (m & Modifiers::YOUNGEST) == Modifiers::YOUNGEST)
                    while (q.size() > 1) q.pop_front();
            }
        }

        void getRelatives(NodeQueue &result, ModifiedRelation query) { // get all of a certain relation
            switch(query.relation) {
                case Relations::SELF:                     result.push_back(this);         break;
                case Relations::PARENT: if (this->parent) result.push_back(this->parent); break;
                case Relations::CHILD: {
                    Node *p = this->child;
                    while(p) { result.push_back(p); p = p->sibling; }
                } break;
                case Relations::SIBLING: {
                    Node *p = this->parent->child;
                    while(p) { if (p != this) result.push_back(p); p = p->sibling; }
                } break;
                case Relations::GRANDPARENT:
                    if ((query.modifier & Modifiers::GREAT) == Modifiers::NONE) {
                        if (this->parent && this->parent->parent) 
                            result.push_back(this->parent->parent);
                    } else {
                        if (this->parent && this->parent->parent && this->parent->parent->parent)
                            result.push_back(this->parent->parent->parent);
                    }
                break;
                case Relations::AUNTORUNCLE: {
                    Node *parent = this->parent;
                    Node *grand  = parent ? parent->parent : nullptr;
                    if (grand) {
                        if ((query.modifier & Modifiers::GREAT) == Modifiers::GREAT) {
                            parent = grand;
                            grand = grand->parent;
                            if (!grand) break;
                        }
                        Node *p = grand->child;
                        while(p) {
                            if (p != parent) result.push_back(p);
                            p = p->sibling;
                        }
                    }
                } break;
                case Relations::COUSIN: {
                    if (this->parent && this->parent->parent) {
                        Node *p = this->parent->parent->child;
                        NodeQueue q;
                        ModifiedRelation descendents(Relations::DESCENDENT,
                                                    Modifiers::NONE,
                                                    "");
                        while(p) { 
                            if (this->parent != p) 
                                p->getRelatives(result, descendents);
                            p = p->sibling;
                        }
                        
                    } 
                } break;
                case Relations::NIECEORNEPHEW: {
                    // TBD
                } break;
                case Relations::GRANDCHILD: {
                    Node *p = this->child;
                    while(p) {
                        Node *q = p->child;
                        while (q) {
                            result.push_back(q);
                            q = q->sibling;
                        }
                        p = p->sibling;
                    }
                } break;
                case Relations::ANCESTOR: {
                    Node *p = this->parent;
                    while(p) {
                        result.push_back(p);
                        p = p->parent;
                    }
                } break;
                case Relations::DESCENDENT: {
                    if (this->child) {
                        ModifiedRelation descendents(Relations::DESCENDENT,
                                                    Modifiers::NONE,
                                                    "");
                        Node *p = this->child;
                        while(p) {
                            result.push_back(p);
                            p->getRelatives(result, descendents);
                            p = p->sibling;
                        }
                    }
                } 
                default:
                break;
            }
            //return result;
        }
    }; // struct Node
    
    class Family {
        private:
        NameMap nmap;
        Node *tree;
                
        public:
        Family() { clear(); } 
        ~Family() { delete tree; }       // recursively delete the tree
        Family &operator=(const Family &other) { 
            nmap = other.nmap; tree = other.tree;
            return *this;
        }
        void clear() { 
            tree = new Node("", 0);
            nmap.clear();
            nmap[""] = tree;
        }
        Result add(Parser &ps) {
            int count = 0;
            int duplicates = 0;
            NameQueue& names = ps.names;
            DateQueue& dates = ps.dates;
            if (ps.subR.empty()) { 
                ps.message += fmtErrorNoSubject;
                return Result::BADQUERY;
            }
            if (names.size() != dates.size()) {
                ps.message += fmtErrorDateMismatch;
                return Result::BADQUERY;
            }
            if (names.size() == 0) {
                ps.message += fmtErrorNoObjects;
                return Result::BADQUERY;
            }
            string parent = ps.subR.front().name;
            auto it = nmap.find(parent);
            if (it == nmap.end()) {
                ps.message += fmtErrorNoSubject;
                return Result::BADQUERY;
            }
            Node *node = it->second;
            while (!names.empty() && !dates.empty()) {
                if (nmap.find(names.front()) == nmap.end()) {
                    auto child = node->addChild(names.front(), dates.front());
                    nmap[names.front()] = child;
                    count++;
                } else { duplicates++; }
                names.pop_front();
                dates.pop_front();
            }
            ps.message += str(fmtMessageAdd % count % parent % duplicates);
            return Result::OK;
        }
        
        Result resolve(Parser &ps) {
            // this is actually exactly like towers of hanoi
            NameQueue     &dest   = ps.names;
            RelationQueue &source = ps.subR;
            NameQueue     &alt    = ps.objects;
            NodeQueue     out;
            string        current;
            if (source.empty()) {
                ps.message += fmtErrorNoSubject;
                return Result::BADQUERY;
            }
            // don't resolve for an add we need to keep whats in names
            if (ps.command == Commands::ADD) return Result::OK;
            if (ps.command == Commands::NEW) return Result::OK;
            alt.clear();
            while (!source.empty()) {
                auto next = source.front();
                if (next.relation == Relations::SELF) {
                    current = next.name;
                    if (nmap.find(current) != nmap.end())
                        dest.push_back(current);
                    else ps.message += fmtErrorNoSubject;
                } else {
                    while (!dest.empty()) {
                        current = dest.front();
                        dest.pop_front();
                        auto it = nmap.find(current);
                        if (it == nmap.end()) {
                            ps.message += fmtErrorNoSubject;
                            return Result::BADQUERY;
                        }
                        auto node = it->second;
                        node->getRelatives(out, next);
                        // sort happens in filter if necessary
                        node->filter(out, next);
                        //if (out.empty()) return Result::NORESULTS;
                        while (!out.empty()) {
                            current = out.front()->getName();
                            //if (nmap.find(current) == nmap.end())
                                alt.push_back(current);
                            //else {
                            //    ps.message += fmtErrorNoSubject;
                            //    return Result::BADQUERY;
                            // }
                            out.pop_front();
                        }
                    }
                    dest.swap(alt);
                }
             
                if (!source.empty()) source.pop_front();
                if (ps.debug) std::cout << ps.toString();      
            }
            if (dest.size() > 0)
                {} //ps.message += str(fmtMessageResolve % dest.size());
            else ps.message += fmtErrorNoResults;
            return Result::OK;
        }
        
        Result execute(Parser &ps) {
            Result result;
            switch(ps.command) {
                case Commands::CLEAR: clear();                   break;
                case Commands::NEW:   clear(); result = add(ps); break;
                case Commands::ADD:            result = add(ps); break;
                case Commands::QUERY: for (auto name : ps.names)
                                          ps.message += name + " ";
                                      ps.message += "\n";
                break;
                case Commands::PRINT:
                break;
                case Commands::TEST:
                break;
                case Commands::COUNT:
                break;
                case Commands::STATS:
                break;
            }
            return result;
        }
    }; // class FamilyTree
} // namespace FamilyTree

#endif
