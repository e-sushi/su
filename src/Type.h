/*

    Internal representation of a type in amu, an attribute of expressions, but can also be expressions themselves

*/

#ifndef AMU_TYPE_H
#define AMU_TYPE_H

#include "Token.h"

namespace amu {

struct Structure;
struct Type {
    // if this is 0, then the type is a pointer 
    Structure* structure; // the base struct of this type
    // when a type is decorated with pointers or arrays, each level of decoration
    // is its own type which points back to the type that it decorates, for example
    // u32*[2] is three Types, u32, u32*, and u32*[2] and these point at each other like so:
    // u32*[2] -> u32* -> u32
    Type* indirection; 
};



namespace type {

struct key{
    String base_type_id;
    Array<token::kind> decorators;
};

template<typename... T> key
make_key(String identifier, T... decorators) {
    key out;
    out.base_type_id = identifier;
    out.decorators = array::init<token::kind>(sizeof...(decorators));
    (array::push(out.decorators, decorators), ...);
}

template<typename... T> u64 
hash(String identifier, T... indirections) {
    u64 id_hash = string::hash(identifier);
}

};


} // namespace amu

#endif // AMU_TYPE_H