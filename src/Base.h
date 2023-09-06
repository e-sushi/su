/*

    Base object of most things in amu so that we can guarantee that certain basic
    functionality is implemented for everything, such as printing a name or dumping
    debug information.

    This seems to work fine, but it's so OOP it kind of disgusts me. I will keep it for now
    but I think later on it might be beneficial to remove it.

    Keep in mind that nearly everything this defines should primarily be used only in cases
    where we need to report something, such as a name or debug dump. 


    The whole 'is' thing is pretty scuffed. I implemented it when the amount of layered stuff wasn't
    very high, but now it's gotten up there and so sometimes we are doing like 3-4 checks to figure 
    something out. This needs to just be an aggregate of all things that inherit it at any level
    and 'is' just checks for that kind directly. I will do this later when amu becomes somewhat stable,
    if this system is even still being used by then.

*/

#ifndef AMU_BASE_H
#define AMU_BASE_H

namespace amu {

namespace base {
enum kind {
    ast,
    literal,
    tac,
    scalar_value,
};
} // namespace base

struct Base {
    base::kind kind;


    // return a generated name for this object 
    virtual DString*
    display() = 0;

    // outputs debug information about this object
    virtual DString*
    dump() = 0;


    // performs a cast of this Base object
    template<typename T> FORCE_INLINE T*
    as() { return (T*)this; }

    /* 
        The following are small utilities for figuring out what something is
        This is primarily for ASTNode, which is where it was originally implemented.
        Since some parts of the AST are layered now, I found myself writing stuff like:
            
            if(n->kind == node::entity && ((Entity*)n)->kind == entity::expr && ((Expr*)n)->kind == ...)
        
        The following allow you to compress something like this into 

            if(n->is(...)) 
            or
            if(n->is<...>()) 

        If this Base thing is ever removed, we should probably just have node kinds store
        everything they can be in a single enum, so it can be done in one check.

        These are FORCE_INLINED because they are so small and I don't want to waste time
        entering a function to do these checks
    */

    template<typename T> FORCE_INLINE b32
    is();

    template<typename... T> FORCE_INLINE b32
    is_any() { return (is<T>() || ...); }

    template<typename T> FORCE_INLINE b32
    is_not() { return !is<T>(); }

    template<typename T> FORCE_INLINE b32
    is(T x);

    template<typename... T> FORCE_INLINE b32
    is_any(T... x) { return (is(x) || ...); }

    template<typename T> FORCE_INLINE b32
    is_not(T x) { return !is(x); }

    Base(base::kind k) : kind(k) {}
};


} // namespace amu 

#endif // AMU_BASE_H