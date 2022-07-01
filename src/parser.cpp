#ifdef FIX_MY_IDE_PLEASE
#include "core/memory.h"

#define KIGU_STRING_ALLOCATOR deshi_temp_allocator
#include "kigu/profiling.h"
#include "kigu/array.h"
#include "kigu/array_utils.h"
#include "kigu/common.h"
#include "kigu/cstring.h"
#include "kigu/map.h"
#include "kigu/string.h"
#include "kigu/node.h"

#define DESHI_DISABLE_IMGUI
#include "core/logger.h"
#include "core/platform.h"
#include "core/file.h"
#include "core/threading.h"

#include <stdio.h>
#include <stdlib.h>

#include "kigu/common.h"
#include "kigu/unicode.h"
#include "kigu/hash.h"
#include "types.h"

#endif


TNode* Parser::declare(Type type){
    logger_push_indent();
    
    switch(type){
        case decl_structure:{
            Struct* s = arena.make_struct();
            s->decl.identifier = curt->raw;
            s->decl.token_start = curt;
            while(curt->type!=Token_CloseBrace){
                curt++;
            }
            s->decl.token_end = curt;
        }break;
    }

    logger_pop_indent();

    return 0;
}

TNode* Parser::define(TNode* node, ParseStage stage){
    logger_push_indent();



    logger_pop_indent();

    return 0;
}



ParsedFile* Parser::parse(PreprocessedFile* prefile){
    Stopwatch time = start_stopwatch();
    Log("", "Parsing ", VTS_BlueFg, prefile->lexfile->file->name, VTS_Default);
    logger_push_indent();

    LexedFile* lexfile = prefile->lexfile;
    File* file = lexfile->file;

    if(parsed_files.has(file->name)){
        Log("", VTS_GreenFg, "File has already been parsed.", VTS_Default);
        return &parsed_files[file->name];
    }

    parsed_files.add(file->front);
    parfile = &parsed_files[file->front];
    parfile->prefile = prefile;

    stacks.structs_pushed.add(0);
    stacks.functions_pushed.add(0);
    stacks.variables_pushed.add(0);

    {//declare all global declarations
        for(u32 idx : prefile->decl.exported.structs){
            curt = &prefile->lexfile->tokens[idx];
            Struct* s = StructFromNode(declare(decl_structure));
            parfile->decl.exported.structs.add(s);
            (*stacks.structs_pushed.last)++;
            stacks.structs.add(s);
        }   

        for(u32 idx : prefile->decl.internal.structs){
            curt = &prefile->lexfile->tokens[idx];
            Struct* s = StructFromNode(declare(decl_structure));
            parfile->decl.internal.structs.add(s);
            (*stacks.structs_pushed.last)++;
            stacks.structs.add(s);
        }

        for(u32 idx : prefile->decl.exported.funcs){
            curt = &prefile->lexfile->tokens[idx];
            Function* f = FunctionFromNode(declare(decl_function));
            parfile->decl.exported.funcs.add(f);
            (*stacks.functions_pushed.last)++;
            stacks.functions.add(f);
        }

        for(u32 idx : prefile->decl.internal.funcs){
            curt = &prefile->lexfile->tokens[idx];
            Function* f = FunctionFromNode(declare(decl_function));
            parfile->decl.internal.funcs.add(f);
            (*stacks.functions_pushed.last)++;
            stacks.functions.add(f);
        }

        for(u32 idx : prefile->decl.exported.vars){
            curt = &prefile->lexfile->tokens[idx];
            Variable* v = VariableFromNode(declare(decl_variable));
            parfile->decl.exported.vars.add(v);
            (*stacks.variables_pushed.last)++;
            stacks.variables.add(v);
        }

        for(u32 idx : prefile->decl.internal.vars){
            curt = &prefile->lexfile->tokens[idx];
            Variable* v = VariableFromNode(declare(decl_variable));
            parfile->decl.internal.vars.add(v);
            (*stacks.variables_pushed.last)++;
            stacks.variables.add(v);
        }
    }

    

    Log("", VTS_GreenFg, "Finished parsing in ", peek_stopwatch(time), " ms", VTS_Default);

    return 0;

}