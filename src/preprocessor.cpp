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
    str8 filename = lexfile->file->name;
    suLog(1, CyanFormatDyn(lexfile->file->name), ": Preprocessing... ");

    Stopwatch time = start_stopwatch();
   
    //first we look for imports, if they are found we lex the file being imported from recursively
    //TODO(sushi) this can probably be multithreaded
    //we gather import paths so we can setup a job for each one 
    // and lex then preprocess each one at the same time.
    CompilerRequest cr;
    for(u32 importidx : lexfile->preprocessor.imports){
        suLog(2, filename, "Processing imports");
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
                        suLog(2, filename, "Adding import path ", curt->raw);
                        cr.filepaths.add(curt->raw);
                        
                    }else{
                        //TODO(sushi) look for imports on PATH
                        preprocess_warn(*curt, "Finding files through PATH is not currently supported.");
                    }
                    //NOTE(sushi) we do not handle selective imports here, that is handled in parsing
                    curt++;
                    if(curt->type == Token_OpenBrace){
                        while(curt->type != Token_CloseBrace){curt++;}
                    }
                }
                curt++;
            }
        }
    }
    if(cr.filepaths.count){
        cr.stages = Stage_Lexer | Stage_Preprocessor;
        compiler.compile(&cr);
    }
    
    suLog(2, filename, "Finding internal declarations.");
    //TODO(sushi) this copying sucks, maybe preprocessedfile should just use lexedfile's maps?
    prefile->exported_decl = lexfile->global_decl;
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
        forI(prefile->exported_decl.count){
            Declaration id = prefile->exported_decl[i];
            u32 idx = id.token->idx;
            if(idx > start && idx < end){
                id.internal = 1;
                prefile->internal_decl.add(id.alias, id);
                prefile->exported_decl.remove(id.alias);
            }
        }
        //local identifiers do not need to be declared internal
    }


    suLog(2, filename, "Finding run directives ", ErrorFormat("(NotImplemented)"));
    for(u32 idx : lexfile->preprocessor.runs){
        preprocess_error(lexfile->tokens[idx], "#run is not implemented yet.");
    }
   

    if(globals.verbosity > 3){
        if(prefile->exported_decl.count)
            suLog(4, filename, "Exported declarations");
        for(Declaration& id : prefile->exported_decl){
            suLog(4, filename, "  ", id.alias);
        }

        if(prefile->internal_decl.count)
            suLog(4, filename, "Internal declarations");
        for(Declaration& id : prefile->internal_decl){
            suLog(4, filename, "  ", id.alias);
        }
    }
    


    
    suLog(1, filename, "Finished preprocessing in ", peek_stopwatch(time), " ms", VTS_Default);
    return prefile;
}