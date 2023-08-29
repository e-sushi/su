#ifndef AMU_PARSER_H
#define AMU_PARSER_H

#include "storage/Map.h"
#include "storage/Array.h"
#include "storage/SharedArray.h"
#include "storage/String.h"
#include "representations/Label.h"
#include "representations/Code.h"

namespace amu {

struct Parser {
    Code* code;

    TNode* root;

    // labels collected at the highest scope of a parser
    LabelTable table;

    Array<Module*> module_stack;
    Module* current_module;
    
    Array<LabelTable*> table_stack;
    LabelTable* current_table;

    Array<TNode*> stack;
    TNode* last;

    // array of Tokens representing identifiers that refer to something
    // unknown when we come across it in parsing 
    Array<Token> unknowns;
};

namespace parser {

Parser*
create(Code* source);

void
destroy(Parser* parser);

void
parse(Code* code);

namespace stack {

void
push(Code* c, TNode* node);

// NOTE(sushi) this is somewhat dangerous cause you can accidentally pass something like 
//             a function pointer to this and it will work
//             but it's useful so i will keep it for now :)
//             if our types inherited TNode, we could use std::is_base_of
//             but i dont want to implement all of that rn
template<typename T> FORCE_INLINE void
push(Code* c, T* node) { push(c, (TNode*)node); }

TNode*
pop(Code* c);

TNode*
last(Code* c);

void
push_table(Code* c, LabelTable* table);

void
pop_table(Code* c);

LabelTable*
current_table(Code* c);

DString
display(Code* c);

} // namespace stack

namespace table {
// adds the given label to whatever table is currently active
void 
add(Code* c, String id, Label* l);
} // namespace table

namespace symbol {

Label*
search(Code* code, u64 hashed_id);

} // namespace symbol


} // namespace parser
} // namespace amu 

#endif // AMU_PARSER_H