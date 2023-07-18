/*

    amu's Node implementation

    This is functionally the exact same as kigu's nodes, but I have decided that I would like to
    implement them locally in case there ever comes a case where we need them to be thread safe. If 
    that ever happens, I would rather not replace all usages of node throughout amu with a new
    interface, we can just change this one.

    !!!!!!!!!!!!!!! IMPORTANT !!!!!!!!!!!!!!!!!
     if it comes to needing to make nodes thread safe, we have to rewrite everything that uses them 
     to call deinit on all possible nodes they could have created!

     for example Pool<T> cannot just memzfree all of its arenas, because they would contain 
     Nodes with initialized mutexes!

*/


#ifndef AMU_NODE_H
#define AMU_NODE_H

#include "storage/String.h"

namespace amu{
struct Token;

// a node for linked lists
// this actually a circularly linked list, eg. next and prev begin by pointing at each other
struct LNode {
    //deshi::mutex lock;

    LNode* next;
    LNode* prev;
};

namespace node {
// @genstrings(data/node_strings.generated)
enum kind {
    label,
    place,
    structure,
    function,
    module,
    statement,
    expression,
    tuple,
    type,
};
} // namespace node::type

// a Node for trees 
struct TNode {
    //deshi::mutex lock;

    node::kind kind;

    Token* start;
    Token* end;

    TNode* next;
    TNode* prev;
	TNode* parent;
	TNode* first_child;
	TNode* last_child;
	u32   child_count;
};

#include "data/node_strings.generated"

namespace node {



global inline void
init(LNode* node);

global inline void
init(TNode* node);

global inline void
deinit(TNode* node);

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
remove(TNode* node);

} // namespace node
} // namespace amu

#endif // AMU_NODE_H