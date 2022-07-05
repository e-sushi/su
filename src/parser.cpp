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

#define perror(token, ...)\
suError(token, __VA_ARGS__)

#define perror_goto(token, label, ...){\
suError(token, __VA_ARGS__);\
goto label;\
}


#define pwarn(token, ...)\
suWarn(token, __VA_ARGS__)

#define expect(...) if(curr_match(__VA_ARGS__))
#define expect_group(...) if(curr_match_group(__VA_ARGS__))


FORCE_INLINE void Parser::push_scope()     { stacks.nested.scopes.add(current.scope); current.scope = arena.make_scope(); }
FORCE_INLINE void Parser::pop_scope()      { current.scope = stacks.nested.scopes.pop(); }
FORCE_INLINE void Parser::push_function()  { stacks.nested.functions.add(current.function); current.function = arena.make_function(); }
FORCE_INLINE void Parser::pop_function()   { current.function = stacks.nested.functions.pop(); }
FORCE_INLINE void Parser::push_expression(){ stacks.nested.expressions.add(current.expression); current.expression = arena.make_expression(); }
FORCE_INLINE void Parser::pop_expression() { current.expression = stacks.nested.expressions.pop(); }
FORCE_INLINE void Parser::push_struct()    { stacks.nested.structs.add(current.structure); current.structure = arena.make_struct(); }
FORCE_INLINE void Parser::pop_struct()     { current.structure = stacks.nested.structs.pop(); }
FORCE_INLINE void Parser::push_variable()  { stacks.nested.variables.add(current.variable); current.variable = arena.make_variable(); }
FORCE_INLINE void Parser::pop_variable()   { current.variable = stacks.nested.variables.pop(); }


