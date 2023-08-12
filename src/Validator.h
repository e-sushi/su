/*

    Semantic analyzer 

*/

namespace amu {

struct Validator {
    Source* source;

    Array<Module*> module_stack;
    Module* current_module;
};

namespace validator {

global Validator
init();

global void
deinit(Validator& v);

void 
execute(Validator& v);

} // namespace validator
} // namespace amu 