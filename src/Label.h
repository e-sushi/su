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

    Label* original; // if this label is an alias of another label, this is the original
};

} //namespace amu

#endif // AMU_LABEL_H