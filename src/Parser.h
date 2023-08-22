#ifndef AMU_PARSER_H
#define AMU_PARSER_H

#include "Label.h"
#include "storage/Map.h"
#include "storage/Array.h"
#include "storage/SharedArray.h"
#include "storage/String.h"
#include "Code.h"

namespace amu {

struct Parser {
    Code* code;

    TNode root;

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

Parser
init(Code* source);

void
deinit(Parser& parser);

void
execute(Code* code);

namespace stack {

void
push(Code* c, TNode* node);

TNode*
pop(Code* c);

TNode*
last(Code* c);

void
push_table(Code* c, LabelTable* table);

} // namespace stack


} // namespace parser
} // namespace amu 

#endif // AMU_PARSER_H