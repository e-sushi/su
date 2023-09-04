/*

    Representation of expressions in amu, the bulk of its syntax    

*/

#ifndef AMu_EXPRESSION_H
#define AMu_EXPRESSION_H

#include "basic/Node.h"
#include "Type.h"
#include "Label.h"
#include "Entity.h"

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

    // a list of TAC that need to jump to this Expr
    // for whatever reason 
    Array<TAC*> jumps_to_me;


    // ~~~~~~ interface ~~~~~~~


    static Expr*
    create(expr::kind kind, Type* type = 0);

    void
    destroy();

    String
    name();

    DString
    debug_str();

    Type*
    resolve_type();

    Expr() : Entity(entity::expr) {}

    Expr(expr::kind k) : kind(k), Entity(entity::expr) {}
};

template<> inline b32 ASTNode::
is<Expr>() { return is<Entity>() && as<Entity>()->kind == entity::expr; }

template<> inline b32 ASTNode::
is(expr::kind k) { return is<Expr>() && as<Expr>()->kind == k; }

struct Block : public Expr {
    LabelTable table;


    // ~~~~~~ interface ~~~~~~~


    static Block*
    create();

    void
    destroy();

    String
    name();

    DString
    debug_str();

    Block() : Expr(expr::block) {}
};

template<> inline b32 ASTNode::
is<Block>() { return is<Expr>() && as<Expr>()->kind == expr::block; }

template<> inline b32 ASTNode::
next_is<Block>() { return next() && next()->is<Block>(); }

struct Call : public Expr {
    Function* callee;
    Tuple* arguments;


    // ~~~~~~ interface ~~~~~~~


    static Call*
    create();

    void
    destroy();

    String
    name();

    DString
    debug_str();

    Call() : Expr(expr::call) {}
};

template<> inline b32 ASTNode::
is<Call>() { return is<Expr>() && as<Expr>()->kind == expr::call; }

struct VarRef : public Expr {
    Var* var;


    // ~~~~~~ interface ~~~~~~~


    static VarRef*
    create();

    void
    destroy(); 

    String
    name();

    DString
    debug_str();

    VarRef() : Expr(expr::varref) {}
};

template<> inline b32 ASTNode::
is<VarRef>() { return is<Expr>() && as<Expr>()->kind == expr::varref; }

// special so we can store what the offset of the access is 
struct Access : public Expr {
    u64 offset; // the offset from the start of base to reach var


    // ~~~~~~ interface ~~~~~~~


    static Access*
    create();

    void
    destroy(); 

    String
    name();

    DString
    debug_str();

    Access() : Expr(expr::binary_access) {}
};

} // namespace amu

#endif // AMu_EXPRESSION_H