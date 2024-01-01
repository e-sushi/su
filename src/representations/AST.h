/*

    A wrapper around amu's TNode providing information about what a node represents and utilities for
    querying these things in a way nicer than how I have been doing it before (casting).

    ASTNode is the root of most things in amu, so most things will share the functionality defined here.
    I'm not sure if this will be good or not yet.

*/

#ifndef AMU_AST_H
#define AMU_AST_H

#include "Base.h"

namespace amu {

struct Type;
struct Token;
struct String;

struct ASTNode : public Base {
	enum class Kind {
		Null,
		Entity,
		Stmt,
		Label,
		Code,
	};

    Kind kind;
    Token* start,* end;

	// accessing through the functions is preferred because
	// casting through the template is easier to write/read 
	struct {
		ASTNode* next;
		ASTNode* prev;
		ASTNode* parent;
		ASTNode* first_child;
		ASTNode* last_child;
	}link;

	u32 child_count;

    struct {
        b32 break_air_gen : 1 = false;
    } flags;

    ASTNode(Kind k) : kind(k), Base(Base::Kind::AST) {
        link.parent =
        link.next =
        link.prev = 
        link.first_child = 
        link.last_child = 0;
        child_count = 0;
    }

	// inserts x after this node in its sibling list
	inline void insert_after(ASTNode* x);
	// inserts x before this node in its sibling list
	inline void insert_before(ASTNode* x);
	// inserts x at the end of this nodes child list
	inline void insert_last(ASTNode* x);
	// inserts x at the front of this nodes child list
	inline void insert_first(ASTNode* x);
	// removes this node from the siblings list
	inline void remove_horizontally();
	// inserts this node above x
	inline void insert_above(ASTNode* x);
	// changes the parent of this node to x
	// can be null
	inline void change_parent(ASTNode* x);
	// moves this node to be the first in its parents child list
	inline void move_to_parent_first();
	// moves this node to be the last in its parents child list
	inline void move_to_parent_last();
	// completely removes this node from the tree
	inline void remove();

 	// gets the next sibling of this ASTNode, 
    // optionally passing a type to cast to 
    template<typename T = ASTNode> inline T*&
    next() { return (T*&)link.next; }

    // similar to 'is' but for the next node
    template<typename T> inline b32
    next_is() { return next() && next()->is<T>(); }

    // gets the previous sibling of this ASTNode, 
    // optionally passing a type to cast to 
    template<typename T = ASTNode> inline T*&
    prev() { return (T*&)link.prev; }

    // gets the parent of this ASTNode, 
    // optionally passing a type to cast to 
    template<typename T = ASTNode> inline T*&
    parent() { return (T*&)link.parent; }

    // gets the first_child of this ASTNode, 
    // optionally passing a type to cast to 
    template<typename T = ASTNode> inline T*&
    first_child() { return (T*&)link.first_child; }

    // gets the last_child of this ASTNode, 
    // optionally passing a type to cast to 
    template<typename T = ASTNode> inline T*&
    last_child() { return (T*&)link.last_child; }

    // Attempts to resolve the AST node into a Type 
    // by default this returns 0, but children that 
    // have some Type or can discern a type from one 
    // of its children should implement this.
    // This is primarily for finding a Type when you don't care
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
is<ASTNode>() { return kind == Base::Kind::AST; }

} // namespace amu

#endif // AMU_AST_H
