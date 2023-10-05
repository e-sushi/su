/*

    Representation of expressions in amu, the bulk of its syntax    

*/

#ifndef AMu_EXPRESSION_H
#define AMu_EXPRESSION_H

#include "basic/Node.h"
#include "Type.h"
#include "Label.h"
#include "Entity.h"
#include "Frame.h"

namespace amu {

struct Token;
struct Entity;
struct Type;
struct Function;
struct Tuple;

namespace expr {
// @genstrings(data/expression_strings.generated)
enum kind : u32 {
    null,

    identifier,
    literal_scalar,
    literal_string,
    literal_array,
    literal_tuple,
    literal_struct,
    function,
    typeref,
    typedef_,
    func_def,
    varref, // a reference to a variable 
	label_ref, 
    call,
    
    block,
    loop,
    for_,
    switch_expr,
    switch_case,
    conditional,
    return_,
    using_,
    break_,
    subscript,

    unary_bit_comp,
    unary_logi_not,
    unary_negate,
    unary_reference,
    unary_dereference,
    unary_assignment,
    unary_comptime,

    binary_plus,
    binary_minus,
    binary_multiply,
    binary_division,
    binary_and,
    binary_bit_and,
    binary_or,
    binary_bit_or,
    binary_less_than,
    binary_greater_than,
    binary_less_than_or_equal,
    binary_greater_than_or_equal,
    binary_equal,
    binary_not_equal,
    binary_modulo,
    binary_bit_xor,
    binary_bit_shift_left,
    binary_bit_shift_right,
    binary_access,
    binary_structure_access,
    binary_assignment,
    binary_comptime,
    binary_range,
    binary_in,

    cast,
    reinterpret,

    // TODO(sushi) make this usable anywhere by allowing it be after and factor maybe 
    vm_break,

	intrinsic_rand_int,
	intrinsic_print,
};

#include "data/expression_strings.generated"

} // namespace expr


struct Expr : public Entity {
    expr::kind kind;

    Type* type; // the semantic type of this expression

    // if this is an access expr, this will point to the member being accessed
    // idk where else to put this atm 
    Member* member;

    // when true, this expression represents a location in memory 
    // and operations performed on it should directly affect it 
    b32 lvalue;
	
	// set true on expressions in Sema that can be considered 
	// computable at compile time
	b32 compile_time;


    // ~~~~~~ interface ~~~~~~~


    static Expr*
    create(expr::kind kind, Type* type = 0);

    void
    destroy();

    DString*
    display();

    DString*
    dump();

    Type*
    resolve_type();

    Expr() : Entity(entity::expr) {}

    Expr(expr::kind k) : kind(k), Entity(entity::expr) {def = this;}
};

template<> inline b32 Base::
is<Expr>() { return is<Entity>() && as<Entity>()->kind == entity::expr; }

template<> inline b32 Base::
is(expr::kind k) { return is<Expr>() && as<Expr>()->kind == k; }

// representation of a single expression meant to be evaluated at compile time
// this is primarily so that we can keep track of frame information that an 
// Expr may need, which is just local variables for now 
// the original expression is the first child, the evaluated AST is the last
struct CompileTime : public Expr {
    // frame information for this compile time expression
    Frame frame;


    // ~~~~~~ interface ~~~~~~~


    static CompileTime*
    create(Type* type = 0);

    void
    destroy();

    DString*
    display();

    DString*
    dump();

    CompileTime() : Expr(expr::unary_comptime) {}
};

template<> inline b32 Base::
is<CompileTime>() { return is<Expr>() && as<Expr>()->kind == expr::unary_comptime; }

// The following Literal structures are to help keep each kind of literal separate
// while also providing a common interface for working with them.
// I originally tried just having a Literal struct, but found that it stored too much information
// and I didn't know where exactly to put it.
// Though I also don't really care for this style either, but if it works better then whatever.
// I just dont like having several representations of literals throughout the project. Tokens have their own
// thing, Expressions have their own thing, and now TAC Args have their own as well.

// the Scalar type of this Expr determines which literal to use 
// This is one place where OOP fails I think. I need to store a kind on ScalarValue
// so that other things that use it can tell what to do with it, but here it's not necessary
// because Expr already has an underlying Type which can be used to determine this, so a u32 or 64, whatever
// C++ makes it, is actually wasted here. There's probably some convoluted way to design this better
// but I don't care for now.
struct ScalarLiteral : public Expr {
    ScalarValue value;


    // ~~~~ interface ~~~~


    static ScalarLiteral*
    create();

