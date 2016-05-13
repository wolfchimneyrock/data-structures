//////////////////////////////////////////////////////////////////////////////////
// definitions.hpp
// Brooklyn College CISC3130 M. Lowenthal  - Assignment #5
// Robert Wagner
// 2016-05-11
//
// this file contains definitions for the parser and family tree classes
// and also text output format strings
// 
///////////////////////////////////////////////////////////////////////////////////
#ifndef __FAMILY_TREE_DEFINITIONS
#define __FAMILY_TREE_DEFINITIONS
#include <memory>
#include <string>
#include <map>
#include <set>
#include <deque>
#include <boost/format.hpp>

namespace FamilyTree {
    using std::string;
    using std::map;
    using std::make_shared;
    using std::deque;
    using boost::format;
    const string fmtPrompt            = ">> ";
    const string fmtErrorNoSubject    = "Error: No such subject found.\n";
    const string fmtErrorDateMismatch = "Error: Name / Date quantity mismatch.\n";
    const string fmtErrorNoObjects    = "Error: No objects found.\n";
    const string fmtErrorNoResults    = "No results found.\n";
    format fmtEcho           ("[%4i] %s%s\n");
    format fmtMessageAdd     ("%i child(ren) born to [%s],  %i duplicates skipped.\n");
    format fmtMessageNew     ("A new family tree started with %s in %i.\n");
    format fmtMessageResolve ("%i names resolved successfully.\n");
    enum class Relations { SELF, PARENT, GRANDPARENT, CHILD, SIBLING,
                           AUNTORUNCLE, COUSIN, NIECEORNEPHEW, GRANDCHILD,
                           DESCENDENT, ANCESTOR, RELATIVE                    }; 
    enum class Commands  { CLEAR, NEW, ADD, QUERY, PRINT, TEST, COUNT, STATS };
    enum class Modifiers : int { 
        NONE        = 0,
        YOUNGEST    = 1,
        YOUNGER     = 2,
        OLDER       = 4,
        OLDEST      = 8,
        GREAT       = 16,
        LIVING      = 32,
        DEAD        = 64,
        COMMON      = 128,
        MAX         = 256,
        AGE         = YOUNGEST | YOUNGER | OLDER | OLDEST
    };
    inline constexpr Modifiers operator&(Modifiers a, Modifiers b) 
    { return static_cast<Modifiers>(static_cast<int>(a) & static_cast<int>(b)); }
    inline constexpr Modifiers operator|(Modifiers a, Modifiers b) 
    { return static_cast<Modifiers>(static_cast<int>(a) | static_cast<int>(b)); }
    string modString(const Modifiers &m);
    struct ModifiedRelation {
        Modifiers modifier;
        Relations relation;
        string    name;
        ModifiedRelation(const Relations &r, const Modifiers &m, const string &n) :
            relation(r), modifier(m), name(n)
            {}
        string print() const;
    };
    enum class Tokens : int  { 
        NOTHING     = 0,
        FILLER      = 1,
        COMMAND     = 2,
        MODIFIER    = 4,
        RELATION    = 8,
        SUBJECT     = 16,
        OBJECT      = 32,
        DATE        = 64,
        POSSESSIVE  = 128,
        PROJECTIVE  = 256,
        NAME        = SUBJECT | OBJECT,
        POSSESSED   = POSSESSIVE | PROJECTIVE,
        OPERATOR    = MODIFIER | RELATION,
        OPERAND     = NAME | DATE,
        NONCOMMAND  = OPERATOR | OPERAND,
        ANY         = COMMAND | NONCOMMAND,
        ALL         = ANY | POSSESSIVE
    };
    const map<Tokens, string> TOKENNAME = 
    {   { Tokens::NOTHING,       "NONE"               },
        { Tokens::FILLER,        "FILLER"             },
        { Tokens::COMMAND,       "COMMAND"            },
        { Tokens::MODIFIER,      "MODIFIER"           },
        { Tokens::RELATION,      "RELATION"           },
        { Tokens::SUBJECT,       "SUBJECT"            },
        { Tokens::OBJECT,        "OBJECT"             },
        { Tokens::DATE,          "DATE"               },
        { Tokens::POSSESSIVE,    "POSSESSIVE"         },
        { Tokens::PROJECTIVE,    "PROJECTIVE"         }
    };
    inline constexpr Tokens operator&(Tokens a, Tokens b) 
    { return static_cast<Tokens>(static_cast<int>(a) & static_cast<int>(b)); }

    inline constexpr Tokens operator|(Tokens a, Tokens b) 
    { return static_cast<Tokens>(static_cast<int>(a) | static_cast<int>(b)); }
    
