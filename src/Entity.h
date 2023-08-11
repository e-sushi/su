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
#include <source_location>

namespace amu{

struct Entity {
    TNode node;
    Label* label; // the most recent label used to represent this entity
};

// representation of something that has a place in memory, name borrowed from rust
struct Place : public Entity {
    // type information for this place in memory
    Type* type;
};

namespace place {

global Place*
create();

global void
destroy(Place& p);

} // namespace place

struct Structure : public Entity {
    u64 size; // size of this structure in bytes
    Map<String, Structure*> members;
};

namespace structure {

global Structure*
create();

global void
destroy(Structure& s);

global b32
is_builtin(Structure* s);

} // namespace structure

struct Function : public Entity {
};

namespace function {

global Function*
create();

global void
destroy(Function& f);

} // namespace function

struct Module : public Entity {
    Array<spt> labels; 

    LabelTable table;    
};

namespace module {

global Module*
create();

global void
destroy(Module& m);

} // namespace module

namespace entity {

// retrieves the label that was originally used when declaring this entity
global Label*
declared_label(Function* f);

global Label*
declared_label(Structure* f);

global Label*
declared_label(Module* f);

global Label*
declared_label(Place* f);

} // namespace entity

void
to_string(DString& start, Place* p);

void
to_string(DString& start, Function* p);

void
to_string(DString& start, Module* p);

void
to_string(DString& start, Structure* p);

} // namespace amu

#endif // AMU_ENTITY_H