/*

    The Generator ahs two jobs: generating three-address code (TAC) and 
    generating amu's intermediate representation (AIR) from TAC. 

    TAC is primarily generated for use in optimization, while AIR is what is
    sent to backends or interpretted by amu's virtual machine. 

*/

#ifndef AMU_GENERATOR_H
#define AMU_GENERATOR_H

#include "storage/Array.h"

namespace amu {

struct Function;
struct Var;

struct TAC;

namespace tac {
    

enum op {
    // a TAC that does nothing, usually serving as a label for something else to reference 
    nop,

    // temp memory of some Type
    temp, 

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

    // markers for block start and end
    block_start,
    block_end,

    // the value given by a block if its last expression is not terminated by a semicolon
    block_value,

    // return from a function
    // takes a single optional argument which indicates something to return
    ret, 

    // a jump to another TAC
    jump,

    // jumps used by conditional expressions
    // first argument is a condition, second is a TAC to jump to 
    conditional_jump,

    
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

struct Arg {
    arg::kind kind;
    union {
        Var* place; // a Place in memory that this Arg is referring to
        Function* func; // a function for call ops
        TAC* temporary; // a pointer to some TAC whose result this Arg references
        u64 literal;
    };

    Arg() : kind(arg::none) {}
    Arg(Var* p) : kind(arg::place), place(p) {}
    Arg(Function* f) : kind(arg::func), func(f) {}
    Arg(TAC* t) : kind(arg::temporary), temporary(t) {}
    Arg(u64 l) : kind(arg::literal), literal(l) {}
};

b32
generate(Code* code);

} // namespace tac

// representation of three-address code
// we use indirect triples style storage
// a TAC consists of a single operation that may take 
// one, two, or no arguments. 
struct TAC {
    tac::op op;
    tac::Arg arg0, arg1;
    // for debug purposes, when a TAC is created its id is the number of TAC created
    // before it. need to move this somewhere better eventually 
    u64 id; 
};

namespace air {

enum class opcode {
    copy, // copy from B to A

    add, // add B to A and store in A
    sub, // sub B from A and store in A
    mul, // multiply B by A and store in A
    div, // divide B from A and store in A

    call, // jump to a routine
    ret,  // return from a routine

    // jump to a position, A,  relative to current instruction
    jump, 

    // jump to a position, B, relative to current instruction 
    // if A is not zero
    jump_not_zero,  
};

b32
generate(Code* code);

} // namespace air

// something a bytecode is acting on 
// I believe this will more or less represent a position on the 
// Machine's stack and a bytecode will refer to that position.
struct Register {
    union {
        u8* ptr;

        u64 _u64;
        u32 _u32;
        u16 _u16;
        u8  _u8;

        s64 _s64;
        s32 _s32;
        s16 _s16;
        s8  _s8;

        f64 _f64;
        f32 _f32;
    };
};

// representation of an AIR bytecode
struct BC {
    air::opcode instr : 6;

    struct {
        b32 left_is_const : 1  = false;
        b32 left_is_big : 1    = false;
        b32 right_is_const : 1 = false;
        b32 right_is_big : 1   = false;
    } flags;

    union{
        struct {
            u32 offset_a;
            u32 offset_b;
        };
        u64 constant;
    };
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

    Array<BC> air;
    Array<Register> registers;
};

namespace gen {

Gen*
create(Code* code);

} // namespace gen

void
to_string(DString& current, tac::Arg arg);

DString
to_string(tac::Arg arg) {
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

void
to_string(DString& current, BC* bc);

DString
to_string(BC* bc) {
    DString out = dstring::init();
    to_string(out, bc);
    return out;
}

} // namespace amu

#endif // AMU_GENERATOR_H