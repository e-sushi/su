/*

    Internal representation of a type in amu, an attribute of expressions, but can also be expressions themselves

*/

#ifndef AMU_TYPE_H
#define AMU_TYPE_H

#include "Token.h"

namespace amu {

struct Structure;
struct Type {
    // a Type may have children to represent parameters 
    // these parameters are those found on structs, modules, and functions
    TNode node;
    Structure* structure; // the base struct of this type
};

namespace type {

global Type*
create();

global void
destroy(Type& t);

global Type*
base(Type& t);

} // namespace type

void
to_string(DString& start, Type* t);

} // namespace amu

#endif // AMU_TYPE_H