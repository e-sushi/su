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
LogE("", ErrorFormat("error: "), (token).file, "(",(token).l0,",",(token).c0,"):", __VA_ARGS__)

#define preprocess_warn(token, ...)\
Log("", WarningFormat("warning: "), (token).file, "(",(token).l0,",",(token).c0,"):", __VA_ARGS__)

PreprocessedFile* Preprocessor::preprocess(LexedFile* lexfile){
    Log("", "Preprocessing...");
    logger_push_indent();
    if(preprocessed_files.has(lexfile->file->front)){
        Log("", "File has already been preprocessed.");
        logger_pop_indent();
        return &preprocessed_files[lexfile->file->front];
    } 

    Stopwatch time = start_stopwatch();
    
    preprocessed_files.add(lexfile->file->front);
    prefile = preprocessed_files.data.last;
    prefile->lexfile = lexfile;

    //first we look for imports, if they are founda we lex the file being imported from recursively
    //TODO(sushi) this can probably be multithreaded
    for(u32 importidx : lexfile->preprocessor.imports){
        Log("", "Processing imports");
        logger_push_indent();
        Token* curt = &lexfile->tokens[importidx];
        curt++;
        if(curt->type == Token_OpenBrace){
            //multiline directive 
            while(curt->type != Token_CloseBrace){
                if(curt->type == Token_LiteralString){
                    Token* mod = curt;
                    
                    str8 check;
                    if(curt->type == Token_Identifier){
                        check = str8_concat(curt->raw, STR8(".su"), deshi_temp_allocator);
                    }else{
                        check = curt->raw;
                    }

                    //attempt to find the module in import paths and current working directory
                    //first check cwd
                    if(file_exists(check)){
                        preprocess(lexer.lex(check));
                        Log("", "Preprocessing -> ", VTS_BlueFg, lexfile->file->name, VTS_Default);
                        logger_push_indent(2);
                        
                    }else{
                        //TODO(sushi) import paths
                        preprocess_warn(*curt, "Finding files through PATH or import paths is not currently supported.");
                    }
                    //NOTE(sushi) we do not handle selective imports here, that is handled in parsing
                    curt++;
                    if(curt->type = Token_OpenBrace){
                        while(curt->type != Token_CloseBrace){curt++;}
                    }
                }else{

                }
                curt++;
            }

        }
        logger_pop_indent();
    }
    
    prefile->decl.exported.vars = lexfile->decl.glob.vars;
    prefile->decl.exported.funcs = lexfile->decl.glob.funcs;
    prefile->decl.exported.structs = lexfile->decl.glob.structs;
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
        
        forI(Max(prefile->decl.exported.vars.count, Max(prefile->decl.exported.funcs.count, prefile->decl.exported.structs.count))){
            if(i < prefile->decl.exported.vars.count){
                if(prefile->decl.exported.vars[i] > start && prefile->decl.exported.vars[i] < end){
                    prefile->decl.internal.vars.add(prefile->decl.exported.vars[i]);
                    prefile->decl.exported.vars.remove_unordered(i);
                }
            }

            if(i < prefile->decl.exported.funcs.count){
                if(prefile->decl.exported.funcs[i] > start && prefile->decl.exported.funcs[i] < end){
                    prefile->decl.internal.funcs.add(prefile->decl.exported.funcs[i]);
                    prefile->decl.exported.funcs.remove_unordered(i);
                }
            }

            if(i < prefile->decl.exported.structs.count){
                if(prefile->decl.exported.structs[i] > start && prefile->decl.exported.structs[i] < end){
                    prefile->decl.internal.structs.add(prefile->decl.exported.structs[i]);
                    prefile->decl.exported.structs.remove_unordered(i);
                }
            }        
        }

    }

    
    logger_push_indent();
    if(prefile->decl.exported.vars.count)
        Log("", "Exported vars are: ");
    logger_push_indent();
    forI(prefile->decl.exported.vars.count){
        Log("", "  ", lexfile->tokens[prefile->decl.exported.vars[i]-2].raw);
    }
    logger_pop_indent();

    if(prefile->decl.exported.funcs.count)
        Log("", " Exported funcs are: ");
    logger_push_indent();
    forI(prefile->decl.exported.funcs.count){
        Log("", "  ", lexfile->tokens[prefile->decl.exported.funcs[i]].raw);
    }
    logger_pop_indent();

    if(prefile->decl.exported.structs.count)
        Log("", " Exported structs are: ");
    logger_push_indent();
    forI(prefile->decl.exported.structs.count){
        Log("", "  ", lexfile->tokens[prefile->decl.exported.structs[i]-2].raw);
    }
    logger_pop_indent();

    if(prefile->decl.internal.vars.count)
        Log("", " Internal vars are: ");
    logger_push_indent();
    forI(prefile->decl.internal.vars.count){
        Log("", "  ", lexfile->tokens[prefile->decl.internal.vars[i]-2].raw);
    }
    logger_pop_indent();

    if(prefile->decl.internal.funcs.count)
        Log("", " Internal funcs are: ");
    logger_push_indent();
    forI(prefile->decl.internal.funcs.count){
        Log("", "  ", lexfile->tokens[prefile->decl.internal.funcs[i]].raw);
    }
    logger_pop_indent();

    if(prefile->decl.internal.structs.count)
        Log("", " Internal structs are: ");
    logger_push_indent();
    forI(prefile->decl.internal.structs.count){
        Log("", "  ", lexfile->tokens[prefile->decl.internal.structs[i]-2].raw);
    }
    logger_pop_indent();
    logger_pop_indent();

    for(u32 idx : lexfile->preprocessor.runs){
        preprocess_error(lexfile->tokens[idx], "#run is not implemented yet.");
    }

    Log("", VTS_GreenFg, "Finished preprocessing in ", peek_stopwatch(time), " ms", VTS_Default);
    logger_pop_indent(2);

    return prefile;
}