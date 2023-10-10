/*

    Structure for representing modules and an interface for interacting with them.

    Modules are the highest level of organization of code in amu. They function as a grouping of Labels
	meaning that they can store anything a file can, in fact a file is itself a Module.

	All labels declared in a module may be accessed globally from within that module. This requires that 
	we perform 'discretization' of modules which involves taking all of the labels found in a modules 
	lexical scope and creating Code objects for them so that they may be parsed independently of each
	other and in any order.

*/

#ifndef AMU_MODULE_H
#define AMU_MODULE_H

#include "Label.h"

namespace amu {

struct Module : public Entity {
    LabelTable* table;


	// ~~~~ interface ~~~~


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
