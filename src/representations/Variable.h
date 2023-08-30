/*

    Structure representing a variable, some amount of memory that exists on the stack.

*/

#ifndef AMU_VARIABLE_H
#define AMU_VARIABLE_H

namespace amu {

struct Register;

struct Var : public Entity {
    Type* type;
    // the Register offset representing this Var
    // not set until TAC generation
    // TODO(sushi) look into a better place to put this if any
    u32 reg_offset;

    // ~~~~~~ interface ~~~~~~ 


    static Var*
    create(Type* type = 0);

    void
    destroy();

    String
    name();

    DString
    debug_str();

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