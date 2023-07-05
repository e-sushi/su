/*

    Representation of a statement in amu.

    A statement is a collections of expressions, a defer, a label declaration, or an assignment.

*/


#ifndef AMU_STATEMENT_H
#define AMU_STATEMENT_H

#include "basic/Node.h"

namespace amu {

struct Statement {
    TNode node;
    Type type;
};

namespace statement {

enum StatementType : u32 {
    Label,
    Assignment,
    Defer,
    Expression
};

} // namespace statement

} // namespace amu

#endif // AMU_STATEMENT_H