/*

    Representation of a statement in amu.

    A statement is a collections of expressions, a defer, a label declaration, or an assignment.

*/


#ifndef AMU_STATEMENT_H
#define AMU_STATEMENT_H

#include "basic/Node.h"

namespace amu {

namespace statement{
enum kind {
    label,
    assignment,
    defer_,
    expression
};
} // namespace statement

struct Statement {
    TNode node;
    statement::kind kind;
};

} // namespace amu

#endif // AMU_STATEMENT_H