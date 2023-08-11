/*

    Internal representation of a type in amu, an attribute of expressions, but can also be expressions themselves
    This represents a fully 
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

global DString
chain_to_base_str(Type& t);

} // namespace type
} // namespace amu

#endif // AMU_TYPE_H