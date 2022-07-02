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

#include "lexer.cpp"
#endif

#define preprocess_error(token, ...)\
suLog(0, ErrorFormat("error: "), (token).file, "(",(token).l0,",",(token).c0,"):", __VA_ARGS__)

#define preprocess_warn(token, ...)\
suLog(0, WarningFormat("warning: "), (token).file, "(",(token).l0,",",(token).c0,"):", __VA_ARGS__)

                   
PreprocessedFile* Preprocessor::preprocess(LexedFile* lexfile){DPZoneScoped;
    suLog(1, "Preprocessing ", lexfile->file->name);

    Stopwatch time = start_stopwatch();
   
    //first we look for imports, if they are founda we lex the file being imported from recursively
    //TODO(sushi) this can probably be multithreaded
    array<str8> import_paths;
    for(u32 importidx : lexfile->preprocessor.imports){
        suLog(2, "Processing imports");
        Token* curt = &lexfile->tokens[importidx];
        curt++;
        if(curt->type == Token_OpenBrace){
            //multiline directive 
            while(curt->type != Token_CloseBrace){
                if(curt->type == Token_LiteralString){
                    Token* mod = curt;
                    
                    //TODO(sushi) when we implement searching PATH, we should also allow the use to omit .su

                    //attempt to find the module in PATH and current working directory
                    //first check cwd
                    if(file_exists(curt->raw)){
                        suLog(2, "Processing import ", curt->raw);
                        LexedFile* lexfile = compiler.start_lexer(curt->raw);
                        PreprocessedFile* pfile = compiler.start_preprocessor(lexfile);
                        prefile->imported_files.add(pfile);
                        
                        suLog(2, "Continuing preprocessing of ", VTS_BlueFg, lexfile->file->name, VTS_Default);
                        logger_push_indent(2);
                        
                    }else{
                        //TODO(sushi) import paths
                        preprocess_warn(*curt, "Finding files through PATH is not currently supported.");
                    }
                    //NOTE(sushi) we do not handle selective imports here, that is handled in parsing
                    curt++;
                    if(curt->type == Token_OpenBrace){
                        while(curt->type != Token_CloseBrace){curt++;}
                    }
                }else{

                }
                curt++;
            }
        }
        logger_pop_indent();
    }
    
    suLog(2, "Finding internal declarations.");
    //TODO(sushi) this copying sucks, maybe preprocessedfile should just use lexedfile's maps?
    prefile->exported_identifiers = lexfile->global_identifiers;
    for(u32 idx : lexfile->preprocessor.internals){
        Token* curt = &lexfile->tokens[idx];
        curt++;
        u32 start = idx, end;
        if(curt->type == Token_OpenBrace){
            //scoped internal, we must find its extent
            u32 scope_depth = 0;
            while(1){
                curt++; idx++;
                if(curt->type == Token_OpenBrace) scope_depth++;
                else if(curt->type == Token_CloseBrace){
                    if(!scope_depth) break;
                    else{
                        scope_depth--;
                    }
                }
            }
            end = idx;
        }else{
            //the rest of the file is considered internal
            end = lexfile->tokens.count;
        }   

        //move exported identifiers into internal
        forI(prefile->exported_identifiers.count){
            Identifier id = prefile->exported_identifiers[i];
            u32 idx = id.token->idx;
            if(idx > start && idx < end){
                id.internal = 1;
                prefile->internal_identifiers.add(id.alias, id);
                prefile->exported_identifiers.remove(id.alias);
            }
        }
        //local identifiers do not need to be declared internal
    }


    suLog(2, "Finding run directives (NotImplemented)");
    for(u32 idx : lexfile->preprocessor.runs){
        preprocess_error(lexfile->tokens[idx], "#run is not implemented yet.");
    }
   

    if(globals.verbosity > 3){
        if(prefile->exported_identifiers.count)
            suLog(4, "Exported identifiers");
        for(Identifier& id : prefile->exported_identifiers){
            suLog(4, "  ", id.alias);
        }

        if(prefile->internal_identifiers.count)
            suLog(4, "Internal identifiers");
        for(Identifier& id : prefile->internal_identifiers){
            suLog(4, "  ", id.alias);
        }
    }
    


    
    suLog(1, VTS_GreenFg, "Finished preprocessing in ", peek_stopwatch(time), " ms", VTS_Default);
    logger_pop_indent(2);

    return prefile;
}