TNode* Parser::parse_import(){DPZoneScoped;
    array<Declaration*> imported_decls;
    curt++;
    expect(Token_OpenBrace){
        curt++; 
        expect(Token_LiteralString){
            str8 module_name = curt->raw;
            u32 path_loc;
            path_loc = str8_find_last(module_name, '/');
            if(path_loc == npos)
                path_loc = str8_find_last(module_name, '\\');
            if(path_loc != npos){
                module_name.str += path_loc+1;
                module_name.count -= path_loc+1;
            }
            
            if(!compiler.parsed_files.has(module_name)){
                CompilerRequest cr;
                cr.filepaths.add(curt->raw);
                //TODO(sushi) just make this Stage_Parser when supported, because we know by this point all files will have been lexed and parsed
                //also, we can probably start parsing early on files with no imports
                cr.stages = Stage_Lexer | Stage_Parser | Stage_Preprocessor;
                compiler.compile(&cr);
            }

            ParsedFile* pfile = &compiler.parsed_files[module_name];

            curt++;
            // expect(Token_OpenBrace){
            //     //we are including specific defintions from the import
            //     while(1){
            //         curt++;
            //         expect(Token_Identifier){
            //             //now we must make sure it exists
            //             Type type = 0;
            //             Declaration* decl = 0;
            //             forI(Max(pfile->decl.exported.funcs.count, Max(pfile->decl.exported.structs.count, pfile->decl.exported.vars.count))){
            //                 if(i < pfile->decl.exported.funcs.count && str8_equal(pfile->decl.exported.funcs[i]->decl.identifier, curt->raw)){
            //                     decl = &pfile->decl.exported.funcs[i]->decl;
            //                     break;           
            //                 }

            //                 if(i < pfile->decl.exported.structs.count && str8_equal(pfile->decl.exported.structs[i]->decl.identifier, curt->raw)){
            //                     decl = &pfile->decl.exported.structs[i]->decl;
            //                     break;           
            //                 }

            //                  if(i < pfile->decl.exported.vars.count && str8_equal(pfile->decl.exported.vars[i]->decl.identifier, curt->raw)){
            //                     decl = &pfile->decl.exported.vars[i]->decl;
            //                     break;           
            //                 }
            //             }

            //             if(!decl) perror(curt, "Attempted to include declaration ", curt->raw, " from module ", pfile->prefile->lexfile->file->front, " but it is either internal or doesn't exist.");
            //             else{
            //                 expect(Token_Identifier){
            //                     curt++;
            //                     if(str8_equal_lazy(STR8("as"), curt->raw)){
            //                         //in this case we must duplicate the declaration and give it a new identifier
            //                         //TODO(sushi) duplication may not be necessary, instead we can store a local name
            //                         //            and just use the same declaration node.
            //                         curt++;
            //                         expect(Token_Identifier){
            //                             switch(decl->type){
            //                                 case decl_structure:{
            //                                     Struct* s = arena.make_struct();
            //                                     memcpy(s, StructFromDeclaration(decl), sizeof(Struct));
            //                                     s->decl.identifier = curt->raw;
            //                                     decl = &s->decl;
            //                                 }break;
            //                                 case decl_function:{
            //                                     Function* s = arena.make_function();
            //                                     memcpy(s, FunctionFromDeclaration(decl), sizeof(Function));
            //                                     s->decl.identifier = curt->raw;
            //                                     decl = &s->decl;
            //                                 }break;
            //                                 case decl_variable:{
            //                                     Variable* s = arena.make_variable();
            //                                     memcpy(s, VariableFromDeclaration(decl), sizeof(Variable));
            //                                     s->decl.identifier = curt->raw;
            //                                     decl = &s->decl;
            //                                 }break;
            //                             }
            //                         }else perror(curt, "Expected an identifier after 'as'");
            //                     }else perror(curt, "Unknown identifier found after import specifier. Did you mean to use {} to specify defs? Did you mean to use 'as'?");
            //                 }
            //                 expect(Token_Comma){} 
            //                 else expect(Token_CloseBrace) {
            //                     break;
            //                 }else perror(curt, "Unknown token.");

            //             }
            //             imported_decls.add(decl);
            //         }else perror(curt, "Expected an identifier for including specific definitions from a module.");
            //     }
            // }else{
            //     //in this case the user isnt specifying anything specific so we import all public declarations from the module
            //      forI(Max(pfile->decl.exported.funcs.count, Max(pfile->decl.exported.structs.count, pfile->decl.exported.vars.count))){
            //         if(i < pfile->decl.exported.funcs.count){
            //             imported_decls.add(&pfile->decl.exported.funcs[i]->decl);
            //         }
            //         if(i < pfile->decl.exported.structs.count){
            //             imported_decls.add(&pfile->decl.exported.structs[i]->decl);
            //         }
            //         if(i < pfile->decl.exported.vars.count){
            //             imported_decls.add(&pfile->decl.exported.vars[i]->decl);
            //         }
            //     }
            // }
            // expect(Token_Identifier){
            //     //in this case the user must be using 'as' to give the module a namespace
            //     //so we make a new struct and add the declarations to it
            //     if(str8_equal_lazy(STR8("as"), curt->raw)){
            //         expect(Token_Identifier){
            //             Struct* s = arena.make_struct();
            //             Struct s_for_real;
            //             memcpy(s, &s_for_real, sizeof(Struct));
            //             s->decl.identifier = curt->raw;
            //             s->decl.token_start = curt;
            //             s->decl.token_end = curt;
            //             s->decl.type = decl_structure;
            //             forI(imported_decls.count){
            //                 switch(imported_decls[i]->type){
            //                     case decl_structure:{s->structs.add(StructFromDeclaration(imported_decls[i]));}break;
            //                     case decl_function :{s->funcs.add(FunctionFromDeclaration(imported_decls[i]));}break;
            //                     case decl_variable :{s->vars.add(VariableFromDeclaration(imported_decls[i]));}break;
            //                 }
            //             }
            //             stacks.known.structs.add(s);
            //             parfile->imported_identifiers.add();
            //         }else perror(curt, "Expected an identifier after 'as'");
            //     }else perror(curt, "Unknown identifier found after import specifier. Did you mean to use {} to specify defs? Did you mean to use 'as'?");
            // }
        }else perror(curt, "Import specifier is not a string or identifier.");
        
    }else expect(Token_LiteralString){

    }else perror(curt, "Import specifier is not a string or identifier.");

    return 0;
}

