/*

    Structure representing a variable, some amount of memory that exists on the stack.

*/

#ifndef AMU_VARIABLE_H
#define AMU_VARIABLE_H

namespace amu {

struct Register;

struct Var : public Entity {
    Type* type;

    // offset in bytes from the beginning of the stack (fp) 
    u64 stack_offset;
    

    // ~~~~~~ interface ~~~~~~ 


    static Var*
    create(Type* type = 0);

    void
    destroy();

    DString
    name();

    DString
    dump();

    Type*
    resolve_type() { return type; }

    Var() : Entity(entity::var) {}
};


void
to_string(DString& start, Var* p);

DString
to_string(Var* p) {
    DString out = dstring::init();
    to_string(out, p);
    return out;
}

} // namespace amu 

#endif // AMU_VARIABLE_H