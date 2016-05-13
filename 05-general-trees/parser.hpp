///////////////////////////////////////////////////////////////////////////////////
// parser.hpp
// Brooklyn College CISC3130 M. Lowenthal  - Assignment #5
// Robert Wagner
// 2016-05-11
//
// this file contains a natural language token parser for the family tree
// key features:
//     Parser object can be passed as a functor to the family tree,
//     which promotes cache coherency and would allow multiple parser clients
//     to access data in the same family tree concurrently.  also by instantiating
//     the Parser in the main() method we don't have to worry about any weird
//     object ownership issues.
//
//     a simple set of grammar rules is encoded in the expects() method
//     the Parser object contains both the query data and the result data
//     and can perform multiple levels of resolution:
//
//         who are the youngest children of Bob's siblings?
//
//     would return a youngest child for each of Bob's siblings, whereas:
//
//         who are the children of Bob's youngest sibling?
//
//     would return all of the children for one of Bob's siblings (youngest)
//
//     to enable debug messages, set the Parser object's debug variable to true 
///////////////////////////////////////////////////////////////////////////////////
#ifndef __FAMILY_TREE_PARSER
#define __FAMILY_TREE_PARSER
#include <cctype>
#include <iostream>
#include <string>
#include "definitions.hpp"

namespace FamilyTree {
    using std::string;
    using std::cout;
    struct Parser {
	    bool  hasCommand, hasName, isSubject, isObject,
	          isModified, isError, debug;
	    RelationQueue subR, objR, unkR;
	    Commands  command;
	    Tokens    found, last;
	    string    message;
	    NameQueue names, objects;
	    DateQueue dates;
            Modifiers modifiers;
	void reset() {
            debug       = false;
            hasCommand  = false;
            hasName     = false;
	    isSubject   = false;
	    isObject    = false;
	    isError     = false;
            modifiers   = Modifiers::NONE;
	    command     = Commands::PRINT; // default command is print
	    found       = Tokens::NOTHING;
	    last        = Tokens::NOTHING;
	    message     = "";  // this will contain any status / error 
            names.clear();
            objects.clear();
	    dates.clear();
            subR.clear();
            objR.clear();
            unkR.clear();
	}
	Parser() { reset(); }
        string toString() {
            string result = "";
            if (isError) result += "Errors found.\n"; 
            else result += "No Errors.\n";
            if (hasCommand) {
                auto cmd = COMMANDNAME.find(command);
                result += "[Command]\n" + cmd->second + "\n";
            } else result += "[No Command]\n";

            if (subR.empty()) result += "[No Subjects]\n";
            else {
                result += "[Subject]\n";
                for (auto sub : subR)
                    result += sub.print();
                result += "\n";
            }
            
            if (objR.empty()) result += "[No Objects]\n";
            else {
                result += "[Object]\n";
                for (auto obj : objR)
                    result += obj.print();
                result += "\n";
            }
            
            if (unkR.empty()) result += "[No Temp]\n";
            else {
                result += "[Temporary]\n";
                for (auto unk : unkR)
                    result += unk.print();
                result += "\n";
            }
            
            if (names.empty()) result += "[No Names]\n";
            else {
                result += "[Names]\n";
                for (auto name : names)
                    result += name + " ";
                result += "\n";
            }
         
            if (dates.empty()) result += "[No Dates]\n";
            else {
                result += "[Dates]\n";
                for (auto date : dates)
                    result += std::to_string(date) + " ";
                result += "\n";
            }

            return result;
        } 
        // based on the last token found, return what tokens are expected next.
        // a lot of the grammar rules are encoded here.
        Tokens expected() const {
            Tokens result;
            switch (last) {
                case Tokens::NOTHING:    result = Tokens::NAME | Tokens::COMMAND;    break;
                case Tokens::NAME:       result = Tokens::ALL;                       break;
                case Tokens::COMMAND:    result = Tokens::NONCOMMAND;                break;
                case Tokens::MODIFIER:   result = Tokens::OPERATOR;                  break;
                case Tokens::RELATION:   result = Tokens::ALL;                       break;
                case Tokens::PROJECTIVE: result = Tokens::NAME;                      break;
                case Tokens::POSSESSIVE: result = Tokens::NAME | Tokens::OPERATOR;   break;
                case Tokens::DATE:       result = Tokens::ANY;                       break; 
               default:                  result = Tokens::NOTHING;                   break;
            }
            if (debug) cout << "\t[last: " << static_cast<int>(last)
                            << " expected: " << static_cast<int>(result) << "] ";
            return result;
        }

