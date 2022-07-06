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
sufile->logger.error(token, __VA_ARGS__)

#define perror_goto(token, label, ...){\
sufile->logger.error(token, __VA_ARGS__);\
goto label;\
}

#define expect(...) if(curr_match(__VA_ARGS__))
#define expect_group(...) if(curr_match_group(__VA_ARGS__))

//TODO(sushi) probably just make these macros
//also i dont think this works well in general so get rid of it
FORCE_INLINE void Parser::push_scope()     {DPZoneScoped; stacks.nested.scopes.add(current.scope); current.scope = arena.make_scope(); }
FORCE_INLINE void Parser::pop_scope()      {DPZoneScoped; current.scope = stacks.nested.scopes.pop(); }
FORCE_INLINE void Parser::push_function()  {DPZoneScoped; stacks.nested.functions.add(current.function); current.function = arena.make_function(); }
FORCE_INLINE void Parser::pop_function()   {DPZoneScoped; current.function = stacks.nested.functions.pop(); }
FORCE_INLINE void Parser::push_expression(){DPZoneScoped; stacks.nested.expressions.add(current.expression); current.expression = arena.make_expression(); }
FORCE_INLINE void Parser::pop_expression() {DPZoneScoped; current.expression = stacks.nested.expressions.pop(); }
FORCE_INLINE void Parser::push_struct()    {DPZoneScoped; stacks.nested.structs.add(current.structure); current.structure = arena.make_struct(); }
FORCE_INLINE void Parser::pop_struct()     {DPZoneScoped; current.structure = stacks.nested.structs.pop(); }
FORCE_INLINE void Parser::push_variable()  {DPZoneScoped; stacks.nested.variables.add(current.variable); current.variable = arena.make_variable(); }
FORCE_INLINE void Parser::pop_variable()   {DPZoneScoped; current.variable = stacks.nested.variables.pop(); }


TNode* Parser::parse_import(){DPZoneScoped;
    array<Declaration*> imported_decls;
    curt++;
    expect(Token_OpenBrace){
        curt++; 
        expect(Token_LiteralString){
            str8 filename = curt->raw;
            u32 path_loc;
            path_loc = str8_find_last(filename, '/');
            if(path_loc == npos)
                path_loc = str8_find_last(filename, '\\');
            if(path_loc != npos){
                filename.str += path_loc+1;
                filename.count -= path_loc+1;
            }

            suFile* sufileex = compiler.files.at(filename);
             
            if(!sufileex || sufileex->stage < FileStage_Parser){
                //this warning indicates that something isnt behaving right in the compiler
                //if its long after 2022/07/05 then this warning can probably be removed
                if(!sufileex) compiler.logger.warn("INTERNAL: A file path was found in parsing that does not exist yet. All files that are dependencies of other files should be lexed and preprocessed before files that depend on them are parsed.");
                CompilerRequest cr;
                cr.filepaths.add(curt->raw);
                cr.stage = FileStage_Parser;
                compiler.compile(&cr);
            }

            curt++;
            expect(Token_OpenBrace){
                //we are including specific defintions from the import
                while(1){
                    curt++;
                    expect(Token_Identifier){
                        //now we must make sure it exists
                        Type type = 0;
                        Declaration* decl = 0;
                        sufileex->parser.find_identifier_externally(curt->raw);
                        if(!decl) perror(curt, "Attempted to include declaration '", curt->raw, "' from module '", sufileex->file->front, "' but it is either internal or doesn't exist.");
                        else{
                            expect(Token_As){
                                //in this case we must duplicate the declaration and give it a new identifier
                                //TODO(sushi) duplication may not be necessary, instead we can store a local name
                                //            and just use the same declaration node.
                                curt++;
                                expect(Token_Identifier){
                                    switch(decl->type){
                                        case Declaration_Structure:{
                                            Struct* s = arena.make_struct();
                                            memcpy(s, StructFromDeclaration(decl), sizeof(Struct));
                                            s->decl.identifier = curt->raw;
                                            decl = &s->decl;
                                        }break;
                                        case Declaration_Function:{
                                            Function* s = arena.make_function();
                                            memcpy(s, FunctionFromDeclaration(decl), sizeof(Function));
                                            s->decl.identifier = curt->raw;
                                            decl = &s->decl;
                                        }break;
                                        case Declaration_Variable:{
                                            Variable* s = arena.make_variable();
                                            memcpy(s, VariableFromDeclaration(decl), sizeof(Variable));
                                            s->decl.identifier = curt->raw;
                                            decl = &s->decl;
                                        }break;
                                    }
                                }else perror(curt, "Expected an identifier after 'as'");
                            }else{
                                
                            }
                        }
                        expect(Token_Comma){} 
                        else expect(Token_CloseBrace) {
                            break;
                        }else perror(curt, "Unknown token.");

 
                        imported_decls.add(decl);
                    }else perror(curt, "Expected an identifier for including specific definitions from a module.");
                }
            }else{
                //in this case the user isnt specifying anything specific so we import all public declarations from the module
                imported_decls.add_array(sufileex->parser.exported_decl);
            }
            expect(Token_As){
                curt++;
                //in this case the user must be using 'as' to give the module a namespace
                //so we make a new struct and add the declarations to it
                expect(Token_Identifier){
                    Struct* s = arena.make_struct();
                    *s = Struct(); 
                    s->decl.identifier = curt->raw;
                    s->decl.declared_identifier = curt->raw;
                    s->decl.token_start = curt;
                    s->decl.token_end = curt;
                    s->decl.type = Declaration_Structure;
                    forI(imported_decls.count){
                        switch(imported_decls[i]->type){
                            case Declaration_Structure:{s->structs.add(StructFromDeclaration(imported_decls[i]));}break;
                            case Declaration_Function :{s->funcs.add(FunctionFromDeclaration(imported_decls[i]));}break;
                            case Declaration_Variable :{s->vars.add(VariableFromDeclaration(imported_decls[i]));}break;
                        }
                    }
                    stacks.known.structs.add(s);
                    sufile->parser.imported_decl.add(s->decl.identifier, &s->decl);
                }else perror(curt, "Expected an identifier after 'as'");
            }else{
                //we are just importing all decls into the global space
                stacks.declarations.add_array(imported_decls);
                (*stacks.declarations_pushed.last) += imported_decls.count;
            }
        }else perror(curt, "Import specifier is not a string or identifier.");
        
    }else expect(Token_LiteralString){

    }else perror(curt, "Import specifier is not a string or identifier.");

    return 0;
}

