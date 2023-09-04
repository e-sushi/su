/*

    Structures for representing various kind of type in amu and an interface for interacting with them.

*/

#ifndef AMU_TYPE_H
#define AMU_TYPE_H

// #include "Token.h"

#include "Entity.h"

namespace amu {

struct Function;
struct Member;

namespace type {
enum class kind {
    void_, // the type representing nothing 
    whatever, // when we might need a type but we don't care what it would be 
    scalar,
    structured,
    pointer,
    function,
    tuple,
    meta,
};
} // namespace type

// base structure of all types, though this is not meant to be created directly
struct Type : public Entity {
    type::kind kind;
    // set of traits applied to this Type
    Array<Trait*> traits;

    // Function objects that define this type as its first parameter
    Array<Function*> methods;

    // Function objects that define a typeref to this Type as its first parameter
    Array<Function*> static_methods;


    // ~~~~~~ interface ~~~~~~ 


    // handles all builtin type coersion and detecting if a type has a user defined 
    // conversion to another
    b32
    can_cast_to(Type* to);

    u64
    hash();

    b32
    has_trait(String name);

    b32
    has_trait(Trait* trait);

    b32
    is_scalar() { return this->kind == type::kind::scalar; }

    // attempts to find the size of a given Type in bytes
    virtual u64
    size() = 0;

    String
    name() = 0;

    DString
    debug_str() = 0;

    Type(type::kind k) : kind(k), Entity(entity::type) {}
};

template<> inline b32 ASTNode::
is<Type>() { return is<Entity>() && as<Entity>()->kind == entity::type; }

template<> inline b32 ASTNode::
next_is<Type>() { return next() && next()->is<Type>(); }

// type representing nothing 
struct Void : public Type { 
    Void() : Type(type::kind::void_) {} 

    u64     size() { return 0; }
    String  name() { return "void"; }
    DString debug_str() { return dstring::init("Void<>"); }
};

template<> inline b32 ASTNode::
is<Void>() { return is<Type>() && as<Type>()->kind == type::kind::void_; }

namespace type{
global Void void_;
} // namespace type

// i dont know if this is particularly useful, I'm only using it to solve
// a specific case:
// if(...) break else ...
// 'break' is an expression, and needs a type, so I just say it is Whatever
// because it isn't going to actually return anything, it just controls flow
// Any type may implicitly convert to this, so it's really just a wildcard
// Though it is an error if you try to use this as a value in any way
// This is extremely experimental and I figure it will be confusing
// so it will probably be removed eventually 
struct Whatever : public Type {
    Whatever() : Type(type::kind::whatever) {}

    u64 size() { return 0; }
    String name() { return "whatever"; }
    DString debug_str() { return dstring::init("Whatever<>"); }
};

template<> inline b32 ASTNode::
is<Whatever>() { return is<Type>() && as<Type>()->kind == type::kind::whatever; }

namespace type{
global Whatever whatever;
} // namespace type

namespace scalar {
enum kind {
    unsigned8,
    unsigned16,
    unsigned32,
    unsigned64,
    signed8,
    signed16,
    signed32,
    signed64,
    float32,
    float64,
};
} // namespace type::builtin

// a numerical type
struct Scalar : public Type {
    scalar::kind kind;
    Scalar(scalar::kind kind) : Type(type::kind::scalar) {
        this->kind = kind;
    }


    // ~~~~~~ interface ~~~~~~~


    String
    name();

    DString
    debug_str();

    u64
    size();
};

template<> b32 inline ASTNode::
is<Scalar>() { return is<Type>() && as<Type>()->kind == type::kind::scalar; }

template<> b32 inline ASTNode::
next_is<Scalar>() { return next() && next()->is<Scalar>(); }

template<> b32 inline ASTNode::
is(scalar::kind k) { return is<Scalar>() && as<Scalar>()->kind == k; };

namespace scalar {
global Scalar scalars[] = {
    {scalar::unsigned8},
    {scalar::unsigned16},
    {scalar::unsigned32},
    {scalar::unsigned64},
    {scalar::signed8},
    {scalar::signed16},
    {scalar::signed32},
    {scalar::signed64},
    {scalar::float32},
    {scalar::float64},
};
} // namespace type::scalar

namespace structured {
enum kind {
    user, // a user defined Structured Type
    static_array,
    view_array,
    dynamic_array,
};
} // namespace structured

// a Type which is defined by a user-defined structure
struct Structured : public Type {
    structured::kind kind;

    Structure* structure;


    // ~~~~~~ interface ~~~~~~~


    static Structured*
    create(Structure* structure);

    Member*
    find_member(String id);

    String
    name();

    DString
    debug_str();

    u64
    size();

    Structured() : Type(type::kind::structured) {}

    Structured(structured::kind k) : kind(k), Type(type::kind::structured) {}
};

template<> inline b32 ASTNode::
is<Structured>() { return is<Type>() && as<Type>()->kind == type::kind::structured; }

template<> inline b32 ASTNode::
is(structured::kind k) { return is<Structured>() && as<Structured>()->kind == k; }

// a Type representing an address in memory where a value of 'type' can be found 
struct Pointer : public Type {
    Type* type;

    static Array<Pointer*> set;


    // ~~~~~~ interface ~~~~~~~


