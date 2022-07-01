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

PreprocessedFile* preprocess(LexedFile* lfile){
    Log("", "Preprocessing...");
    logger_push_indent();
    if(preprocessor.files.has(lfile->file->front)){
        Log("", "File has already been preprocessed.");
        logger_pop_indent();
        return 0;
    } 

    Stopwatch time = start_stopwatch();
    
    preprocessor.files.add(lfile->file->front);
    PreprocessedFile* pfile = preprocessor.files.data.last;
    pfile->lfile = lfile;

    //first we look for imports, if they are found we lex the file being imported from recursively
    //TODO(sushi) this can probably be multithreaded
    for(u32 importidx : lfile->preprocessor.imports){
        Log("", "Processing imports");
        logger_push_indent();
        Token* curt = &lfile->tokens[importidx];
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
                        preprocess(lex_file(check));
                        Log("", "Preprocessing -> ", VTS_BlueFg, lfile->file->name, VTS_Default);
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
    
    pfile->decl.exported.vars = lfile->decl.glob.vars;
    pfile->decl.exported.funcs = lfile->decl.glob.funcs;
    pfile->decl.exported.structs = lfile->decl.glob.structs;
    for(u32 idx : lfile->preprocessor.internals){
        Token* curt = &lfile->tokens[idx];
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
            end = lfile->tokens.count;
        }
        
        forI(Max(pfile->decl.exported.vars.count, Max(pfile->decl.exported.funcs.count, pfile->decl.exported.structs.count))){
            if(i < pfile->decl.exported.vars.count){
                if(pfile->decl.exported.vars[i] > start && pfile->decl.exported.vars[i] < end){
                    pfile->decl.internal.vars.add(pfile->decl.exported.vars[i]);
                    pfile->decl.exported.vars.remove_unordered(i);
                }
            }

            if(i < pfile->decl.exported.funcs.count){
                if(pfile->decl.exported.funcs[i] > start && pfile->decl.exported.funcs[i] < end){
                    pfile->decl.internal.funcs.add(pfile->decl.exported.funcs[i]);
                    pfile->decl.exported.funcs.remove_unordered(i);
                }
            }

            if(i < pfile->decl.exported.structs.count){
                if(pfile->decl.exported.structs[i] > start && pfile->decl.exported.structs[i] < end){
                    pfile->decl.internal.structs.add(pfile->decl.exported.structs[i]);
                    pfile->decl.exported.structs.remove_unordered(i);
                }
            }        
        }

    }
    
    logger_push_indent();
    if(pfile->decl.exported.vars.count)
        Log("", "Exported vars are: ");
    logger_push_indent();
    forI(pfile->decl.exported.vars.count){
        Log("", "  ", lfile->tokens[pfile->decl.exported.vars[i]-2].raw);
    }
    logger_pop_indent();

    if(pfile->decl.exported.funcs.count)
        Log("", " Exported funcs are: ");
    logger_push_indent();
    forI(pfile->decl.exported.funcs.count){
        Token* curt = &lfile->tokens[pfile->decl.exported.funcs[i]];
        while(curt->type != Token_OpenParen) curt--;
        Log("", "  ", (--curt)->raw);
    }
    logger_pop_indent();

    if(pfile->decl.exported.structs.count)
        Log("", " Exported structs are: ");
    logger_push_indent();
    forI(pfile->decl.exported.structs.count){
        Log("", "  ", lfile->tokens[pfile->decl.exported.structs[i]-2].raw);
    }
    logger_pop_indent();

    if(pfile->decl.internal.vars.count)
        Log("", " Internal vars are: ");
    logger_push_indent();
    forI(pfile->decl.internal.vars.count){
        Log("", "  ", lfile->tokens[pfile->decl.internal.vars[i]-2].raw);
    }
    logger_pop_indent();

    if(pfile->decl.internal.funcs.count)
        Log("", " Internal funcs are: ");
    logger_push_indent();
    forI(pfile->decl.internal.funcs.count){
        Token* curt = &lfile->tokens[pfile->decl.internal.funcs[i]];
        while(curt->type != Token_OpenParen) curt--;
        Log("", "  ", (--curt)->raw);
    }
    logger_pop_indent();

    if(pfile->decl.internal.structs.count)
        Log("", " Internal structs are: ");
    logger_push_indent();
    forI(pfile->decl.internal.structs.count){
        Log("", "  ", lfile->tokens[pfile->decl.internal.structs[i]-2].raw);
    }
    logger_pop_indent();
    logger_pop_indent();

    for(u32 idx : lfile->preprocessor.runs){
        preprocess_error(lfile->tokens[idx], "#run is not implemented yet.");
    }

    Log("", VTS_GreenFg, "Finished preprocessing in ", peek_stopwatch(time), " ms", VTS_Default);
    logger_pop_indent(2);

    return pfile;
}