TNode* Parser::declare(Type type){DPZoneScoped;
    TNode* ret = 0;
    switch(type){
        case Declaration_Structure:{
            // Struct* s = arena.make_struct();
            // s->decl.identifier = curt->raw;
            // s->decl.token_start = curt;
            // while(curt->type!=Token_CloseBrace){
            //     curt++;
            // }
            // s->decl.token_end = curt;
            // ret = &s->decl.node;
        }break;

        case Declaration_Function:{
            expect(Token_Identifier){//NOTE(sushi) the failure of this check indicates something wrong internally
                str8 id = curt->raw;
                curt++;
                expect(Token_OpenParen){
                    while(!next_match(Token_CloseParen)){
                        curt++;
                        expect(Token_Identifier){
                            Variable* v = VariableFromNode(declare(Declaration_Variable));

                        } else perror(curt, "expected identifier for function parameter declaration.");
                    }
                } else perror(curt, "expected ( after identifier in function declaration.");
            } else perror(curt, "perror() was called with Declaration_Function, but the initial token is not an identifier.");
        }break;

        case Declaration_Variable:{
            expect(Token_Identifier){//NOTE(sushi) the failure of this check indicates something wrong internally
                str8 id = curt->raw;
                curt++;
                expect(Token_Colon){
                    curt++;
                    expect(TokenGroup_Type){

                    } else expect(Token_Identifier) {
                        //in this case previous stages were unable to mark this token as a struct, so we must look for it here

                    }
                } else perror(curt, "Expected a ':' after identifier for variable decalration.");
            } else perror(curt, "perror() was called with Declaration_Variable, but the initial token is not an identifier.");
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
                            TNode* n = declare(Declaration_Function);
                            

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

void Parser::parse(){DPZoneScoped;
    Stopwatch time = start_stopwatch();
    sufile->logger.log(1, "Parsing...");

    stacks.known.structs_pushed.add(0);
    stacks.known.functions_pushed.add(0);
    stacks.known.variables_pushed.add(0);
   
    sufile->logger.log(2, "Checking that imported files are parsed");

    {// make sure that all imported modules have been parsed before parsing this one
     // then gather the defintions from them that this file wants to use
        forI(sufile->lexer.imports.count){
            Token* itok = &sufile->lexer.tokens[sufile->lexer.imports[i]];
            if(!itok->scope_depth){
                curt = itok;
                parse_import();
            }
        }
        
        forI(sufile->preprocessor.imported_files.count){
            if(sufile->preprocessor.imported_files[i]->stage < FileStage_Parser){
                compiler.logger.warn("INTERNAL: somehow a file wasnt parsed after the initial check above ", __FILENAME__, ", line ", __LINE__);
            }
        }
    }

   
    forI(sufile->preprocessor.internal_decl.count){
        Declaration* decl = sufile->preprocessor.internal_decl[i];
        sufile->logger.log(4, "Parsing internal declaration '", decl->identifier, "'");
        curt = decl->token_start;
        TNode* n = declare(decl->type);
    }

    forI(sufile->preprocessor.exported_decl.count){
        Declaration* decl = sufile->preprocessor.exported_decl[i];
        sufile->logger.log(4, "Parsing exported declaration '", decl->identifier, "'");
        curt = decl->token_start;
        TNode* n = declare(decl->type);
    }
    
    sufile->logger.log(1, VTS_GreenFg, "Finished parsing in ", peek_stopwatch(time), " ms", VTS_Default);
}