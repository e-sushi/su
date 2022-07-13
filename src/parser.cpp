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

#define perror_ret(token, ...){\
sufile->logger.error(token, __VA_ARGS__);\
return 0;\
}

#define expect(...) if(curr_match(__VA_ARGS__))
#define expect_group(...) if(curr_match_group(__VA_ARGS__))

TNode* ParserThread::define(TNode* node, Type stage){DPZoneScoped;
    sufile->logger.log(4, "Parsing token ", curt->raw, " in ", curt->file, "(",curt->l0,",",curt->c0,")", " on stage ", psStrs[stage]);
    ThreadSetName(suStr8("parsing ", curt->raw, " in ", curt->file));
    switch(stage){
        case psFile:{ //-------------------------------------------------------------------------------------------------File
            // i dont think this stage will ever be touched
        }break;

        case psDirective:{ //---------------------------------------------------------------------------------------Directive
            // there is no reason to check for invalid directives here because that is handled by lexer

        }break;

        case psImport:{ //---------------------------------------------------------------------------------------------Import
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
                        imported_decls.reserve(sufileex->parser.exported_decl.data.count);
                        forI(sufileex->parser.exported_decl.data.count){
                            imported_decls.add(sufileex->parser.exported_decl.data[i].second);
                        }
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
                                s->members.add(imported_decls[i]->identifier, imported_decls[i]);
                            }
                            stacks.known_declarations.add(&s->decl);
                            sufile->parser.imported_decl.add(s->decl.identifier, &s->decl);
                        }else perror(curt, "Expected an identifier after 'as'");
                    }else{
                        //we are just importing all decls into the global space
                        forI(imported_decls.count){
                            stacks.known_declarations.add(imported_decls[i]);
                            (*stacks.known_declarations_pushed.last)++;
                        }
                    }
                }else perror(curt, "Import specifier is not a string or identifier.");
                
            }else expect(Token_LiteralString){

            }else perror(curt, "Import specifier is not a string or identifier.");
        }break;

        case psRun:{ //---------------------------------------------------------------------------------------------------Run
            NotImplemented;
        }break;

        case psScope:{ //-----------------------------------------------------------------------------------------------Scope
            stacks.nested.scopes.add(current.scope);
            current.scope = arena.make_scope();
            current.scope->token_start = curt;
            TNode* me = &current.scope->node;
            insert_last(node, &current.scope->node);
            while(!next_match(Token_CloseBrace)){
                curt++;
                expect(Token_Identifier){
                    if(curt->is_declaration){
                        define(me, psDeclaration);
                    }else{
                        define(me, psStatement);
                    }
                } else {
                    define(me, psStatement);
                }
            }
            current.scope = stacks.nested.scopes.pop();
        }break;

        case psDeclaration:{ //-----------------------------------------------------------------------------------Declaration
            expect(Token_Identifier) {} else perror_ret(curt, "INTERNAL: parser stage psDeclaration started, but current token is not an identifier.");
            DPTracyDynMessage(toStr("declaring identifier ", curt->raw));
            switch(curt->decl_type){
                case Declaration_Structure:{
                    Declaration* decl = 0;

                    //first make sure that the user isnt trying to redefine a structure that was already made in an imported file
                    if(Declaration* d = sufile->parser.imported_decl.at(curt->raw)){
                        if(d->type == Declaration_Structure){
                            perror(curt, "attempt to define structure '", curt->raw, "' but it already exists as an import from ", d->token_start->file);
                            sufile->logger.log(d->token_start, 0, "original definition of '", curt->raw, "' is here.");
                            return 0;
                        }
                    } 

                    if(curt->is_global){
                        //if the token is global then it will have been added to pending_globals 
                        if(Declaration* d = parser->pending_globals.at(curt->raw)){
                            if(d->complete || d->in_progress){
                                perror(curt, "attempt to define structure '", curt->raw, "' but it already has a definition.");
                                sufile->logger.log(curt, 0, "original definition of '", curt->raw, "' is here.");
                                return 0;
                            }
                            decl = d;
                        }
                    }else{
                        //otherwise just make a new one
                        //TODO(sushi) we should probably check for already existant struct defs here too, since someone could try and redefine a 
                        //            struct in a local scope
                        Struct* s = arena.make_struct();
                        decl = &s->decl;
                        if(is_internal){
                            sufile->parser.internal_decl.add(curt->raw, decl);
                        }else{
                            sufile->parser.exported_decl.add(curt->raw, decl);                            
                        }
                    }

                    Struct* s = StructFromDeclaration(decl);
                    s->members = declmap();
                    s->decl.in_progress = 1;
                    s->decl.token_start = curt;
                    s->decl.type = Declaration_Structure;
                    s->decl.identifier = curt->raw;
                    s->decl.declared_identifier = curt->raw;

                    if(curt->is_global){
                        if(is_internal) sufile->parser.internal_decl.add(curt->raw, &s->decl);
                        else            sufile->parser.exported_decl.add(curt->raw, &s->decl);
                    }

                    curt++;
                    expect(Token_Colon){
                        curt++;
                        expect(Token_StructDecl){
                            curt++;
                            expect(Token_OpenBrace){
                                while(!next_match(Token_CloseBrace)){
                                    curt++;
                                    expect(Token_Identifier){
                                        if(!curt->is_declaration){
                                            perror(curt, "only declarations are allowed inside struct definitions. NOTE(sushi) if this is a declaration, then this error indicates something wrong internally, please let me know.");
                                            return 0;
                                        }
                                        TNode* fin = define(&s->decl.node, psDeclaration);
                                        if(!fin) return 0;
                                        Declaration* d = DeclarationFromNode(fin);
                                        s->members.add(d->identifier, d);
                                    }
                                }
                            } else perror(curt, "expected a '{' after 'struct' in definition of struct '", s->decl.identifier, "'.");
                        } else perror(curt, "INTERNAL: expected 'struct' after ':' for struct definition. NOTE(sushi) tell me if this happens");
                    } else perror(curt, "INTERNAL: expected ':' for struct declaration. NOTE(sushi) tell me if this happens");

                    s->decl.complete = 1;
                    this->cv.notify_all();

                }break;

                case Declaration_Function:{
                    str8 fname = suStr8(curt->raw, curt->l0, curt->c0);
                    Declaration* decl = parser->pending_globals.at(fname);

                    if(curt->is_global && !decl){
                        perror(curt, "INTERNAL: a global declaration token does not have a corresponding pending_globals entry.");
                        return 0;
                    }else if(!decl){
                        Function* f = arena.make_function();
                        decl = &f->decl;
                    }

                    Function* f = FunctionFromDeclaration(decl);
                    f->decl.token_start = curt;
                    b32 is_global = curt->is_global;
                    
                    stacks.known_declarations.add(&f->decl);
                    str8 id = curt->raw;
                    f->internal_label = id;
                    curt++;
                    expect(Token_OpenParen){ // name(
                        while(1){
                            curt++;
                            expect(Token_CloseParen) { break; }
                            else expect(Token_Comma){}
                            else expect(Token_Identifier){
                                Variable* v = VariableFromDeclaration(define(&f->decl.node, psDeclaration));
                                f->internal_label = str8_concat(f->internal_label, suStr8("@", (v->decl.token_start + 2)->raw, ","), deshi_temp_allocator);
                                forI(v->pointer_depth){
                                    f->internal_label = str8_concat(f->internal_label, STR8("*"), deshi_temp_allocator);
                                }

                            } else perror_ret(curt, "expected an identifier for function variable declaration.");
                        }
                        if(is_global){
                            if(is_internal) sufile->parser.internal_decl.add(f->internal_label, &f->decl);
                            else            sufile->parser.exported_decl.add(f->internal_label, &f->decl);
                        }
                        curt++;
                        expect(Token_Colon){ // name(...) :
                            curt++;
                            //TODO(sushi) multiple return types
                            expect_group(TokenGroup_Type){ // name(...) : <type>
                                f->data_type = curt->type;
                                curt++;
                                expect(Token_OpenBrace){ // name(...) : <type> {
                                    define(&f->decl.node, psScope);
                                } else perror(curt, "expected '{' after function declaration.");
                            }else expect(Token_Identifier){ // name(...) : <type>
                                //we are most likely referencing a struct type in this case, so we must look to see if it exists
                                b32 found = 0;
                                forI(stacks.known_declarations.count){
                                    Declaration* d = stacks.known_declarations[stacks.known_declarations.count-1-i];
                                    if(str8_equal_lazy(d->identifier, curt->raw)){
                                        if(d->type == Declaration_Structure){
                                            f->data_type = curt->type;
                                        } else perror(curt, "expected a struct identifier for type specifier in declaration of function '", id, "'. You may have shadowed a structure's identifier by making a variable with its name. TODO(sushi) we can check for this.");
                                    }
                                }
                                //if it is not found in our known stack its possible it is a global declaration that has yet to be parsed
                                if(!found){
                                    if(Declaration* d = parser->pending_globals.at(curt->raw)){
                                        //NOTE(sushi) there is no need to wait here
                                        if(d->type != Declaration_Structure){
                                            perror(curt, "using a declaration identifier that does not pertain to a structure as the return type of function '", id, "'");
                                            sufile->logger.log(d->token_start, 0, "see declaration of identifier.");
                                            return 0;
                                        }
                                        f->data_type = Token_Struct;
                                        f->struct_data = StructFromDeclaration(d);
                                    } else perror(curt, "unknown identifier '", curt->raw, "' used as return type for function declaration of '", id, "'");
                                }
                            } else perror(curt, "expected a type specifier after ':' in function declaration.");
                        } else perror(curt, "expected : after function definition.");
                    } else perror(curt, "expected ( after identifier in function declaration.");
                }break;

                case Declaration_Variable:{
                    Variable* v = arena.make_variable();
                    if(curt->is_global){
                        if(is_internal) sufile->parser.internal_decl.add(curt->raw, &v->decl);
                        else            sufile->parser.exported_decl.add(curt->raw, &v->decl);
                    }
                    stacks.known_declarations.add(&v->decl);
                    v->decl.declared_identifier = curt->raw;
                    v->decl.identifier = curt->raw;
                    v->decl.token_start = curt;
                    str8 id = curt->raw;
                    curt++;
                    expect(Token_Colon){
                        curt++;
                        expect_group(TokenGroup_Type){
                            v->data_type = curt->type;
                            return &v->decl.node;
                        }else expect(Token_Identifier){
                            //in this case we expect this identifier to actually be a struct so we must look for it
                            //first check known declarations 
                            b32 found = 0;
                            forI(stacks.known_declarations.count){
                                Declaration* d = stacks.known_declarations[stacks.known_declarations.count-1-i];
                                if(str8_equal_lazy(d->identifier, curt->raw)){
                                    if(d->type == Declaration_Structure){
                                        v->data_type = Token_Struct;
                                        return &v->decl.node;
                                    }else perror(curt, "expected a struct identifier for type specifier in declaration of variable '", id, "'. You may have shadowed a structure's identifier by making a variable with its name. TODO(sushi) we can check for this, or make it so that this is valid and which id you use is based on context.");
                                    found = 1;
                                }
                            }
                            //if it is not found in our known stack its possible it is a global declaration that has yet to be parsed
                            if(!found){
                                if(Declaration* d = parser->pending_globals.at(curt->raw)){
                                    //NOTE(sushi) there is no need to wait here because we are just making a variable of a certain type
                                    if(d->type != Declaration_Structure){
                                        perror(curt, "attempt to use a declaration identifier that is not a structure as the type specifier of variable '", id, "'");
                                        sufile->logger.log(d->token_start, 0, "see declaration of identifier.");
                                        return 0;
                                    }
                                    v->data_type = Token_Struct;
                                    v->struct_data = StructFromDeclaration(d);
                                    return &v->decl.node;
                                } else perror(curt, "unknown identifier '", curt->raw, "' used as type specifier in declaration of variable '", id, "'");
                            }
                        }else expect(Token_Assignment){
                            //in this case the variable's type is implicit and depends on what the return of the expression is
                            //this is not parsed here, so this variables type is determined later
                            //TODO(sushi) implicit variable declaration type
                            NotImplemented;
                        } else perror(curt, "Expected a type specifier or assignment after ':' in declaration of variable '", id, "'");
                    } else perror(curt, "Expected a ':' after identifier for variable decalration.");
                }break;
            }
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


    threads = array<ParserThread>(deshi_allocator);
    pending_globals = declmap();

    //stacks.known_declarations_pushed.add(0);
   
    sufile->logger.log(2, "Checking that imported files are parsed");

    {// make sure that all imported modules have been parsed before parsing this one
     // then gather the defintions from them that this file wants to use
        forI(sufile->lexer.imports.count){
            Token* itok = &sufile->lexer.tokens[sufile->lexer.imports[i]];
            if(!itok->scope_depth){
                //dummy thread, this shouldnt actually be concurrent
                ParserThread pt;
                pt.curt = itok;
                pt.sufile = sufile;
                pt.parser = this;
                pt.define(&sufile->parser.base, psImport);
            }
        }
        
        forI(sufile->preprocessor.imported_files.count){
            if(sufile->preprocessor.imported_files[i]->stage < FileStage_Parser){
                compiler.logger.warn("INTERNAL: somehow a file wasnt parsed after the initial check above ", __FILENAME__, ", line ", __LINE__);
            }
        }
    }

    threads.reserve(sufile->preprocessor.internal_decl.count+sufile->preprocessor.exported_decl.count);

    for(u32 idx : sufile->preprocessor.internal_decl){
        threads.add(ParserThread());
        ParserThread* pt = threads.last;
        pt->cv.init();
        pt->curt = &sufile->lexer.tokens[idx];
        pt->parser = this;
        pt->node = &sufile->parser.base;
        pt->stage = psDeclaration;
        pt->is_internal = 1;
        switch(sufile->lexer.tokens[idx].decl_type){
            case Declaration_Function:{
                Function* f = arena.make_function();
                f->decl.working_thread = pt;
                str8 fname = suStr8(sufile->lexer.tokens[idx].raw, sufile->lexer.tokens[idx].l0, sufile->lexer.tokens[idx].c0);
                pending_globals.add(fname, &f->decl);
            }break;
            case Declaration_Variable:{
                Variable* v = arena.make_variable();
                v->decl.working_thread = pt;
                sufile->parser.internal_decl.add(sufile->lexer.tokens[idx].raw, &v->decl);
            }break;
            case Declaration_Structure:{
                Struct* s = arena.make_struct();
                s->decl.working_thread = pt;
                sufile->parser.internal_decl.add(sufile->lexer.tokens[idx].raw, &s->decl);
            }break;
        }
        DeshThreadManager->add_job({&parse_threaded_stub, pt});
    }

    for(u32 idx : sufile->preprocessor.exported_decl){
        threads.add(ParserThread());
        ParserThread* pt = threads.last;
        pt->cv.init();
        pt->curt = &sufile->lexer.tokens[idx];
        pt->parser = this;
        pt->node = &sufile->parser.base;
        pt->stage = psDeclaration;
        pt->is_internal = 0;
        //TODO(sushi) this is probably where we want to look out for variable/struct name conflicts 
        //            functions dont matter because of overloading
        switch(sufile->lexer.tokens[idx].decl_type){
            case Declaration_Function:{
                Function* f = arena.make_function();
                f->decl.working_thread = pt;
                f->decl.type = Declaration_Function;
                //because we support overloaded functions we cant just store a function in this 
                //map by name. we also dont want to parse the function for its signature here either
                //so we store its name followed by its line and column number
                //like so:  main10
                str8 fname = suStr8(sufile->lexer.tokens[idx].raw, sufile->lexer.tokens[idx].l0, sufile->lexer.tokens[idx].c0);
                pending_globals.add(fname, &f->decl);
            }break;
            case Declaration_Variable:{
                Variable* v = arena.make_variable();
                v->decl.working_thread = pt;
                v->decl.type = Declaration_Variable;
                pending_globals.add(sufile->lexer.tokens[idx].raw, &v->decl);
            }break;
            case Declaration_Structure:{
                Struct* s = arena.make_struct();
                s->decl.working_thread = pt;
                s->decl.type = Declaration_Structure;
                pending_globals.add(sufile->lexer.tokens[idx].raw, &s->decl);
            }break;
        }
        DeshThreadManager->add_job({&parse_threaded_stub, pt});
    }

    DeshThreadManager->wake_threads();

    forI(threads.count){
        while(!threads[i].finished){
            threads[i].cv.wait();
        }
    }
    
    sufile->logger.log(1, VTS_GreenFg, "Finished parsing in ", peek_stopwatch(time), " ms", VTS_Default);
}