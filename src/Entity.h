/*
    An Entity represents any thing that a label may be attached to and the way a label 
    may be used is dependent upon its entity.
*/

#ifndef AMU_ENTITY_H
#define AMU_ENTITY_H

#include "Node.h"
#include "Pool.h"
#include "Label.h"



namespace amu{

struct Entity {
    Node node;
    Type type;

    struct Variable {
        b32 initialized;
        u32 pointer_depth;

    };

    struct Function {

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