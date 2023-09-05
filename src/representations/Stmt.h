/*

    Representation of a statement in amu.

    A statement is a collections of expressions, a defer, a label declaration, or an assignment.

*/


#ifndef AMU_STATEMENT_H
#define AMU_STATEMENT_H

#include "basic/Node.h"

namespace amu {

namespace stmt{
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
    stmt::kind kind;


    // ~~~~~~ interface ~~~~~~~


    static Stmt*
    create();

    void
    destroy();

    DString*
    name();

    DString*
    dump();

    Stmt() : ASTNode(ast::stmt) {}
};

template<> inline b32 Base::
is<Stmt>() { return is<ASTNode>() && as<ASTNode>()->kind == ast::stmt; }

template<> inline b32 Base::
is(stmt::kind k) { return is<Stmt>() && as<Stmt>()->kind == k; }

void
to_string(DString*& start, Stmt* s);

} // namespace amu

#endif // AMU_STATEMENT_H