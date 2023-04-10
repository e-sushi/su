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
    // amuLogger& logger = amufile->logger; 
    // logger.log(Verbosity_Stages, "Preprocessing...");

    // SetThreadName("Preprocessing ", amufile->file->name);

    // Stopwatch time = start_stopwatch();
   
    //TODO(sushi) this can probably be multithreaded
    // logger.log(Verbosity_StageParts, "Processing imports");
    // CompilerRequest cr;
    // for(u32 importidx : amufile->lexer.imports){
    //     Token* curt = amufile->lexer.tokens.readptr(importidx);
    //     curt++;
    //     if(curt->type == Token_LiteralString){
    //         if(!file_exists(curt->raw)){
    //             logger.error("the path ", curt->raw, " could not be found.");
    //             logger.note("searching PATH is not implemented yet.");
    //             continue;
    //         }
    //         //we have to make sure that a filepath doesnt get added twice
    //         //this can happen in a case where someone uses import on the same path
    //         //but imports in different scopes or specifies different subimports
    //         //TODO(sushi) maybe theres a better way to handle this?
    //         b32 found = 0;
    //         forI(cr.filepaths.count){
    //             if(str8_equal_lazy(curt->raw, cr.filepaths[i])){
    //                 found = 1;
    //             }
    //         }
    //         if(!found){
    //             logger.log(Verbosity_StageParts, "Adding import path ", curt->raw);
    //             cr.filepaths.add(curt->raw);
    //         } 
    //     }else if(curt->type == Token_Identifier){
    //         str8 path = amuStr8(compiler.working_dir, curt->raw, ".amu");
    //         if(!file_exists(path)){
    //             logger.error("the module ", curt->raw, " could not be found in the working directory.");
    //             logger.note("searching PATH in not implemented yet.");
    //             continue;
    //         }
    //         cr.filepaths.add(path);
    //     }else if(curt->type == Token_OpenBrace){
    //         NotImplemented; 
    //         //TODO(sushi) multiple imports, or just don't allow it at all
    //     }
    // }
    
    // if(cr.filepaths.count){
    //     cr.stage = FileStage_Validator;
    //     CompilerReport report = compiler.compile(&cr, 0);
    //     if(report.failed) return;
    //     amufile->preprocessor.imported_files = report.units;
    // }
    
    // logger.log(Verbosity_StageParts, "Resolving colon tokens as possible valid declarations.");
    // amuArena<u32> decls_glob;
    // amuArena<u32> decls_loc; //this is really only for debug info, not skipping local tokens because they still need to be marked as declarations
    // decls_glob.init();
    // decls_loc.init();
    // for(u32 idx : amufile->lexer.declarations){
    //     Token* tok = amufile->lexer.tokens.readptr(idx);

    //     if((tok-1)->type == Token_Identifier){
    //         // this is either a variable or struct declaration
    //         if((tok+1)->group == TokenGroup_Type || (tok+1)->type == Token_Assignment){
    //             //this is a normal declaration using a built-in type, or it is implicitly typed
    //             (tok-1)->is_declaration = 1;
    //             (tok-1)->decl_type = Declaration_Variable;
    //             if(tok->is_global) decls_glob.add((tok-1)->idx);
    //             else               decls_loc.add((tok-1)->idx);
    //         }else if((tok+1)->type == Token_Identifier){
    //             // this is most likely a variable declaration, but we have to check if the type specifier is actually
    //             // a declaration of a structure
    //             if((tok+2)->type == Token_Colon){
    //                 // this colon will be examined again, though, but this time it will be seen as a struct 
    //                 // definition, so we need to check in that case if the identifier is preceded by a colon,
    //                 // and if it is, do nothing, because we've already handled it here.
    //                 if((tok+3)->type != Token_StructDecl){
    //                     amufile->preprocessor.failed = 1;
    //                     if((tok+3)->type == Token_ModuleDecl){
    //                         logger.error(tok+3, "the type of a variable cannot be a module.");
    //                         break;
    //                     }
    //                     logger.error(tok+3, "unexpected token in variable declaration type specifier.");
    //                     break;
    //                 }
    //             }
    //             (tok-1)->is_declaration = 1;
    //             (tok-1)->decl_type = Declaration_Variable;
    //             if(tok->is_global) decls_glob.add((tok-1)->idx);
    //             else               decls_loc.add((tok-1)->idx);
    //         }else if((tok+1)->type == Token_Colon){
    //             // this is a variable declaration that is using an anonymous struct declaration as its type specifier
    //             // we do the same procedure as above
    //             if((tok+2)->type != Token_StructDecl){
    //                 amufile->preprocessor.failed = 1;
    //                 if((tok+2)->type == Token_ModuleDecl){
    //                         logger.error(tok+2, "the type of a variable cannot be a namespace.");
    //                         break;
    //                     }
    //                 logger.error(tok+2, "unexpected token in variable declaration type specifier.");
    //                 break;
    //             }
    //             (tok-1)->is_declaration = 1;
    //             (tok-1)->decl_type = Declaration_Variable;
    //             if(tok->is_global) decls_glob.add((tok-1)->idx);
    //             else               decls_loc.add((tok-1)->idx);
    //         }else if((tok+1)->type == Token_StructDecl){
    //             // this is a struct declaration, so, as mentioned above, we must check if this is being used as a type specifier
    //             // if so, we don't do anything. this declaration will be handled when the variable declaration is parsed
    //             (tok-1)->is_declaration = 1;
    //             (tok-1)->decl_type = Declaration_Structure;
    //             if((tok-2)->type != Token_Colon){
    //                 if(tok->is_global) decls_glob.add((tok-1)->idx);
    //                 else               decls_loc.add((tok-1)->idx);
    //             }
    //         }
    //     }else if((tok+1)->type == Token_StructDecl){
    //         // this must be an anonymous struct declaration, but we have to check if it is the type specifier of 
    //         // a variable declaration. if it is, we don't do anything.
    //         tok->is_declaration = 1;
    //         tok->decl_type = Declaration_Structure;
    //         if((tok-1)->type != Token_Colon){
    //             if(tok->is_global) decls_glob.add((tok-1)->idx);
    //             else               decls_loc.add((tok-1)->idx);
    //         }
    //     }else if((tok-1)->type == Token_CloseParen){
    //         //this must be a function decl. we do not check the right side of the colon here, instead we just error in parsing
    //         Token* cur = tok;
    //         while(cur->type != Token_OpenParen){
    //             if(!cur->idx){
    //                 logger.error(tok, "Malformed syntax at start of file. ')' followed by ':' followed by a type or identifier implies a function declaration.");
    //                 amufile->preprocessor.failed = 1;
    //                 break;
    //             }
    //             cur--;
    //         }
    //         cur--;
    //         if(cur->type == Token_Identifier){
    //             cur->is_declaration = 1;
    //             cur->decl_type = Declaration_Function;
    //             if(tok->is_global) decls_glob.add(cur->idx);
    //             else               decls_loc.add(cur->idx);
    //         }
    //     }
    // }

    // logger.log(Verbosity_StageParts, "Finding internal declarations.");
    // amufile->preprocessor.exported_decl = decls_glob;
    // for(u32 idx : amufile->lexer.internals){
    //     Token* curt = amufile->lexer.tokens.readptr(idx);
    //     curt++;
    //     u32 start = idx, end;
    //     if(curt->type == Token_OpenBrace){
    //         //scoped internal, we must find its extent
    //         u32 scope_depth = 0;
    //         while(1){
    //             curt++; idx++;
    //             if(curt->type == Token_OpenBrace) scope_depth++;
    //             else if(curt->type == Token_CloseBrace){
    //                 if(!scope_depth) break;
    //                 else scope_depth--;
    //             }
    //         }
    //         end = idx;
    //     }else{
    //         //the rest of the file is considered internal
    //         end = amufile->lexer.tokens.count;
    //     }

    //     //move exported identifiers into internal
    //     forI(decls_glob.count){
    //         if(decls_glob[i] > start && decls_glob[i] < end){
    //             amufile->preprocessor.internal_decl.add(decls_glob[i]);
    //             amufile->preprocessor.exported_decl.remove_unordered(i);
    //         }
    //     }
    //     //local identifiers do not need to be declared internal
    // }


    // logger.log(Verbosity_StageParts, "Finding run directives ", ErrorFormat("(NotImplemented)"));
    // for(u32 idx : amufile->lexer.runs){
    //     logger.error(&amufile->lexer.tokens[idx], "#run is not implemented yet.");
    //     amufile->preprocessor.failed = 1;
    // }

    // if(globals.verbosity >= Verbosity_Debug){

    //     auto decltypestr = [](Type type){
    //         switch(type){
    //             case Declaration_Function:  return STR8("func");
    //             case Declaration_Variable:  return STR8("var");
    //             case Declaration_Structure: return STR8("struct");
    //         }
    //         return STR8(ErrorFormat("UNKNOWN"));
    //     };

    //     if(amufile->preprocessor.exported_decl.count)
    //         logger.log(Verbosity_Debug, "Exported declarations");
    //     for(u32 idx : amufile->preprocessor.exported_decl){
    //         logger.log(Verbosity_Debug, "  ", amufile->lexer.tokens[idx].raw, " : ", decltypestr(amufile->lexer.tokens[idx].decl_type));
    //     }

    //     if(amufile->preprocessor.internal_decl.count)
    //         logger.log(Verbosity_Debug, "Internal declarations");
    //     for(u32 idx : amufile->preprocessor.internal_decl){
    //         logger.log(Verbosity_Debug, "  ", amufile->lexer.tokens[idx].raw, " : ", decltypestr(amufile->lexer.tokens[idx].decl_type));
    //     }

    //     if(decls_loc.count)
    //         logger.log(Verbosity_Debug, "Local declarations");
    //     for(u32 idx : decls_loc){
    //         logger.log(Verbosity_Debug, "  ", amufile->lexer.tokens[idx].raw, " : ", decltypestr(amufile->lexer.tokens[idx].decl_type));
    //     }
    // }
    
    // logger.log(Verbosity_Stages, "Finished preprocessing in ", peek_stopwatch(time), " ms", VTS_Default);
}