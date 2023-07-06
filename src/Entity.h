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

namespace entity{
enum Type : u32 {
    unknown,
    variable,
    function,
    structure,
    module,
    loop,
};
} // namespace entity

struct Entity {
    TNode node;
    entity::Type type;

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



} // namespace entity
} // namespace amu

#endif // AMU_ENTITY_H