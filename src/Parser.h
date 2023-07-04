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
        SharedArray<Label*> exported;
        SharedArray<Label*> imported;
        SharedArray<Label*> internal;
    } labels;

     struct{
        Array<Label*> stack;
        Map<String, Label*> table;
        Label* current;
    }label;

    struct{ 
        b32 failed;
    }status;    
};

// data used by individual threads
struct ParserThread {
    Parser* parser;

   

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