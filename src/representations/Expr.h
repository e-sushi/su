/*

    Representation of expressions in amu, the bulk of its syntax    

*/

#ifndef AMu_EXPRESSION_H
#define AMu_EXPRESSION_H

#include "basic/Node.h"
#include "Type.h"
#include "Label.h"
#include "Entity.h"
#include "Literal.h"

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
    literal,
    entity_func,
    entity_module,    
    typeref,
    typedef_,
    func_def,
    varref, // a reference to a variable 
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

    unary_bit_comp,
    unary_logi_not,
    unary_negate,
    unary_reference,
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

    cast,
    reinterpret,

};

#include "data/expression_strings.generated"

} // namespace expr


struct Expr : public Entity {
    expr::kind kind;

    Type* type; // the semantic type of this expression

    // assortment of flags for each expression type that hint towards certain things 
    struct {
        struct {
            b32 returning : 1 = 0;
        } conditional;
    } flags;

    // if this is an access expr, this will point to the member being accessed
    // idk where else to put this atm 
    Member* member;
    Literal literal;


    // ~~~~~~ interface ~~~~~~~


    static Expr*
    create(expr::kind kind, Type* type = 0);

    void
    destroy();

    DString
    name();

    DString
    dump();

    Type*
    resolve_type();

    Expr() : Entity(entity::expr) {}

    Expr(expr::kind k) : kind(k), Entity(entity::expr) {}
};

template<> inline b32 Base::
is<Expr>() { return is<Entity>() && as<Entity>()->kind == entity::expr; }

template<> inline b32 Base::
is(expr::kind k) { return is<Expr>() && as<Expr>()->kind == k; }

// namespace literal {
// enum kind {
//     u64_,
//     u32_,
//     u16_,
//     u8_,
//     s64_,
//     s32_,
//     s16_,
//     s8_,
//     f64_,
//     f32_,
//     array,
//     tuple, 
//     string,
//     chr,
// };
// } // namespace literal

// // when this is a tuple or array literal, elements are accessed as children of 
// // this ASTNode
// struct Literal : public Expr {
    


//     // ~~~~~~ interface ~~~~~~~


//     static Literal*
//     create(literal::kind k);

//     void
//     destroy();

//     DString
//     name();

//     DString
//     debug_str();

//     // cast this literal to some other literal kind
//     // so that literal casts dont need to actually appear in the AST 
//     void
//     cast_to(literal::kind k);

//     Literal() : Expr(expr::literal) {}
// };

// template<> b32 inline Base::
// is<Literal>() { return is<Expr>() && as<Expr>()->kind == expr::literal; }

// template<> b32 inline Base::
// is(literal::kind k) { return is<Literal>() && as<Literal>()->kind == k; }

struct Block : public Expr {
    LabelTable table;


    // ~~~~~~ interface ~~~~~~~


    static Block*
    create();

    void
    destroy();

    DString
    name();

    DString
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

    DString
    name();

    DString
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

    DString
    name();

    DString
    dump();

    VarRef() : Expr(expr::varref) {}
};

template<> inline b32 Base::
is<VarRef>() { return is<Expr>() && as<Expr>()->kind == expr::varref; }

} // namespace amu

#endif // AMu_EXPRESSION_H