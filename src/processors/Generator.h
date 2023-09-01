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

    addition,
    multiplication,
    subtraction,
    division,

    equal,
    not_equal,
    less_than,
    less_than_or_equal,
    greater_than,
    greater_than_or_equal,

    logical_and,
    logical_or,

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

    // conditional jumps
    jump_zero,
    jump_not_zero,
};

namespace arg {
enum kind {
    none,
    var,
    func,
    temporary,
    literal,
};
} // namespace arg

struct Arg {
    arg::kind kind;
    union {
        Var* var; // a Place in memory that this Arg is referring to
        Function* func; // a function for call ops
        TAC* temporary; // a pointer to some TAC whose result this Arg references
        u64 literal;
    };

    Arg() : kind(arg::none) {}
    Arg(Var* p) : kind(arg::var), var(p) {}
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

    // a list of TAC that jump to this TAC 
    Array<TAC*> jump_from;
    // a TAC that this TAC jumps to  
    TAC* jump_to;

    u32 bc_offset;

    // for debug purposes, when a TAC is created its id is the number of TAC created
    // before it. need to move this somewhere better eventually 
    u64 id; 

    // the node the information of this TAC was retrieved from
    ASTNode* node;


    static TAC*
    create();
};

namespace air {

enum class op {
    copy, // copy from B to A

    add, // add B to A and store in A
    sub, // sub B from A and store in A
    mul, // multiply B by A and store in A
    div, // divide B from A and store in A

    // TODO(sushi) consider replacing this 
    eq,  // sets A to one if A == B
    neq, // sets A to one if A != B
    lt,  // sets A to one if A < B
    gt,  // sets A to one if A > B
    le,  // sets A to one if A <= B
    ge,  // sets A to one if A >= B

    call, // jump to a routine
    ret,  // return from a routine

    // jump to a position, A,  relative to current instruction
    jump, 

    // jump to a position, B, relative to current instruction 
    // if A is zero
    jump_zero,  

    // jump to position, B, relative to current instruction
    // if A is non_zero
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

#if BUILD_SLOW
    Var* v; // if this register represents a Var, this points to it 
    u32 idx;
#endif
};

// representation of an AIR bytecode
struct BC {
    air::op instr : 6;

    struct {
        b32 left_is_const : 1  = false;
        b32 left_is_big : 1    = false;
        b32 right_is_const : 1 = false;
        b32 right_is_big : 1   = false;
    } flags;

    union{
        struct {
            s32 offset_a;
            s32 offset_b;
        };
        u64 constant;
    };

#if BUILD_SLOW
    Code* code;
    ASTNode* node;
#endif
};

/*
    A Generator stores the actual array of TAC triples used throughout whatever
    is generating them. Triples store arrays of references to this TAC. This may
    not be efficient and we may want to consider using quadruples instead.

    With this it stores the final TAC sequence, 'fin'.

    The reason we store these things separately is because it's possible for us
    to want to modify the TAC sequence and individual TAC later on, so in order
    to prevent needing to readjust TAC, we just store pointers to stationary
    TAC.
*/
struct Gen {
    u64 tac_count;
    Pool<TAC> tac_pool;
    Array<TAC*> tac;

    // incremented by 1 every time the loop stage ends, indicating to 'add_tac'
    // that it needs to resolve the breaks in break_stacks to the TAC that is
    // about to be created
    u32 resolve_breaks;
    // stacks of jump TAC representing breaks, to be resolved 
    // when a loop's expression returns 
    Array<Array<TAC*>> break_stacks;

    // filled out by both TAC generation and AIR generation TAC emplaces local
    // variables while AIR fills out temp registers used by TAC
    Array<Register> registers;

    Array<BC> air;

    static Gen*
    create(Code* code);
};

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
to_string(DString& current, BC* bc, Code* c);

DString
to_string(BC* bc, Code* c) {
    DString out = dstring::init();
    to_string(out, bc, c);
    return out;
}

void
to_string(DString& current, Register r);

DString
to_string(Register r) {
    DString out = dstring::init();
    to_string(out, r);
    return out;
}

} // namespace amu

#endif // AMU_GENERATOR_H