    const map<string, Relations> RELATIONKEY = 
    {   { "parent",          Relations::PARENT        },
        { "mother",          Relations::PARENT        },
        { "father",          Relations::PARENT        },
        { "child",           Relations::CHILD         },
        { "children",        Relations::CHILD         },
        { "kid",             Relations::CHILD         },
        { "son",             Relations::CHILD         },
        { "daughter",        Relations::CHILD         },
        { "sibling",         Relations::SIBLING       },
        { "brother",         Relations::SIBLING       },
        { "sister",          Relations::SIBLING       },
        { "aunt",            Relations::AUNTORUNCLE   },
        { "uncle",           Relations::AUNTORUNCLE   },
        { "grandparent",     Relations::GRANDPARENT   },
        { "grandmother",     Relations::GRANDPARENT   },
        { "grandfather",     Relations::GRANDPARENT   },
        { "grandchild",      Relations::GRANDCHILD    },
        { "grandchildren",   Relations::GRANDCHILD    },
        { "grandson",        Relations::GRANDCHILD    },
        { "granddaughter",   Relations::GRANDCHILD    },
        { "cousin",          Relations::COUSIN        },
        { "niece",           Relations::NIECEORNEPHEW },
        { "nephew",          Relations::NIECEORNEPHEW },
        { "descendent",      Relations::DESCENDENT    },
        { "ancestor",        Relations::ANCESTOR      },
        { "relative",        Relations::RELATIVE      }
    };
    const map<Relations, string> RELATIONNAME = 
    {   { Relations::PARENT,        "parent"          },
        { Relations::CHILD,         "child"           },
        { Relations::SIBLING,       "sibling"         },
        { Relations::AUNTORUNCLE,   "aunt/uncle"      },
        { Relations::GRANDPARENT,   "grandparent"     },
        { Relations::GRANDCHILD,    "grandchild"      },
        { Relations::COUSIN,        "cousin"          },
        { Relations::NIECEORNEPHEW, "niece/nephew"    },
        { Relations::DESCENDENT,    "descendent"      },
        { Relations::ANCESTOR,      "ancestor"        },
        { Relations::RELATIVE,      "relative"        }
    };
    const map<string, Modifiers> MODIFIERKEY = 
    {   { "youngest",        Modifiers::YOUNGEST      },
        { "eldest",          Modifiers::OLDEST        },
        { "oldest",          Modifiers::OLDEST        },
        { "younger",         Modifiers::YOUNGER       },
        { "elder",           Modifiers::OLDER         },
        { "older",           Modifiers::OLDER         },
        { "great",           Modifiers::GREAT         },
        { "common",          Modifiers::COMMON        },
        { "living",          Modifiers::LIVING        },
        { "dead",            Modifiers::DEAD          }
    };
    const map<Modifiers, string> MODIFIERNAME = 
    {   { Modifiers::YOUNGEST, "youngest"             },
        { Modifiers::OLDEST,   "oldest"               },
        { Modifiers::YOUNGER,  "younger"              },
        { Modifiers::OLDER,    "older"                },
        { Modifiers::GREAT,    "great"                },
        { Modifiers::COMMON,   "common"               },
        { Modifiers::LIVING,   "living"               },
        { Modifiers::DEAD,     "dead"                 }
    };
    string modString(const Modifiers &m) {
        string result = "";
        for (int a = 1; a < static_cast<int>(Modifiers::MAX); a *= 2) {
            auto it = MODIFIERNAME.find(static_cast<Modifiers>(a));
            if ((static_cast<int>(m) & a) == a) result += it->second + " ";
        }
        return result;
    }
    string ModifiedRelation::print() const {
        string result = "{ ";
        if (relation == Relations::SELF) result += name;
        else {
            result = result + modString(modifier);
            auto it = RELATIONNAME.find(relation);
            if (it == RELATIONNAME.end()) result += "UNKNOWN";
            else result += it->second;
        }
        return result + " }";
    }
    const map<string, Commands> COMMANDKEY =
       {{ "beget",           Commands::ADD            },
        { "had",             Commands::ADD            },
        { "birthed",         Commands::ADD            },
        { "start",           Commands::NEW            },
        { "started",         Commands::NEW            },
        { "print",           Commands::PRINT          },
        { "tell",            Commands::PRINT          },
        { "who",             Commands::QUERY          },
        { "how",             Commands::COUNT          },
        { "what",            Commands::STATS          },
        { "clear",           Commands::CLEAR          },
        { "is",              Commands::TEST           },
        { "are",             Commands::TEST           }
    };
    const map<Commands, string> COMMANDNAME =
    {   { Commands::ADD,     "ADD  "                  },
        { Commands::NEW,     "NEW  "                  },
        { Commands::PRINT,   "PRINT"                  },
        { Commands::QUERY,   "QUERY"                  },
        { Commands::STATS,   "STATS"                  },
        { Commands::CLEAR,   "CLEAR"                  },
        { Commands::TEST,    "TEST "                  }
    };
    enum class Result { OK, NORESULTS, BADQUERY };
    typedef deque<string>           NameQueue; 
    typedef deque<ModifiedRelation> RelationQueue;
    typedef deque<int>              DateQueue;
} // namespace FamilyTree

#endif
