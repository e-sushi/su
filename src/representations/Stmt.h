/*

    Representation of a statement in amu.

    A statement is a collections of expressions, a defer, a label declaration, or an assignment.

*/


#ifndef AMU_STATEMENT_H
#define AMU_STATEMENT_H

#include "basic/Node.h"

namespace amu {

struct Stmt : public ASTNode {
	enum class Kind {
		Unknown,
		Label,
		Defer,
		Expr,
		BlockFinal,
	};

    Kind kind;


    // ~~~~~~ interface ~~~~~~~


    static Stmt*
    create();

    void
    destroy();

    DString*
    display();

    DString*
    dump();

    Stmt() : ASTNode(ASTNode::Kind::Stmt) {}
};

void
to_string(DString& start, Stmt* s);

} // namespace amu

#endif // AMU_STATEMENT_H
