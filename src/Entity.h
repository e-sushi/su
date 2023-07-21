/*
    An Entity represents any thing that a label may be attached to and the way a label 
    may be used is dependent upon its entity.
*/

#ifndef AMU_ENTITY_H
#define AMU_ENTITY_H

#include "basic/Node.h"
#include "storage/Pool.h"
#include "Label.h"
#include "Type.h"

namespace amu{

// representation of something that has a place in memory, name borrowed from rust
struct Place {
    TNode node;

    // type information for this place in memory
    Type type;
};

struct Structure {
    TNode node;
    u64 size; // size of this structure in bytes
    Map<String, Structure*> members;
};

struct Function {
    TNode node;
};

struct Module {
    TNode node;

    Array<spt> labels; 

    LabelTable table;    
};

} // namespace amu

#endif // AMU_ENTITY_H