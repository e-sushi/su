/*

    Semantic analyzer 

*/

#ifndef AMU_VALIDATOR_H
#define AMU_VALIDATOR_H

namespace amu {

struct LabelTable;

struct Sema {
    Source* source;

    Array<Module*> module_stack;
    Module* current_module;

    Array<LabelTable*> table_stack;
    LabelTable* current_table;
};

namespace sema {

Sema*
create();

void
destroy(Sema* v);

b32
analyze(Code* code);

namespace table {
    
void
push(Code* code, LabelTable* table);

void
pop(Code* code);

} // namespace table

} // namespace sema



} // namespace amu 

#endif // AMU_VALIDATOR_H