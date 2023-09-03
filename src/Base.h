/*

    Base object of most things in amu so that we can guarantee that certain basic
    functionality is implemented for everything, such as printing a name or dumping
    debug information.

    This is just an idea that I've yet to implement. I would like this to be implemented
    on everything so that everything may be inspected in a uniform manner, but the fact
    that it introduces a vtable onto whatever uses it bother me. For instance, I wanted
    Register in Gen.h to use this, but that would make Register no longer only 8 bytes.

    This can be implemented for things like the AST, TAC, and the various stages, though.

*/

#ifndef AMU_BASE_H
#define AMU_BASE_H

namespace amu {

struct Base {

    // return a generated name for this object 
    virtual DString
    name() = 0;

    // outputs debug information about this object
    virtual DString
    dump() = 0;


    // performs a cast of this Base object
    template<typename T> inline T*
    as() { return (T*)this; }

    // base template meant to be instantiated by children
    // that determines how to check if they are a given type
    // useful for layered types so that you don't have to write
    // so much to check if something is, say, a Block
    template<typename T> inline b32
    is();

    template<typename... T> inline b32
    is_any() { return (is<T>() || ...); }

    template<typename T> inline b32
    is_not() { return !is<T>(); }

    template<typename T> inline b32
    is(T x);

    template<typename... T> inline b32
    is_any(T... x) { return (is(x) || ...); }

    template<typename T> inline b32
    is_not(T x) { return !is(x); }
};


} // namespace amu 

#endif // AMU_BASE_H