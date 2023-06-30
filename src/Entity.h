/*
    An Entity represents any thing that a label may be attached to and the way a label 
    may be used is dependent upon its entity.
*/

#ifndef AMU_ENTITY_H
#define AMU_ENTITY_H

#include "basic/Node.h"
#include "storage/Pool.h"
#include "Label.h"

namespace amu{

struct Entity {
    Node node;
    Type type;

    struct Variable {
        b32 initialized;
        u32 pointer_depth;

    };

    struct Structure {

    };

    struct Function {

    };

    struct Module {

    };  

    union {
        Variable variable;
        Structure structure;
        Function function;
        Module module;
    };

    Label* label; 

};


namespace entity{

enum EntityType : u32 {
    Unknown,
    Variable,
    Function,
    Structure,
    Module,
    Loop,
};

// initialize an Entity of some given type, returning a pointer to it
global Entity* 
init(EntityType type);

global void
deinit(Entity* entity);


} // namespace entity
} // namespace amu

#endif // AMU_ENTITY_H