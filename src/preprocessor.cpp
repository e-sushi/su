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

    ThreadSetName(suStr8("preprocessing ", sufile->file->name));

    Stopwatch time = start_stopwatch();
   
    //first we look for imports, if they are found we lex the file being imported from recursively
    //TODO(sushi) this can probably be multithreaded
    //we gather import paths so we can setup a job for each one 
    // and lex then preprocess each one at the same time.
    logger.log(2, "Processing imports");
    CompilerRequest cr;
    for(u32 importidx : sufile->lexer.imports){
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
    
    logger.log(2, "Resolving colon tokens as possible valid declarations.");
    array<u32> decls_glob;
    array<u32> decls_loc; //this is really only for debug info, not skipping local tokens because they still need to be marked as declarations
    for(u32 idx : sufile->lexer.declarations){
        Token* tok = &sufile->lexer.tokens[idx];
        if((tok-1)->type == Token_Identifier){
            // this must be a variable declaration, so we check that the next token is either in token group 
            // type or is just another identifier, we check both because it is possible a struct type is being used
            // also it is possible for there to be no type specifier and an assignment instead
            // if it is none of these things we check if its a struct declaration, which is just simply checking if "struct" was found after :
            if((tok+1)->group == TokenGroup_Type || (tok+1)->type == Token_Identifier || (tok+1)->type == Token_Assignment){
                (tok-1)->is_declaration = 1;
                (tok-1)->decl_type = Declaration_Variable;
                if(tok->is_global) decls_glob.add((tok-1)->idx);
                else               decls_loc.add((tok-1)->idx);
            }
            else if((tok+1)->type == Token_StructDecl){
                (tok-1)->is_declaration = 1;
                (tok-1)->decl_type = Declaration_Structure;
                if(tok->is_global) decls_glob.add((tok-1)->idx);
                else               decls_loc.add((tok-1)->idx);
            }
        }else if((tok-1)->type == Token_CloseParen){
            // this must be a function declaration, so we check that the next token is either in a token group type
            // or is another identifier
            if((tok+1)->group == TokenGroup_Type || (tok+1)->type == Token_Identifier){
                // now we look back for the function's identifier
                Token* cur = tok;
                while(cur->type != Token_OpenParen){
                    if(!cur->idx){
                        //TODO(sushi) this error probably isnt valid after custom operators are implemented
                        logger.error(tok, "Malformed syntax at start of file. ')' followed by ':' followed by a type or identifier implies a function declaration (for now). TODO(sushi) need better detection of what is happening here.");
                        break;
                    }
                    cur--;
                }
                cur--;
                if(cur->type == Token_Identifier){
                    cur->is_declaration = 1;
                    cur->decl_type = Declaration_Function;
                    if(tok->is_global) decls_glob.add(cur->idx);
                    else               decls_loc.add(cur->idx);
                }
            }
        }
    }

    logger.log(2, "Finding internal declarations.");
    sufile->preprocessor.exported_decl = decls_glob;
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
                    else scope_depth--;
                }
            }
            end = idx;
        }else{
            //the rest of the file is considered internal
            end = sufile->lexer.tokens.count;
        }   

        //move exported identifiers into internal
        forI(decls_glob.count){
            if(decls_glob[i] > start && decls_glob[i] < end){
                sufile->preprocessor.internal_decl.add(decls_glob[i]);
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

        auto decltypestr = [](Type type){
            switch(type){
                case Declaration_Function:  return STR8("func");
                case Declaration_Variable:  return STR8("var");
                case Declaration_Structure: return STR8("struct");

            }
            return STR8(ErrorFormat("UNKNOWN"));
        };

        if(sufile->preprocessor.exported_decl.count)
            logger.log(4, "Exported declarations");
        for(u32 idx : sufile->preprocessor.exported_decl){
            logger.log(4, "  ", sufile->lexer.tokens[idx].raw, " : ", decltypestr(sufile->lexer.tokens[idx].decl_type));
        }

        if(sufile->preprocessor.internal_decl.count)
            logger.log(4, "Internal declarations");
        for(u32 idx : sufile->preprocessor.internal_decl){
            logger.log(4, "  ", sufile->lexer.tokens[idx].raw, " : ", decltypestr(sufile->lexer.tokens[idx].decl_type));
        }

        if(decls_loc.count)
            logger.log(4, "Local declarations");
        for(u32 idx : decls_loc){
            logger.log(4, "  ", sufile->lexer.tokens[idx].raw, " : ", decltypestr(sufile->lexer.tokens[idx].decl_type));
        }
    }
    


    
    logger.log(1, "Finished preprocessing in ", peek_stopwatch(time), " ms", VTS_Default);
}