TNode* Parser::declare(Type type){DPZoneScoped;
    TNode* ret = 0;
    switch(type){
        case decl_structure:{
            // Struct* s = arena.make_struct();
            // s->decl.identifier = curt->raw;
            // s->decl.token_start = curt;
            // while(curt->type!=Token_CloseBrace){
            //     curt++;
            // }
            // s->decl.token_end = curt;
            // ret = &s->decl.node;
        }break;

        case decl_function:{
            // expect(Token_Identifier){
            //     Function* f = arena.make_function();
            //     f->decl.identifier = curt->raw;
            //     f->decl.token_start = curt;
            //     curt++;
            //     if(curt->type == Token_OpenParen){
            //         curt++;
            //         while(curt->type != Token_CloseParen){
            //             expect(Token_Identifier){
            //                 str8 id = curt->raw;
            //                 curt++;
            //                 expect(Token_Colon){
            //                     curt++;
            //                     expect_group(TokenGroup_Type){
                                    
            //                     }else expect(Token_Assignment){

            //                     }else perror_goto(curt, earlyout, "Expected either a type or assignment after colon in function parameter declaration.");
            //                 }else perror_goto(curt, earlyout, "Expected a : after identifer in function parameter declaration.");
            //             }else perror_goto(curt, earlyout, "Expected an identifier or ) in function declaration.");
            //             curt++;
            //             expect(Token_Comma){curt++;}
            //             else expect(Token_CloseParen){break;}
            //             else perror(curt, "Missing comma before function parameter declaration.");
            //         }
            //     }else perror(curt, "Expected ( before parameters in function declaration.");
            //     ret =  &f->decl.node;
            // } else {
            //     expect_group(TokenGroup_Keyword) {
            //         perror(curt, "Attempted to declare a function using a reserved keyword, '", curt->raw, "'");
            //     } else expect_group(TokenGroup_Type) {
            //         perror(curt, "Attempted to declare a function using a type keyword, '", curt->raw, "'");
            //     }
            // } 
        }break;
    }

earlyout:
    return ret;
}

TNode* Parser::define(TNode* node, ParseStage stage){DPZoneScoped;

    switch(stage){
        case psFile:{ //-------------------------------------------------------------------------------------------------File
            // i dont think this stage will ever be touched
        }break;

        case psDirective:{ //---------------------------------------------------------------------------------------Directive
            //there is no reason to check for invalid directives here because that is handled by lexer

        }break;

        case psRun:{ //---------------------------------------------------------------------------------------------------Run
            NotImplemented;
        }break;

        case psScope:{ //-----------------------------------------------------------------------------------------------Scope
            push_scope();
            current.scope->token_start = curt;
            TNode* me = &current.scope->node;
            insert_last(node, &current.scope->node);
            while(!next_match(Token_CloseBrace)){
                curt++;
                expect(Token_Identifier){
                    //first we look to see if this identifier is one that we already know of 


                    //we need to determine what we are doing with this identifier, specifically if we are declaring 
                    //something for it. so we save it and look ahead
                    Token* save = curt;
                    curt++;
                    expect(Token_OpenParen){
                        //skip to end of function id
                        while(!next_match(Token_CloseParen)) curt++;
                        if(next_match(Token_Colon)){
                            // we are declaring a function, so we set the token back to the identifier and call declare
                            Token* save2 = curt;  
                            curt = save;
                            TNode* n = declare(decl_function);
                            

                        }
                    }   

                }
            }
            pop_scope();
        }break;

        case psStatement:{ //---------------------------------------------------------------------------------------Statement

        }break;

        case psExpression:{ //-------------------------------------------------------------------------------------Expression

        }break;

        case psConditional:{ //-----------------------------------------------------------------------------------Conditional

        }break;

        case psLogicalOR:{ //---------------------------------------------------------------------------------------LogicalOR

        }break;

        case psLogicalAND:{ //-------------------------------------------------------------------------------------LogicalAND

        }break;

        case psBitwiseOR:{ //---------------------------------------------------------------------------------------BitwiseOR

        }break;

        case psBitwiseXOR:{ //-------------------------------------------------------------------------------------BitwiseXOR

        }break;

        case psBitwiseAND:{ //-------------------------------------------------------------------------------------BitwiseAND

        }break;

        case psEquality:{ //-----------------------------------------------------------------------------------------Equality

        }break;

        case psRelational:{ //-------------------------------------------------------------------------------------Relational

        }break;

        case psBitshift:{ //-----------------------------------------------------------------------------------------Bitshift

        }break;

        case psAdditive:{ //-----------------------------------------------------------------------------------------Additive

        }break;

        case psTerm:{ //-------------------------------------------------------------------------------------------------Term

        }break;

        case psFactor:{ //---------------------------------------------------------------------------------------------Factor

        }break;

    }


    return 0;
}

