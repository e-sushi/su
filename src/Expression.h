/*

    Representation of expressions in amu, the bulk of its syntax    

*/

#ifndef AMu_EXPRESSION_H
#define AMu_EXPRESSION_H

#include "basic/Node.h"
#include "Type.h"

namespace amu {

namespace expression {
// @genstrings(data/expression_strings.generated)
enum kind : u32 {
    null,

    top_level,

    identifier,
    literal,
    entity_func,
    entity_module,    
    typeref,
    
    block,
    loop,
    for_,
    switch_expr,
    switch_case,
    conditional,
    return_,
    using_,

    unary_bit_comp,
    unary_logi_not,
    unary_negate,
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

} // namespace expression

struct Token;
struct Entity;
struct Expression {
    TNode node;
    expression::kind kind;

    union {
        Type* type; // the semantic type of this expression
        Entity* entity;
    };
};

namespace expression {

global Expression*
create();

global void
destroy(Expression& e);

} // namespace expression

void
to_string(DString& start, Expression* e);

} // namespace amu

#endif // AMu_EXPRESSION_H