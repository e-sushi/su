/*

    Representation of a statement in amu.

    A statement is a collections of expressions, a defer, a label declaration, or an assignment.

*/


#ifndef AMU_STATEMENT_H
#define AMU_STATEMENT_H

#include "basic/Node.h"

namespace amu {

namespace statement{
// @genstrings(data/statement_strings.generated)
enum kind {
    unknown,
    label,
    defer_,
    expression,
    block_final,
};

#include "data/statement_strings.generated"

} // namespace statement

struct Stmt : public ASTNode {
    statement::kind kind;


    // ~~~~~~ interface ~~~~~~~


    static Stmt*
    create();

    String
    name();

    DString
    debug_str();

    Stmt() : ASTNode(ast::stmt) {}
};

template<> inline b32 ASTNode::
is<Stmt>() { return this->kind == ast::stmt; }

template<> inline b32 ASTNode::
is(statement::kind k) { return this->is<Stmt>() && as<Stmt>()->kind == k; }

void
to_string(DString& start, Stmt* s);

} // namespace amu

#endif // AMU_STATEMENT_H