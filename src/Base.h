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

#include "Common.h"

// for use on inheriting structures so that we can 
// create overrides to perform direct checks 
#define IS_TEMPLATE_DECLS                   \
	template<typename T> inline b32 is();   \
	template<typename T> inline b32 is(T x);

#define IS_TEMPLATE_DEF(parent, base, kindval)                 \
	template<> inline b32 Base::                                         \
	is<base>() { return is<parent>() && as<parent>()->kind == kindval; } \
	template<> inline b32 parent::                                       \
	is<base>() { return kind == kindval; }          

namespace amu {

struct DString;

struct Base {
	enum class Kind {
		Entity,
		Expr,
		Stmt,
		TAC,
		ScalarValue,
		AST,
	};

    Kind kind;

    // return a generated name for this object 
    virtual DString
    display() = 0;

    // outputs debug information about this object
    virtual DString
    dump() = 0;

    // performs a cast of this Base object
    template<typename T> inline T*
    as() { return (T*)this; }

    template<typename T> b32
    is();

    template<typename... T> inline b32
    is_any() { return (is<T>() || ...); }

    template<typename T> inline b32
    is_not() { return !is<T>(); }

    template<typename T> b32
    is(T x);

    template<typename... T> inline b32
    is_any(T... x) { return (is(x) || ...); }

    template<typename T> inline b32
    is_not(T x) { return !is(x); }

    Base(Kind k) : kind(k) {}
};


} // namespace amu 

#endif // AMU_BASE_H
