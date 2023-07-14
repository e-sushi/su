/*
    Representation of a label in amu
    
    A label is a handle to any entity in amu

*/

#ifndef AMU_LABEL_H
#define AMU_LABEL_H

#include "basic/Node.h"
#include "storage/String.h"

namespace amu{
    
struct Token;
struct Label {
    TNode node;
    Token* token; // token representing this Label

    TNode* entity;

    Label* original; // if this label is an alias of another label, this is the original
};

// a special kind of map that points back to previous maps
// so that we may store scoped labels
struct LabelTable {
    LabelTable* last;
    Map<String*, TNode*> table;
};

} //namespace amu

#endif // AMU_LABEL_H