    void
    destroy();

    DString*
    display();

    DString*
    dump();

    // NOTE(sushi) it is IMPORTANT!!! that you call this and NOT the function
    //             on 'value' because otherwise the type of this expression will
    //             not change! 
    //             this KINDA SUCKS but whatever
    void
    cast_to(scalar::kind k);

    void
    cast_to(Type* t);

    // NOTE(sushi) this returns if the scalar is of signed INTEGER type
    b32 
    is_signed();

    b32
    is_float();

    b32
    is_negative();

    ScalarLiteral() : Expr(expr::literal_scalar) {}
};

template<> b32 inline Base::
is<ScalarLiteral>() { return is<Expr>() && as<Expr>()->kind == expr::literal_scalar; }

struct StringLiteral : public Expr {
    String raw;


    // ~~~~ interface ~~~~


    static StringLiteral*
    create();

    void
    destroy();

    DString*
    display();

    DString*
    dump();

    StringLiteral() : Expr(expr::literal_string) {}
};

template<> b32 inline Base::
is<StringLiteral>() { return is<Expr>() && as<Expr>()->kind == expr::literal_string; }

// the Type of this expression will be StaticArray and the underlying type of the 
// array can be reached from there.

// idk if this is really necessary, there's no reason to store an Array of Expr* because 
// the elements are just its children anyways
struct ArrayLiteral : public Expr {
    Array<Expr*> elements;


    // ~~~~ interface ~~~~


    static ArrayLiteral*
    create();

    void
    destroy();

    DString*
    display();

    DString*
    dump();

    // casts this array in place to an array of the given
    // type and applies casts to each of its elements 
    // count cannot change here and this doesn't do any checks
    // to see if the cast is valid
    void
    cast_to(Type* t);

    ArrayLiteral() : Expr(expr::literal_array) {}
};

template<> b32 inline Base::
is<ArrayLiteral>() { return is<Expr>() && as<Expr>()->kind == expr::literal_array; }

// NOTE(sushi) this represents just plain tuple literals and tuple literals being 
//             used as struct initializers 
struct TupleLiteral : public Expr {
    Tuple* t;


    // ~~~~ interface ~~~~


    static TupleLiteral*
    create();

    void
    destroy();

    DString*
    display();

    DString*
    dump();

    TupleLiteral() : Expr(expr::literal_tuple) {}
};

template<> b32 inline Base::
is<TupleLiteral>() { return is<Expr>() && as<Expr>()->kind == expr::literal_tuple; }

// any definition of a function, so an expr of any of the forms: 
// 	1. full:
// 		(...) -> ... { ... }
// 	2. reduced:
// 		(...) [ -> ...] { ... }
struct FunctionLiteral : public Expr {
	Function* func;


	// ~~~~ interface ~~~~
	

	static FunctionLiteral*
	create();

	void
	destroy();

	DString*
	display();

	DString*
	dump();

	FunctionLiteral() : Expr(expr::function) {}
};

struct Block : public Expr {
    LabelTable table;


    // ~~~~~~ interface ~~~~~~~


    static Block*
    create();

    void
    destroy();

    DString*
    display();

    DString*
    dump();

    Block() : Expr(expr::block) {}
};

template<> inline b32 Base::
is<Block>() { return is<Expr>() && as<Expr>()->kind == expr::block; }

struct Call : public Expr {
    Function* callee;
    Tuple* arguments;


    // ~~~~~~ interface ~~~~~~~


    static Call*
    create();

    void
    destroy();

    DString*
    display();

    DString*
    dump();

    Call() : Expr(expr::call) {}
};

template<> inline b32 Base::
is<Call>() { return is<Expr>() && as<Expr>()->kind == expr::call; }

struct VarRef : public Expr {
    Var* var;


    // ~~~~~~ interface ~~~~~~~


    static VarRef*
    create();

    void
    destroy(); 

    DString*
    display();

    DString*
    dump();

    VarRef() : Expr(expr::varref) {}
};

template<> inline b32 Base::
is<VarRef>() { return is<Expr>() && as<Expr>()->kind == expr::varref; }

// for loops hold a table
struct For : public Expr {
    LabelTable table;


    // ~~~~~~ interface ~~~~~~~


    static For*
    create();

    void
    destroy();

    DString*
    display();

    DString*
    dump();

    For() : Expr(expr::for_) {} 
};

struct IntegerRange : public Expr {
    s64 left;
    s64 right;
};

} // namespace amu

#endif // AMu_EXPRESSION_H
