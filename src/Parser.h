#ifndef AMU_PARSER_H
#define AMU_PARSER_H

#include "Label.h"
#include "storage/Map.h"
#include "storage/Array.h"
#include "storage/SharedArray.h"
#include "storage/String.h"
#include "Source.h"

namespace amu {

struct Parser {
    Source* source;

    struct {
        Array<Label*> exported; // !Threading: these will need to be SharedArrays or locked with some mutex
        Array<Label*> imported;
        Array<Label*> internal;
    } labels;

    Array<Module*> module_stack;
    Module* current_module;
};

namespace parser {

Parser
init(Source* source);

void
deinit(Parser& parser);

void
execute(Parser& parser);

} // namespace parser

} // namespace amu 

#endif // AMU_PARSER_H