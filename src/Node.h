/*
    Thread safe nodes

    These nodes ensure that multiple threads do not manipulate them at the same time
    
    I am not totally sure if this is how I should go about this, or if I should just 
    design systems to prevent the same node structures from ever being accessed.
    I'm not sure atm if this is possible, so to stay safe I will do it this way for now.

    Currently a mutex is stored per node, which means that every node must be initialized,
    but we may want to experiment later with a Forest approach, such that a mutex is stored
    for groups of nodes, but this prevents operations on the entire node structure, rather
    than allowing different parts to be manipulated at the same time, but this feature may
    also be bad.

*/


#ifndef AMU_NODE_H
#define AMU_NODE_H

#include "kigu/common.h"
namespace deshi {
    #include "core/threading.h"
}

namespace amu{

// a node for linked lists
// this actually a circularly linked list, eg. next and prev begin by pointing at each other
struct LNode {
    deshi::mutex lock;

    LNode* next;
    LNode* prev;
};

// a Node for trees 
struct TNode {
    deshi::mutex lock;

    Type type;
    Flags flags;

    TNode* next;
    TNode* prev;
	TNode* parent;
	TNode* first_child;
	TNode* last_child;
	u32   child_count;
};

namespace node {

global inline void
init(LNode* node);

global inline void
init(TNode* node);

global inline void
insert_after(LNode* target, LNode* node);

global inline void
insert_before(LNode* target, LNode* node);

global inline void
insert_after(TNode* target, TNode* node);

global inline void
insert_before(TNode* target, TNode* node);

global inline void 
remove_horizontally(TNode* node);

// inserts the given 'node' as the last child of the node 'parent'
global inline void
insert_last(TNode* parent, TNode* node);

// inserts the given 'node' as the first child of the node 'parent'
global inline void 
insert_first(TNode* parent, TNode* node);

global inline void
change_parent(TNode* new_parent, TNode* node);

// rearrages the parent of 'node' such that 'node' is the first child, 
// moving all other children nodes to the right
global inline void
move_to_parent_first(TNode* node);

// rearrages the parent of 'node' such that 'node' is the last child, 
// moving all other children nodes to the left
global inline void
move_to_parent_last(TNode* node);

global inline void 
remove(LNode* node);

global inline void
remove(LNode* node);

} // namespace node
} // namespace amu

#endif // AMU_NODE_H