/*
    Representation of a label in amu
    
    A label is a handle to any entity in amu

*/

#ifndef AMU_LABEL_H
#define AMU_LABEL_H

#include "basic/Node.h"
#include "storage/String.h"

namespace amu{

struct Entity;
struct Token;
struct Label {
    TNode node;
    // if this is 0, then what the Label points to must be resolved
    // from node.first_child
    Entity* entity;

    Label* aliased; // if this label is an alias of another label, this is the original
};

struct LabelTable {
    LabelTable* last;
    Map<String, Label*> map;
    TNode* owner; // temp debug so I can figure out who these tables belong to 
};

namespace label {

global Label*
create();

global void
destroy();

global Label*
base(Label* l);

// attempts to resolve a Label from a given Node
// returns 0 if it can't find one 
Label*
resolve(TNode* n);

namespace table {

LabelTable
init(TNode* creator);

FORCE_INLINE void
add(LabelTable* table, String id, Label* l);

} // namespace table

} // namespace label

global void
to_string(DString& start, Label* l);

} //namespace amu

#endif // AMU_LABEL_H