/*

    A wrapper around amu's TNode providing information about what a node represents and utilities for
    querying these things in a way nicer than how I have been doing it before (casting).

    ASTNode is the root of most things in amu, so most things will share the functionality defined here.
    I'm not sure if this will be good or not yet.

*/

#ifndef AMU_AST_H
#define AMU_AST_H

namespace amu {

struct Type;

namespace ast {
// @genstrings(data/astnode_strings.generated)
enum kind {
    null,
    code,
    label,
    entity,
    stmt,
    tuple,
};

#include "data/astnode_strings.generated"

} // namespace ast 


struct ASTNode : public TNode {
    ast::kind kind;
    Token* start,* end;

    ASTNode(ast::kind k) : kind(k) {
        memory::zero(this, sizeof(TNode));
    }

    // a user-friendly name for whatever this ASTNode represents 
    virtual String
    name() = 0;

    // a more technical string giving more information about what this 
    // ASTNode represents
    virtual DString
    debug_str() = 0;

    // returns a formatted DString
    // TODO(sushi) consistent way to setup formatting options
    // virtual DString
    // display() = 0;

    // casts the ASTNode to the given Type
    // of course you can just use plain C casts, but I plan to use this just in case
    // there ever comes a time where we can't have ASTNode has the ultimate root 
    // for some reason. Before this I was using C casts *everywhere* and the formatting
    // job if that day comes would be have been a nightmare.
    template<typename T> inline T*
    as() { return (T*)this; }

    // test that the ASTNode is a given type
    // this base declaration is never implemented and children of ASTNode are 
    // expected to implement this for their types
    template<typename T> inline b32
    is();

    template<typename T> inline b32
    is(T x);

    // attempts to resolve the AST node into a Type 
    // by default this returns 0, but children that 
    // have some Type or can discern a type from one 
    // of its children should implement this 
    // this is primarily for finding a Type when you don't care
    // about what that Type comes from, like with printing
    // This shouldn't be used in any real logic. Do NOT use this to 
    // check if an ASTNode HAS a type, because things like Label and Statement
    // which are not typed will return the Type of the expressions they wrap.
    virtual Type*
    resolve_type() { return 0; }

    // gets the next sibling of this ASTNode, 
    // optionally passing a type to cast to 
    template<typename T = ASTNode> inline T*
    next() { return (T*)TNode::next; }

    // similar to 'is' but for the next node
    template<typename T> inline b32
    next_is();

    // gets the previous sibling of this ASTNode, 
    // optionally passing a type to cast to 
    template<typename T = ASTNode> inline T*
    prev() { return (T*)TNode::prev; }

    // gets the parent of this ASTNode, 
    // optionally passing a type to cast to 
    template<typename T = ASTNode> inline T*
    parent() { return (T*)TNode::parent; }

    // gets the first_child of this ASTNode, 
    // optionally passing a type to cast to 
    template<typename T = ASTNode> inline T*
    first_child() { return (T*)TNode::first_child; }

    // gets the last_child of this ASTNode, 
    // optionally passing a type to cast to 
    template<typename T = ASTNode> inline T*
    last_child() { return (T*)TNode::last_child; }

    template<void (*callback)(DString&, ASTNode*)> DString
    print_tree(b32 newlines = true);

    DString
    print_tree(b32 newlines = true);
};




} // namespace amu

#endif // AMU_AST_H