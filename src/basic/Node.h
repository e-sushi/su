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


    Nodes that actually represent pieces of the AST are implemented in AST.h/cpp
    and provide an interface for interacting with the AST beyond what is given here.

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
// NOTE(sushi) instead of just storing that a node could be an Entity
//             and having to access what kind of Entity it is by first casting to Entity
//             we just store Entity tags on the Node itself, since, so far, there has been
//             no reason to access just the data held by Entity


} // namespace node::type

// a Node for trees 
struct TNode {
    TNode* next = 0;
    TNode* prev = 0;
	TNode* parent = 0;
	TNode* first_child = 0;
	TNode* last_child = 0;
	u32    child_count = 0;
};

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

template<typename A, typename B> FORCE_INLINE void
insert_after(A* target, B* node) { insert_after((TNode*)target, (TNode*)node); }

global inline void
insert_before(TNode* target, TNode* node);

template<typename A, typename B> FORCE_INLINE void 
insert_before(A* target, B* node) { insert_before((TNode*)target, (TNode*)node); }

global inline void 
remove_horizontally(TNode* node);

template<typename T> FORCE_INLINE void 
remove_horizontally(T* node) { remove_horizontally((TNode*)node); }

// inserts the given 'node' as the last child of the node 'parent'
global inline void
insert_last(TNode* parent, TNode* node);

template<typename A, typename B> FORCE_INLINE void 
insert_last(A* target, B* node) { insert_last((TNode*)target, (TNode*)node); }

// inserts the given 'node' as the first child of the node 'parent'
global inline void 
insert_first(TNode* parent, TNode* node);

template<typename A, typename B> FORCE_INLINE void 
insert_first(A* target, B* node) { insert_first((TNode*)target, (TNode*)node); }

global inline void
insert_above(TNode* below, TNode* above);

template<typename A, typename B> FORCE_INLINE void 
insert_above(A* target, B* node) { insert_above((TNode*)target, (TNode*)node); }

global inline void
change_parent(TNode* new_parent, TNode* node);

template<typename A, typename B> FORCE_INLINE void 
change_parent(A* target, B* node) { change_parent((TNode*)target, (TNode*)node); }

// rearrages the parent of 'node' such that 'node' is the first child, 
// moving all other children nodes to the right
global inline void
move_to_parent_first(TNode* node);

template<typename T> FORCE_INLINE void 
move_to_parent_first(T* node) { move_to_parent_first((TNode*)node); }

// rearrages the parent of 'node' such that 'node' is the last child, 
// moving all other children nodes to the left
global inline void
move_to_parent_last(TNode* node);

template<typename T> FORCE_INLINE void
move_to_parent_last(T* node) { move_to_parent_last((TNode*)node); }

global inline void 
remove(LNode* node);

global inline void
remove(TNode* node);

template<typename T> FORCE_INLINE void
remove(T* node) { remove((TNode*)node); }

namespace util {
    
// print a given tree
// the callback provided is called on each node and expects a String returned to represent it
template<void (*callback)(DString*&, TNode*)> DString*
print_tree(TNode* root, b32 newlines = true);

DString*
print_tree(TNode* root, b32 newlines = true);

} // namespace util
} // namespace node

// if 'expand' is true, it will resolve the TNode to what it represents, then return that string
void
to_string(DString*& start, TNode* n, b32 expand = false);

DString*
to_string(TNode* n, b32 expand = false);

} // namespace amu

#endif // AMU_NODE_H