/*

    Structure representing a variable, some amount of memory that exists on the stack.

    A Var may only exist at compile time. In this case, 'memory', points to a location in memory
    with enough space to fit the type of the Var.

*/

#ifndef AMU_VARIABLE_H
#define AMU_VARIABLE_H

namespace amu {

struct Var : public Entity {
    Type* type;

    // offset in bytes from the beginning of the stack (fp) 
    u64 stack_offset;

    // set when a variable is evaluated at compile time, indicating
    // that any references to it should take on its value, not store
    // a VarRef to it 
    b32 is_compile_time;
    b32 is_global;
    u8* memory;
    

    // ~~~~~~ interface ~~~~~~ 


    static Var*
    create(Allocator allocator, Type* type = 0);

    void
    destroy();

    DString
    display();

    DString
    dump();

    Type*
    resolve_type() { return type; }

    Var() : Entity(Entity::Kind::Var) {}
};

} // namespace amu 

#endif // AMU_VARIABLE_H
