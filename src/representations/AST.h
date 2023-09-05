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


struct ASTNode : public TNode, public Base {
    ast::kind kind;
    Token* start,* end;

    struct {
        b32 break_air_gen : 1 = false;
    } flags;

    ASTNode(ast::kind k) : kind(k), Base(base::ast) {
        this->TNode::parent =
        this->TNode::next =
        this->TNode::prev = 
        this->TNode::first_child = 
        this->TNode::last_child = 0;
        this->TNode::child_count = 0;
    }

     // gets the next sibling of this ASTNode, 
    // optionally passing a type to cast to 
    template<typename T = ASTNode> inline T*
    next() { return (T*)TNode::next; }

    // similar to 'is' but for the next node
    template<typename T> inline b32
    next_is() { return next() && next()->is<T>(); }

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

    // replace this ASTNode with 'n'
    void
    replace(ASTNode* n);

    template<void (*callback)(DString&, ASTNode*)> DString
    print_tree(b32 newlines = true);

    DString
    print_tree(b32 newlines = true);

    // returns a String encompassing the first line
    // this this node covers
    String
    first_line(b32 line_numbers = false, b32 remove_leading_whitespace = false);

    // returns a String encompassing all the lines that 
    // this node and its children cover 
    String
    lines();

    // returns a DString of the line representing the current node
    // as well as an underline of the contents of the node
    // TODO(sushi) this currenly only works with space indentation
    DString
    underline();
};

template<> b32 inline Base::
is<ASTNode>() { return kind == base::ast; }


} // namespace amu

#endif // AMU_AST_H