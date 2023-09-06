/*

    Structure for representing modules and an interface for interacting with them.

    Modules are the highest level of organization of code in amu.

*/

#ifndef AMU_MODULE_H
#define AMU_MODULE_H

#include "Label.h"

namespace amu {

struct Module : public Entity {
    LabelTable table;

    static Module*
    create();

    void
    destroy();

    Label*
    find_label(String s);

    DString*
    display();

    DString*
    dump();

    Module() : Entity(entity::module) {}
};

template<> inline b32 Base::
is<Module>() { return is<Entity>() && as<Entity>()-> kind == entity::module; }

void
to_string(DString* start, Module* m);

DString*
to_string(Module* m) {
    DString* out = DString::create();
    to_string(out, m);
    return out;
}

} // namespace amu 

#endif // AMU_MODULE_H