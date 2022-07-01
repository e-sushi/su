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


TNode* Parser::declare(TNode* node, Type type){
    logger_push_indent();
    
    switch(type){
        case decl_structure:{
            Struct* s = arena.make_struct();
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
    ParsedFile* parfile = &parsed_files[file->front];
    parfile->prefile = prefile;




    for(u32 idx : prefile->decl.exported.structs){

    }   

    Log("", VTS_GreenFg, "Finished parsing in ", peek_stopwatch(time), " ms", VTS_Default);

    return 0;

}