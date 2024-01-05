/*

    Function structure and utilities for interacting with it, as well as structures related to functions.

*/

#ifndef AMU_FUNCTION_H
#define AMU_FUNCTION_H

#include "Entity.h"
#include "Type.h"
#include "Expr.h"
#include "Frame.h"

namespace amu {

struct FunctionType;
// TODO(sushi) the AST for Functions is stupid atm. This doesn't even appear in the AST
//             its Label does. 
struct Function : public Entity {
    FunctionType* type;
    Frame frame;

    static Function*
    create(FunctionType* type = 0);

    void
    destroy();

    DString
    display();

    DString
    dump();

    Function() : Entity(Entity::Kind::Func) {}
};

template<> inline b32 Base::
is<Function>() { return is<Expr>() && as<Expr>()->kind == Expr::Kind::Function; }

// when a label is assigned to a second function entity, this is created
// and the label points at it instead of any of the Functions
struct OverloadedFunction : public Entity {
    Array<Function*> overloads;
};

void
to_string(DString* start, Function* p);

DString*
to_string(Function* f) {
    DString* out = DString::create();
    to_string(out, f);
    return out;
}

} // namespace amu 

#endif // AMU_FUNCTION_H