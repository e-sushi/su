/*

    Structure representing a variable, some amount of memory that exists on the stack.

*/

#ifndef AMU_VARIABLE_H
#define AMU_VARIABLE_H

namespace amu {

struct Place : public Entity {
    Type* type;


    // ~~~~~~ interface ~~~~~~ 


    static Place*
    create(Type* type = 0);

    void
    destroy();
};


void
to_string(DString& start, Place* p);

DString
to_string(Place* p) {
    DString out = dstring::init();
    to_string(out, p);
    return out;
}

} // namespace amu 

#endif // AMU_VARIABLE_H