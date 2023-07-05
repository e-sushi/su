/*

    Representation of expressions in amu, the bulk of its syntax    

*/

#include "basic/Node.h"

namespace amu {

struct Expression {
    TNode node;
    Type type;
};

namespace expression {

enum ExpressionType : u32 {
    identifier,
    literal,
    entity,
    
    block,
    loop,
    switch_expr,
    switch_case,
    conditional,

    unary_bit_comp,
    unary_logi_not,
    unary_negate,

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

    cast,
    reinterpret,
};

} // namespace expression
} // namespace amu