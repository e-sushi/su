/*

    Representation of expressions in amu, the bulk of its syntax    

*/

#include "basic/Node.h"
#include "Type.h"

namespace amu {

namespace expression {

enum kind : u32 {
    null,

    top_level,

    identifier,
    literal,
    entity,
    typeref,
    
    block,
    loop,
    switch_expr,
    switch_case,
    conditional,

    unary_bit_comp,
    unary_logi_not,
    unary_negate,
    unary_assignment,
    unary_comptime,

    type, // this expression represents a handle to some type 

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

} // namespace expression

struct Token;
struct Expression {
    TNode node;
    expression::kind kind;

    Type type; // the semantic type of this expression

    b32 is_compiletime;

    Token* start;
    Token* end;
};

} // namespace amu