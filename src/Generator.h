/*

    Types and interface for interacting with the IR generator as well as types that it uses.

*/

#ifndef AMU_GENERATOR_H
#define AMU_GENERATOR_H

#include "storage/Array.h"

namespace amu {

struct Label;

namespace tac {
enum op {
    stack_push, // N bytes to push
    stack_pop,  // N bytes to pop

    addition,
    multiplication,
    subtraction,
    division,

    conditional_jump, // condition, jump location
};

namespace arg {
enum kind {
    none,
    symbol,
    temporary,
    literal,
};
} // namespace arg
} // namespace tac

struct Arg {
    tac::arg::kind kind;
    union {
        Label* symbol;
        u64 temporary;
        u64 literal;
    };
};

// representation of Three-address Code
// we use triples style storage
struct TAC {
    tac::op op;
    Arg arg0, arg1;
};

struct IntermediateFunction {
    Array<u64> arguments;
};

struct Generator {
    Array<TAC> tac;
};

namespace generator {

Generator*
create(Code* code);

void
execute(Code* code);

} // namespace generator
} // namespace amu

#endif // AMU_GENERATOR_H