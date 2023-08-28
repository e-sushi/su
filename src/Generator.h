/*

    Types and interface for interacting with the IR generator as well as types that it uses.

*/

#ifndef AMU_GENERATOR_H
#define AMU_GENERATOR_H

#include "storage/Array.h"

namespace amu {

struct Function;
struct Place;
struct TAC;

namespace tac {
enum op {
    stack_push, // N bytes to push
    stack_pop,  // N bytes to pop

    // arithmetic, always makes a temporary
    addition,
    multiplication,
    subtraction,
    division,

    // assignment between 2 things
    assignment, 

    // a parameter for an upcoming call
    param,

    // a call to a function
    call,

    // markers for block start and stop so that we may clean Variables up when 
    // their containing scopes
    block_start,
    block_end,

    // return from a function
    // takes a single optional argument which indicates something to return
    ret, 

    // a jump to another TAC
    jump,

    // jumps used by conditional expressions
    // first argument is a condition, second is a TAC to jump to 
    conditional_jump,

    // the value given by a block if its last expression is not terminated by a semicolon
    block_value,
};

namespace arg {
enum kind {
    none,
    place,
    func,
    temporary,
    literal,
};
} // namespace arg
} // namespace tac

struct Arg {
    tac::arg::kind kind;
    union {
        Place* place; // a Place in memory that this Arg is referring to
        Function* func; // a function for call ops
        TAC* temporary; // a pointer to some TAC whose result this Arg references
        u64 literal;
    };
};

// representation of Three-address Code
// we use indirect triples style storage
struct TAC {
    tac::op op;
    Arg arg0, arg1;
    // for debug purposes, when a TAC is created its id is the number of TAC created
    // before it. need to move this somewhere better eventually 
    u64 id; 
};

/*
    A Generator stores the actual array of TAC triples used throughout whatever is generating them.
    Triples store arrays of references to this TAC. This may not be efficient and we may want to consider
    using quadruples instead.

    With this it stores the final TAC sequence, 'fin'.

    The reason we store these things separately is because it's possible for us to want to modify the TAC
    sequence and individual TAC later on, so in order to prevent needing to readjust TAC, we just store pointers
    to stationary TAC.
*/
struct Gen {
    u64 tac_count;
    Pool<TAC> tac_pool;
    Array<TAC*> tac;
};

namespace gen {

Gen*
create(Code* code);

b32
generate(Code* code);

} // namespace gen

void
to_string(DString& current, Arg arg);

DString
to_string(Arg arg) {
    DString out = dstring::init();
    to_string(out, arg);
    return out;
}


void
to_string(DString& current, TAC* tac);

DString
to_string(TAC* tac) { 
    DString out = dstring::init();
    to_string(out, tac);
    return out;
}

} // namespace amu

#endif // AMU_GENERATOR_H