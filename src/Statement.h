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
    expression
};

#include "data/statement_strings.generated"

} // namespace statement

struct Statement {
    TNode node;
    statement::kind kind;
};

namespace statement {

global Statement*
create();

global void
destroy(Statement& s);

} // namespace statement

void
to_string(DString& start, Statement* s);

} // namespace amu

#endif // AMU_STATEMENT_H