ParsedFile* Parser::parse(PreprocessedFile* prefile){DPZoneScoped;
    str8 filename = prefile->lexfile->file->name;
    
    Stopwatch time = start_stopwatch();
    suLog(1, filename, "Parsing ", VTS_BlueFg, prefile->lexfile->file->name, VTS_Default);

    LexedFile* lexfile = prefile->lexfile;
    File* file = lexfile->file;

    stacks.known.structs_pushed.add(0);
    stacks.known.functions_pushed.add(0);
    stacks.known.variables_pushed.add(0);

    suLog(2, filename, "Beginning parse from entry point \"main\".");
    //TODO(sushi) mark where the entry point is in lexing 
    //TODO(sushi) custom entrypoint names
   
    suLog(2, filename, "Checking that imported files are parsed");

    {// make sure that all imported modules have been parsed before parsing this one
     // then gather the defintions from them that this file wants to use
        forI(lexfile->preprocessor.imports.count){
            Token* itok = &lexfile->tokens[lexfile->preprocessor.imports[i]];
            if(!itok->scope_depth){
                curt = itok;
                parse_import();
            }
        }
        
        forI(prefile->imported_files.count){
            Parser subparser;
            subparser.parse(prefile->imported_files[i]);
        }
    }

    //TODO(sushi) custom entry point or build entry point
    if(prefile->exported_decl.has(STR8("main"))){

    } else if (prefile->internal_decl.has(STR8("main"))){

    } else {
        suError("Unable to find entry point 'main'. TODO(sushi) custom entry point name or entry point 'build'");
    }

    //TODO(sushi) custom entry point, or entry point 'build'
    // Decalration* id = prefile->internal_decl.at(STR8("main"));
    // if(!id) id = prefile->exported_decl.at(STR8("main"));
    // if(id){
    //     curt = id->token;
    // }else{
    //     suLog(0, "TODO MAKE THIS AN ERROR WHEN SUERROR IS MADE: unable to find entry point function.");
    // }

    //suLog(2, "Parsing top-level declarations");
    
    // {//declare all global declarations
    //     suLog(3, "Declaring global structs");
    //     for(u32 idx : prefile->decl.exported.structs){
    //         curt = &prefile->lexfile->tokens[idx];
    //         Struct* s = StructFromNode(declare(decl_structure));
    //         parfile->decl.exported.structs.add(s);
    //         (*stacks.known.structs_pushed.last)++;
    //         stacks.known.structs.add(s);
    //     }   
    //     suLog(3, "Declaring internal structs");
    //     for(u32 idx : prefile->decl.internal.structs){
    //         curt = &prefile->lexfile->tokens[idx];
    //         Struct* s = StructFromNode(declare(decl_structure));
    //         parfile->decl.internal.structs.add(s);
    //         (*stacks.known.structs_pushed.last)++;
    //         stacks.known.structs.add(s);
    //     }

    //     suLog(3, "Declaring global functions");
    //     for(u32 idx : prefile->decl.exported.funcs){
    //         curt = &prefile->lexfile->tokens[idx];
    //         Function* f = FunctionFromNode(declare(decl_function));
    //         parfile->decl.exported.funcs.add(f);
    //         (*stacks.known.functions_pushed.last)++;
    //         stacks.known.functions.add(f);
    //     }

    //     suLog(3, "Declaring internal functions");
    //     for(u32 idx : prefile->decl.internal.funcs){
    //         curt = &prefile->lexfile->tokens[idx];
    //         Function* f = FunctionFromNode(declare(decl_function));
    //         parfile->decl.internal.funcs.add(f);
    //         (*stacks.known.functions_pushed.last)++;
    //         stacks.known.functions.add(f);
    //     }

    //     suLog(3, "Declaring global vars");
    //     for(u32 idx : prefile->decl.exported.vars){
    //         curt = &prefile->lexfile->tokens[idx];
    //         Variable* v = VariableFromNode(declare(decl_variable));
    //         parfile->decl.exported.vars.add(v);
    //         (*stacks.known.variables_pushed.last)++;
    //         stacks.known.variables.add(v);
    //     }

    //     suLog(3, "Declaring internal vars");
    //     for(u32 idx : prefile->decl.internal.vars){
    //         curt = &prefile->lexfile->tokens[idx];
    //         Variable* v = VariableFromNode(declare(decl_variable));
    //         parfile->decl.internal.vars.add(v);
    //         (*stacks.known.variables_pushed.last)++;
    //         stacks.known.variables.add(v);
    //     }
    // }

    // {//define all global declarations
    //     for(u32 idx : prefile->decl.exported.structs){
    //         curt = &prefile->lexfile->tokens[idx];
    //         Struct* s = StructFromNode(declare(decl_structure));
    //         parfile->decl.exported.structs.add(s);
    //     }   
    //     suLog(3, "Declaring internal structs");
    //     for(u32 idx : prefile->decl.internal.structs){
    //         curt = &prefile->lexfile->tokens[idx];
    //         Struct* s = StructFromNode(declare(decl_structure));
    //         parfile->decl.internal.structs.add(s);
    //     }

    //     suLog(3, "Declaring global functions");
    //     for(u32 idx : prefile->decl.exported.funcs){
    //         curt = &prefile->lexfile->tokens[idx];
    //         Function* f = FunctionFromNode(declare(decl_function));
    //         parfile->decl.exported.funcs.add(f);
    //     }

    //     suLog(3, "Declaring internal functions");
    //     for(u32 idx : prefile->decl.internal.funcs){
    //         curt = &prefile->lexfile->tokens[idx];
    //         Function* f = FunctionFromNode(declare(decl_function));
    //         parfile->decl.internal.funcs.add(f);
    //     }

    //     suLog(3, "Declaring global vars");
    //     for(u32 idx : prefile->decl.exported.vars){
    //         curt = &prefile->lexfile->tokens[idx];
    //         Variable* v = VariableFromNode(declare(decl_variable));
    //         parfile->decl.exported.vars.add(v);
    //     }

    //     suLog(3, "Declaring internal vars");
    //     for(u32 idx : prefile->decl.internal.vars){
    //         curt = &prefile->lexfile->tokens[idx];
    //         Variable* v = VariableFromNode(declare(decl_variable));
    //         parfile->decl.internal.vars.add(v);
    //     }
    // }

    suLog(1, filename, VTS_GreenFg, "Finished parsing in ", peek_stopwatch(time), " ms", VTS_Default);
    return 0;

}