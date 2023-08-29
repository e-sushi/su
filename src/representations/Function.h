/*

    Function structure and utilities for interacting with it, as well as structures related to functions.

*/

#ifndef AMU_FUNCTION_H
#define AMU_FUNCTION_H

#include "Entity.h"
#include "Type.h"

namespace amu {

struct FunctionType;

struct Function : public Entity {
    FunctionType* type;

    static Function*
    create(FunctionType* type = 0);

    void
    destroy();
};

// when a label is assigned to a second function entity, this is created
// and the label points at it instead of any of the Functions
struct OverloadedFunction : public Entity {
    Array<Function*> overloads;
};

void
to_string(DString& start, Function* p);

DString
to_string(Function* f) {
    DString out = dstring::init();
    to_string(out, f);
    return out;
}

} // namespace amu 

#endif // AMU_FUNCTION_H