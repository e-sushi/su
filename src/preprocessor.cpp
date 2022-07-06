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



                   
void Preprocessor::preprocess(){DPZoneScoped;
    suLogger& logger = sufile->logger; 
    logger.log(1, "Preprocessing...");

    Stopwatch time = start_stopwatch();
   
    //first we look for imports, if they are found we lex the file being imported from recursively
    //TODO(sushi) this can probably be multithreaded
    //we gather import paths so we can setup a job for each one 
    // and lex then preprocess each one at the same time.
    CompilerRequest cr;
    for(u32 importidx : sufile->lexer.imports){
        logger.log(2, "Processing imports");
        Token* curt = &sufile->lexer.tokens[importidx];
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
                        logger.log(2, "Adding import path ", curt->raw);
                        cr.filepaths.add(curt->raw);
                        
                    }else{
                        //TODO(sushi) look for imports on PATH
                        logger.warn(curt, "Finding files through PATH is not currently supported.");
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
        cr.stage = FileStage_Preprocessor;
        compiler.compile(&cr);
    }
    
    logger.log(2, "Finding internal declarations.");
    sufile->preprocessor.exported_decl = sufile->lexer.global_decl;
    for(u32 idx : sufile->lexer.internals){
        Token* curt = &sufile->lexer.tokens[idx];
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
            end = sufile->lexer.tokens.count;
        }   

        //move exported identifiers into internal
        forI(sufile->preprocessor.exported_decl.count){
            Declaration* id = sufile->preprocessor.exported_decl[i];
            u32 idx = id->token_start->idx;
            if(idx > start && idx < end){
                id->internal = 1;
                sufile->preprocessor.internal_decl.add(id);
                TestMe;
                sufile->preprocessor.exported_decl.remove_unordered(i);
            }
        }
        //local identifiers do not need to be declared internal
    }


    logger.log(2, "Finding run directives ", ErrorFormat("(NotImplemented)"));
    for(u32 idx : sufile->lexer.runs){
        logger.error(&sufile->lexer.tokens[idx], "#run is not implemented yet.");
    }
   
    if(globals.verbosity > 3){
        if(sufile->preprocessor.exported_decl.count)
            logger.log(4, "Exported declarations");
        for(Declaration* id : sufile->preprocessor.exported_decl){
            logger.log(4, "  ", id->declared_identifier);
        }

        if(sufile->preprocessor.internal_decl.count)
            logger.log(4, "Internal declarations");
        for(Declaration* id : sufile->preprocessor.internal_decl){
            logger.log(4, "  ", id->declared_identifier);
        }
    }
    


    
    logger.log(1, "Finished preprocessing in ", peek_stopwatch(time), " ms", VTS_Default);
}