    static Pointer*
    create(Type* type);

    String
    name();

    DString
    debug_str();

    u64
    size();

    Pointer() : Type(type::kind::pointer) {}
};

template<> b32 inline ASTNode::
is<Pointer>() { return is<Type>() && as<Type>()->kind == type::kind::pointer; }

// NOTE(sushi) the following array types inherit from Structured, because they have
//             accessible members and thus need some Structure

// a StaticArray is an array of the form
//      T[N]
// where N is some integer. A StaticArray is allocated onto the stack
// and its data pointer and count cannot be changed
struct StaticArray : public Structured {
    Type* type;
    // NOTE(sushi) the size of an StaticArray does not matter when it comes to type checking
    //             and unique storage of StaticArray, it is used to keep track of what size 
    //             a static array was declared with 
    u64   count;

    static Array<StaticArray*> set;

    // ~~~~~~ interface ~~~~~~~


    static StaticArray*
    create(Type* type, u64 size);

    String
    name();

    DString
    debug_str();

    u64
    size();

    StaticArray() : Structured(structured::static_array) {}
};

template<> b32 inline ASTNode::
is<StaticArray>() { return is<Structured>() && as<Structured>()->kind == structured::static_array; }

// a DynamicArray is an array of the form
//      T[..]
// it stores the members: 
//      data: T*
//      count: u64
//      space: u64
//      allocater: $allocator // TODO(sushi)
struct DynamicArray : public Structured {
    Type* type;

    static Array<DynamicArray*> set;


    // ~~~~~~ interface ~~~~~~~


    static DynamicArray*
    create(Type* type);

    String
    name();

    DString
    debug_str();

    u64
    size();

    DynamicArray() : Structured(structured::dynamic_array) {}
};

template<> b32 inline ASTNode::
is<DynamicArray>() { return is<Structured>() && as<Structured>()->kind == structured::static_array;  }

// a ViewArray is the same as a StaticArray, except that it does not 
// allocate anything onto the stack and its count and data pointer 
// can be changed at runtime
// it is of the form
//      T[]
struct ViewArray : public Structured {
    Type* type;

    static Array<ViewArray*> set;


    // ~~~~~~ interface ~~~~~~~


    static ViewArray*
    create(Type* type);

    String
    name();

    DString
    debug_str();

    u64
    size();

    ViewArray() : Structured(structured::view_array) {}
};

template<> b32 inline ASTNode::
is<ViewArray>() { return is<Structured>() && as<Structured>()->kind == structured::static_array;  }

namespace type::array {
struct ExistantArray {
    u64 hash;
    StaticArray* atype;
};
extern Array<ExistantArray> set;
} // namespace type::array

// a Type which may take on the form of some collection of Types
// this is a tagged union
/* for example:
    
    IP :: variant {
        v4(u8,u8,u8,u8),
        v6(u8[]),
    }

    is equivalent to the C code:

    enum IP_Type {
        v4,
        v6
    };

    struct IP {
        IP_Type type;
        union {
            struct {
                u8 a,b,c,d;
            }v4;
            struct{
                char* str;
            }v6;
        };
    };
*/
struct Variant : public Type {
    Array<Type*> variants;
    
    static Variant*
    create();
};

namespace type::variant {
} // namespace type::variant

struct FunctionType : public Type {
    // pointers to the nodes that define these things
    ASTNode* parameters;
    ASTNode* returns;
    Type* return_type;


    // ~~~~~~ interface ~~~~~~~


    static FunctionType*
    create();

    String
    name();

    DString
    debug_str();

    u64
    size();

    FunctionType() : Type(type::kind::function) {}
};

namespace type::function {
} // namespace type::function

// a tuple acting as a type, eg. one that only contains references to types and not values
struct TupleType : public Type {
    b32 is_named;
    union {
        Array<Type*> types;
        LabelTable table;
    };

    static Array<TupleType*> set;


    // ~~~~~~ interface ~~~~~~~


    // creates a Tuple type from an array of Type*
    // this takes ownership of the 'types' array
    // if the TupleType already exists, the array is deinitialized
    static TupleType*
    create(Array<Type*>& types);

    String
    name();

    DString
    debug_str();

    u64
    size();

    TupleType() : Type(type::kind::tuple) {}
};

namespace type::tuple {
struct ExistantTupleType {
    u64 hash;
    TupleType* ttype;
};
extern Array<ExistantTupleType> set;
} // namespace type::tuple

namespace type::meta {
enum class kind {
    place,
    structure,
    module,
    function,
    trait,
};
} // namespace type::meta

// a Type which represents information about the language
// planned to be mapped 1:1 with the types we use in the compiler
struct MetaType : public Type {
    type::meta::kind kind;
    Structure* s;
};

namespace type {

extern Map<Type*, Type*> type_map;

// struct Formatting {
//     u32 max_parameter_nesting = -1;
//     u32 col;
//     String prefix = "'", suffix = "'";
// }; 

// void
// display(DString& current, Type* type, Formatting format = Formatting(), b32 allow_color = true);

// DString
// display(Type* type, Formatting format = Formatting(), b32 allow_color = true) {
//     DString out = dstring::init();
//     display(out, type, format, allow_color);
//     return out;
// }

} // namespace type




} // namespace amu

#endif // AMU_TYPE_H