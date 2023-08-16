/*

    Internal representation of a type in amu, an attribute of expressions, but can also be expressions themselves

    TODOs
        Currently a unique Type is created regardless of if that Type already exists
        for example, if we have u32* in two places, there are four Types representing these
        We need a way to store and index types so that we may reuse them, especially when generics 
        start to get implemented.
        
        This whole structure may be redundent, we can probably get the same effect by just storing an extra TNode
        on Structure and handling what we handle here there.

*/

// #ifndef AMU_TYPE_H
// #define AMU_TYPE_H

// #include "Token.h"

// namespace amu {


// void
// to_string(DString& start, Type* t);

// namespace util{
// template<> FORCE_INLINE u64 
// hash(const Type& t) {

// }
// template<> FORCE_INLINE u64 hash(Type* t) {return hash(*t);}
// }

// } // namespace amu

// #endif // AMU_TYPE_H