        // parse a token.  don't validate inputs that is FamilyTree's job
        void operator()(const string& token) {
            bool complete = false;
            // check if a possessive
            // if we find a name with a possessive we know we are in subject clause
            // if we find a relation with a possessive after it is also subject
            if (!complete && (token.compare("s") == 0)) {
                found = Tokens::POSSESSIVE;
                if ((expected() & found) == found) {
                    switch (last) {
                        case Tokens::NAME:
                            // take the last item off objects and make it subject
                            // should be at least one since last == NAME
                            // if isSubject already set, previous name already
                            // in subR and not in names so do nothing
                            if(!isSubject) {
                                string name(names.back());
                                subR.push_back(
                                    ModifiedRelation(Relations::SELF,
                                                     Modifiers::NONE,
                                                     name
                                ));
                                names.pop_back();
                                isSubject = true;
                                complete = true;
                             }
                        break;
                        case Tokens::RELATION: isSubject = true;
                        break;
                        default: isError = true;
                        break;
                    }
                } else isError = true; 
            }

            // check if a projective i.e. in a sentence: 'oldest child _of_ sam'
            // object clause preceeds, subject clause follows
            if (!complete && (token.compare("of") == 0)) {
                found = Tokens::PROJECTIVE;
                if ((expected() & found) == found) {
                    // this only makes sense if no names have been named yet
                    if (!hasName) {
                        // any relations before this are object,
                        // unknown relations before the projective moved to object
                        while (!unkR.empty()) {
                            objR.push_back(unkR.front());
                            unkR.pop_front();
                        }
                        isObject = false;
                        isSubject = true; // further tokens are subject
                        complete = true;
                    } else isError = true;
                } // no error here 'of' could be a fill word if not expected
            }

            // check if a proper-case name and add to queue if so
            if (!complete && std::isupper(token.front())) { 
                found = Tokens::NAME;
                if ((expected() & found) == found) {
                    if (isSubject) {  // we can directly put it on subR
                        subR.push_back(
                            ModifiedRelation(Relations::SELF,
                                             Modifiers::NONE,
                                             token
                        )); 
                    } else names.push_back(token); // but not objR
                    hasName = true;
                    complete = true;
                } else isError = true;
            }

            if (!complete && !hasCommand) {
                auto cmd = COMMANDKEY.find(token);
                if (cmd != COMMANDKEY.end()) {
                    found = Tokens::COMMAND;
                    if ((expected() & found) == found) {
                        // any name / relation before this is subject
                        // any after is object
                        if (names.size() < 2) {
                            while (!names.empty()) {
                                subR.push_back(
                                    ModifiedRelation(Relations::SELF,
                                                     Modifiers::NONE,
                                                     names.front()
                                ));
                                names.pop_front();
                            }
                            while (!unkR.empty()) {
                                subR.push_back(unkR.front());
                                unkR.pop_front();
                            }
                        } else isError = true;
                        hasCommand = true;
                        isSubject  = false;
                        isObject   = true;
                        command    = cmd->second;
                        complete   = true;
                    } else isError = true;
                }
            }

            // we can have many modifiers
            // this using bitmasking
            // the expand function should handle modifier priority
            if (!complete) {
                auto mod = MODIFIERKEY.find(token);
                if (mod != MODIFIERKEY.end()) {      // a modifier found
                    found = Tokens::MODIFIER;
                    if ((expected() & found) == found) {
                        modifiers = modifiers | mod->second;
                        complete = true;
                    } else isError = true;
                }

            // check if a date crudely - first character is numeric
            if (!complete && isdigit(token.front())) {
               try {
                   int value = std::stoi(token);
                   found = Tokens::DATE;
                   if ((expected() & found) == found) {
                       dates.push_back(value);
                       complete = true;
                   } else { isError = true; }
               } catch(std::exception &e) { isError = true; }; 
            }

            // check for relations, requires stemming so do it last
            //if (!hasRelation) {
            if (!complete) {
                string stemmed(token);
                if (stemmed.back() == 's') stemmed.pop_back();
                auto rel = RELATIONKEY.find(stemmed);
                if (rel != RELATIONKEY.end()) {
                    found = Tokens::RELATION;
                    if ((expected() & found) == found) {
                        //hasRelation = true;
                        ModifiedRelation r(rel->second, modifiers, "");
                        if      (isSubject) subR.push_back(r);
                        else if (isObject)  objR.push_back(r);
                        else                unkR.push_back(r);
                        modifiers = Modifiers::NONE;
                        complete = true;
                    } else isError = true;
                }
            }
            // if complete is false, it must have been a filler word
        } // operator()
        last = found;
        }
        void finalize() {
            // if there is not yet a subject found through a possessive, 
            // the first name becomes the subject
            if (subR.empty() && names.size() > 0) {
                subR.push_back(ModifiedRelation(Relations::SELF,
                                                Modifiers::NONE,
                                                names.front()));
                names.pop_front();
            }
            while (!objR.empty()) {
                subR.push_back(objR.front());
                objR.pop_front();
            }
            while (!unkR.empty()) {
                subR.push_back(unkR.front());
                unkR.pop_front();
            }
            if (!hasCommand) {
                command = Commands::PRINT;
                hasCommand = true;
            }
            // kludge fix for NEW command to act consistently with others
            if (command == Commands::NEW) {
                if (subR.size() == 0) isError = true;
                else {
                    names.push_front(subR.front().name);
                    subR.pop_front();
                    subR.push_front(ModifiedRelation(Relations::SELF,
                                                     Modifiers::NONE,
                                                     ""));
                }
            }
            if (dates.size() != names.size()) isError = true;
        } 
    }; // struct Parser
} // namespace FamilyTree
#endif
