/*

    Semantic analyzer 

*/

#ifndef AMU_VALIDATOR_H
#define AMU_VALIDATOR_H

namespace amu {

struct Validator {
    Source* source;

    Array<Module*> module_stack;
    Module* current_module;
};

namespace validator {

Validator*
create();

void
destroy(Validator* v);

void 
execute(Code* code);

} // namespace validator
} // namespace amu 

#endif // AMU_VALIDATOR_H