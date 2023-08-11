/*

    Internal representation of a type in amu, an attribute of expressions, but can also be expressions themselves
    This represents a fully 
*/

#ifndef AMU_TYPE_H
#define AMU_TYPE_H

#include "Token.h"

namespace amu {

namespace type {} // namespace type

struct Structure;
struct Type {
    // a Type may have children which serve to represent parameters 
    // these parameters are those found on structs, modules, and functions
    TNode node;
    Structure* structure; // the base struct of this type
};

namespace type {


global Type*
create();

global void
destroy(Type& t);

// adds a layer of indirection to the type and returns the new layer
// this creates a new Type
Type*
add_indirection(Type* type, Structure* s);

} // namespace type
} // namespace amu

#endif // AMU_TYPE_H