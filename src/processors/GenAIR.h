/*

    AIR generator. See AIR.h for information on what AIR is.

    This takes Code that has generated TAC that's optionally been optimized and generates a sequence of
    AIR representing it. 

*/

#ifndef AMU_GENAIR_H
#define AMU_GENAIR_H

namespace amu {

// something that'll be put on the stack that is not a
// local variable.
struct StackThing {
    u64 offset;
    width w;
    Type* type;
    DString* name;
};

struct GenAIR {
    Code* code;

    Array<BC> seq;

    // stack_offset in bytes 
    u32 stack_offset;

    Array<u64> scoped_temps;

    Map<BC*, StackThing> stack_things;

    static GenAIR*
    create(Code* code);

    void
    generate();

private:
    void start();
    void body();

    // creates room on the stack for data of some Type
    // then returns the beginning of that data
    u64
    new_reg(Type* t);

    void
    push_temp(TAC* tac);

    void
    clean_temps();

    void
    push_scope();

    void
    pop_scope();
};

} // namespace amu 

#endif // AMU_GENAIR_H