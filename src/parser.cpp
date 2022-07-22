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

#define pwarn(token, ...)\
sufile->logger.warn(token, __VA_ARGS__);


#define expect(...) if(curr_match(__VA_ARGS__))
#define expect_group(...) if(curr_match_group(__VA_ARGS__))

#define push_variable(in)   {stacks.nested.variables.add(current.variable); current.variable = in;}
#define pop_variable()      {current.variable = stacks.nested.variables.pop();}
#define push_expression(in) {stacks.nested.expressions.add(current.expression); current.expression = in;}
#define pop_expression()    {current.expression = stacks.nested.expressions.pop();}
#define push_struct(in)     {stacks.nested.structs.add(current.structure); current.structure = in;}
#define pop_struct()        {current.structure = stacks.nested.structs.pop();}
#define push_scope(in)      {stacks.nested.scopes.add(current.scope); current.scope = in;}
#define pop_scope()         {current.scope = stacks.nested.scopes.pop();}
#define push_function(in)   {stacks.nested.functions.add(current.function); current.function = in;}
#define pop_function()      {current.function = stacks.nested.functions.pop();}
#define push_statement(in)   {stacks.nested.statements.add(current.statement); current.statement = in;}
#define pop_statement()      {current.statement = stacks.nested.statements.pop();}

str8 type_token_to_str(Type type){
    switch(type){
        case Token_Void:       return STR8("void");
        case Token_Signed8:    return STR8("s8");
        case Token_Signed16:   return STR8("s16");
        case Token_Signed32:   return STR8("s32");
        case Token_Signed64:   return STR8("s64");
        case Token_Unsigned8:  return STR8("u8");
        case Token_Unsigned16: return STR8("u16");
        case Token_Unsigned32: return STR8("u32");
        case Token_Unsigned64: return STR8("u64");
        case Token_Float32:    return STR8("f32");
        case Token_Float64:    return STR8("f64");
        case Token_String:     return STR8("str");
        case Token_Any:        return STR8("any");
        case Token_Struct:     return STR8("struct-type");
    }
    return STR8("UNKNOWN DATA TYPE");
}

Declaration* ParserThread::resolve_identifier(Token* tok){
	b32 found = 0;
	forI(stacks.known_declarations.count){
		Declaration* d = stacks.known_declarations[stacks.known_declarations.count-1-i];
		if(str8_equal_lazy(d->identifier, tok->raw)) return d;
	}
	//if it is not found in our known stack its possible it is a global declaration that has yet to be parsed
	if(Declaration* d = parser->pending_globals.at(tok->raw)) return d;
	return 0;
}

template<typename... T>
TNode* ParserThread::binop_parse(TNode* node, TNode* ret, Type next_stage, T... tokchecks){
    //TODO(sushi) need to detect floats being used in bitwise operations (or just allow it :) 
    TNode* out = ret;
    TNode* lhs_node = ret;
    while(next_match(tokchecks...)){
        curt++;
        //make binary op expression
        Expression* op = arena.make_expression(curt->raw);
        op->expr_type = binop_token_to_expression(curt->type);
        op->token_start = curt;

        //readjust parents to make the binary op the new child of the parent node
        //and the ret node a child of the new binary op
        change_parent(node, &op->node);
        change_parent(&op->node, out);

        //evaluate next expression
        curt++;
        Expression* rhs = ExpressionFromNode(define(&op->node, next_stage));

        //check types on each side and make sure they're compatible
        //if they are we set the operator's type to the type whose data loss is minimized in the conversion
        //for lack of a better word.. what i mean is like the case of 1.2 + 1
        //the result of the op will be a float because converting an integer to a float loses no data where as 
        //converting from a float to an int does. please let me (sushi) know what this is called if you ever find this
        //we dont actually change all the expression nodes types, only the binary op's
        Expression* lhs = ExpressionFromNode(lhs_node);
        if(lhs->data.type >= rhs->data.type){
            op->data.type = lhs->data.type;
        }else if(rhs->data.type > lhs->data.type){
            op->data.type = rhs->data.type;
        }
        
        out = &op->node;
    }
    return out;
}


#define ConvertAndThatsIt(actualtype,type1,type2)\
actualtype og_val = tv->type1; tv->type2 = og_val;

//only valid for integer conversions if they are both either signed or unsigned
#define ConvertAndDetectOverflow(actualtype, type1, type2)\
ConvertAndThatsIt(actualtype,type1,type2);\
if(og_val!=tv->type2){\
    sufile->logger.warn(curt, "conversion from ", STRINGIZE(type1), " to ", STRINGIZE(type2), " causes overflow.");\
    sufile->logger.note(curt, "original value of ", og_val, " becomes ", tv->type2);\
}

#define ConvertGroup(a,b)                                                             \
switch(to){                                                              \
    case Token_Signed8:    { ConvertAndDetectOverflow(a, b,    int8); return true; } \
    case Token_Signed16:   { ConvertAndDetectOverflow(a, b,   int16); return true; } \
    case Token_Signed32:   { ConvertAndDetectOverflow(a, b,   int32); return true; } \
    case Token_Signed64:   { ConvertAndDetectOverflow(a, b,   int64); return true; } \
    case Token_Unsigned8:  { ConvertAndDetectOverflow(a, b,   uint8); return true; } \
    case Token_Unsigned16: { ConvertAndDetectOverflow(a, b,  uint16); return true; } \
    case Token_Unsigned32: { ConvertAndDetectOverflow(a, b,  uint32); return true; } \
    case Token_Unsigned64: { ConvertAndDetectOverflow(a, b,  uint64); return true; } \
    case Token_Float32:    { ConvertAndThatsIt(a, b, float32); return true; } \
    case Token_Float64:    { ConvertAndThatsIt(a, b, float64); return true; } \
    case Token_Struct:     { NotImplemented; }                                    \
}

//attempts to convert a type to another 
//currently this will only handle built in coercion, but should handle custom conversions later as well 
b32 ParserThread::type_conversion(Type to, Type from, TypedValue* tv){
    switch(from){
        case Token_Signed8:    ConvertGroup( s8,    int8); break;
		case Token_Signed16:   ConvertGroup(s16,   int16); break;
		case Token_Signed32:   ConvertGroup(s32,   int32); break;
		case Token_Signed64:   ConvertGroup(s64,   int64); break;
		case Token_Unsigned8:  ConvertGroup( u8,   uint8); break;
		case Token_Unsigned16: ConvertGroup(u16,  uint16); break;
		case Token_Unsigned32: ConvertGroup(u32,  uint32); break;
		case Token_Unsigned64: ConvertGroup(u64,  uint64); break;
		case Token_Float32:    ConvertGroup(f32, float32); break;
		case Token_Float64:    ConvertGroup(f64, float64); break;
		case Token_Struct:     NotImplemented; break;
    }
    return false;
}

TNode* ParserThread::define(TNode* node, Type stage){DPZoneScoped;
    //ThreadSetName(suStr8("parsing ", curt->raw, " in ", curt->file));
    switch(stage){
        case psFile:{ //-------------------------------------------------------------------------------------------------File
            // i dont think this stage will ever be touched
        }break;

        case psDirective:{ //---------------------------------------------------------------------------------------Directive
            // there is no reason to check for invalid directives here because that is handled by lexer

        }break;

        case psImport:{ //---------------------------------------------------------------------------------------------Import
            //#import 
            array<Declaration*> imported_decls;
            curt++;
            expect(Token_OpenBrace){ //#import {
                curt++; 
                while(1){
                    expect(Token_LiteralString){ //#import { "..." 
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
                            sufileex = compiler.files.at(filename);
                        }

                        curt++;
                        expect(Token_OpenBrace){ //#import { "..." { 
                            //we are including specific defintions from the import
                            while(1){
                                curt++;
                                expect(Token_Identifier){ //#import { "..." { <id> 
                                    //now we must make sure it exists
                                    Type type = 0;
                                    Declaration* decl = 0;
                                    sufileex->parser.find_identifier_externally(curt->raw);
                                    if(!decl) perror(curt, "Attempted to include declaration '", curt->raw, "' from module '", sufileex->file->front, "' but it is either internal or doesn't exist.");
                                    else expect(Token_As){ //#import { "..." { <id> as
                                        //in this case we must duplicate the declaration and give it a new identifier
                                        //TODO(sushi) duplication may not be necessary, instead we can store a local name
                                        //            and just use the same declaration node.
                                        //TODO(sushi) dont duplicate the declaration
                                        curt++;
                                        expect(Token_Identifier) {//#import { "..." { <id> as <id>
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
                                    expect(Token_Comma){} //#import { "..." ... ,
                                    else expect(Token_CloseBrace) { //#import { "..." ... } 
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
                                Struct* s = arena.make_struct(curt->raw);
                                s->members.init(); 
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
                            }
                        }
                    }else expect(Token_CloseBrace){
                        //handle empty import directives and warn about it
                        pwarn(curt, "empty import directive.");
                        return 0;
                    }else perror(curt, "Import specifier is not a string or identifier.");
                    curt++;
                    expect(Token_Comma){
                        curt++;
                        //support having a comma after the last entry, because it slighty eases adding new entries to the list
                        expect(Token_CloseBrace) break;
                    } 
                    else expect(Token_CloseBrace) break;
                    else perror_ret(curt, "unexpected token '", curt->raw, "' in import directive.");
                }
            }else expect(Token_LiteralString){

            }else perror(curt, "Import specifier is not a string or identifier.");
            curt++;
        }break;

        case psRun:{ //---------------------------------------------------------------------------------------------------Run
            NotImplemented;
        }break;

        case psScope:{ //-----------------------------------------------------------------------------------------------Scope
            stacks.nested.scopes.add(current.scope);
            current.scope = arena.make_scope(curt->raw);
            current.scope->token_start = curt;
            stacks.known_declarations_scope_begin_offsets.add(stacks.known_declarations.count);
            TNode* me = &current.scope->node;
            insert_last(node, &current.scope->node);
            while(!next_match(Token_CloseBrace)){
                curt++;
                expect(Token_Identifier){
                    if(curt->is_declaration){
                        TNode* ret = define(me, psDeclaration);
                        if(!ret) return 0;
                        Declaration* d = DeclarationFromNode(ret);
                        if(d->type == Declaration_Variable){
                            curt++;
                            expect(Token_Semicolon){}
                            else perror_ret(curt, "expected ; after variable declaration.");
                        }
                    }else{
                        define(me, psStatement);
                    }
                }else expect(Token_EOF){
                    perror_ret(curt, "unexpected end of file.");
                } else {
                    define(me, psStatement);
                }
            }
            curt++;
            stacks.known_declarations.pop(stacks.known_declarations.count - stacks.known_declarations_scope_begin_offsets.pop());
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
                        Struct* s = arena.make_struct(curt->raw);
                        decl = &s->decl;
                        if(is_internal){
                            sufile->parser.internal_decl.add(curt->raw, decl);
                        }else{
                            sufile->parser.exported_decl.add(curt->raw, decl);                            
                        }
                    }

                    Struct* s = StructFromDeclaration(decl);
                    push_struct(s);
                    s->members.init();

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
                                            pop_struct();
                                            return 0;
                                        }
                                        TNode* fin = define(&s->decl.node, psDeclaration);
                                        if(!fin){
                                            pop_struct();
                                            return 0;
                                        } 
                                        Declaration* d = DeclarationFromNode(fin);
                                        s->members.add(d->identifier, d);
                                    }
                                }
                            } else perror(curt, "expected a '{' after 'struct' in definition of struct '", s->decl.identifier, "'.");
                        } else perror(curt, "INTERNAL: expected 'struct' after ':' for struct definition. NOTE(sushi) tell me if this happens");
                    } else perror(curt, "INTERNAL: expected ':' for struct declaration. NOTE(sushi) tell me if this happens");

                    s->decl.complete = 1;
                    this->cv.notify_all();
                    pop_struct();
                }break;

                case Declaration_Function:{
                    str8 fname = suStr8(curt->raw, curt->l0, curt->c0);
                    Declaration* decl = parser->pending_globals.at(fname);

                    if(curt->is_global && !decl){
                        perror(curt, "INTERNAL: a global declaration token does not have a corresponding pending_globals entry.");
                        return 0;
                    }else if(!decl){
                        Function* f = arena.make_function(curt->raw);
                        decl = &f->decl;
                    }

                    Function* f = FunctionFromDeclaration(decl);
                    push_function(f);
                    f->decl.token_start = curt;
                    f->decl.type = Declaration_Function;
                    b32 is_global = curt->is_global;
                    stacks.known_declarations.add(&f->decl);
                    str8 id = curt->raw;
                    f->internal_label = id;
                    f->internal_label = suStr8(f->internal_label, "@");
                    curt++;
                    expect(Token_OpenParen){ // name(
                        while(1){
                            curt++;
                            expect(Token_CloseParen) { break; }
                            else expect(Token_Comma){}
                            else expect(Token_Identifier){
                                Variable* v = VariableFromNode(define(&f->decl.node, psDeclaration));
                                f->internal_label = suStr8(f->internal_label, (v->decl.token_start + 2)->raw, ",");
                                forI(v->pointer_depth){
                                    f->internal_label = suStr8(f->internal_label, STR8("*"));
                                }

                            } else perror_ret(curt, "expected an identifier for function variable declaration.");
                        }
                        curt++;
                        expect(Token_Colon){ // name(...) :
                            curt++;
                            //TODO(sushi) multiple return types
                            expect_group(TokenGroup_Type){ // name(...) : <type>
                                f->data_type = curt->type;
                                f->internal_label = suStr8(f->internal_label, "@", curt->raw);
                                if(is_global){
                                    if(is_internal) sufile->parser.internal_decl.add(f->internal_label, &f->decl);
                                    else            sufile->parser.exported_decl.add(f->internal_label, &f->decl);
                                }
                            }else expect(Token_Identifier){ // name(...) : <type>
                                //we are most likely referencing a struct type in this case, so we must look to see if it exists
                                Declaration* d = resolve_identifier(curt);
                                if(!d){
                                    perror(curt, "unknown identifier '", curt->raw, "' used as return type for function declaration of '", id, "'");
                                    pop_function();
                                    return 0;
                                }
                                if(d->type != Declaration_Structure){
                                    perror(curt, "expected a struct identifier for type specifier in declaration of function '", id, "'. You may have shadowed a structure's identifier by making a variable with its name. TODO(sushi) we can check for this.");
                                    pop_function();
                                    return 0;
                                }
                                f->data_type = Token_Struct;
                                f->struct_data = StructFromDeclaration(d);
                                f->internal_label = suStr8(f->internal_label, "@", curt->raw);
                                if(curt->is_global){
                                    if(is_internal) sufile->parser.internal_decl.add(f->internal_label, &f->decl);
                                    else            sufile->parser.exported_decl.add(f->internal_label, &f->decl);
                                }
                            } else perror(curt, "expected a type specifier after ':' in function declaration.");
                            curt++;
                            expect(Token_OpenBrace) {
                                define(&f->decl.node, psScope);
                            } else perror(curt, "expected '{' after function declaration.");
                        } else perror(curt, "expected : after function definition.");
                    } else perror(curt, "expected ( after identifier in function declaration.");
                    pop_function();
                }break;

                case Declaration_Variable:{ //name
                    Variable* v = arena.make_variable(curt->raw);
                    push_variable(v);
                    if(curt->is_global){
                        if(is_internal) sufile->parser.internal_decl.add(curt->raw, &v->decl);
                        else            sufile->parser.exported_decl.add(curt->raw, &v->decl);
                    }
                    stacks.known_declarations.add(&v->decl);
                    v->decl.declared_identifier = curt->raw;
                    v->decl.identifier = curt->raw;
                    v->decl.token_start = curt;
                    v->decl.type = Declaration_Variable;
                    str8 id = curt->raw;
                    curt++;
                    expect(Token_Colon){ // name :
                        curt++;
                        b32 implicit_type = 0;
                        expect_group(TokenGroup_Type){ // name : <type>
                            v->data.type = curt->type;
                        }else expect(Token_Identifier){ // name : <type>
                            //in this case we expect this identifier to actually be a struct so we must look for it
                            //first check known declarations 
                            Declaration* d = resolve_identifier(curt);
                            if(!d){
                                perror(curt, "unknown identifier '", curt->raw, "' used as type specifier for variable declaration of '", id, "'");
                                pop_variable();
                                return 0;
                            }
                            if(d->type != Declaration_Structure){
                                perror(curt, "expected a struct identifier for type specifier in declaration of variable '", id, "'. You may have shadowed a structure's identifier by making a variable with its name. TODO(sushi) we can check for this.");
                                pop_variable();
                                return 0;
                            }
                            v->data.type = Token_Struct;
                            v->data.struct_type = StructFromDeclaration(d);
                        }else implicit_type = 1;
                        expect(Token_Assignment){ // name : <type> = || name := 
                            Token* before = curt;
                            curt++;
                            Expression* e = ExpressionFromNode(define(&v->decl.node, psExpression));
                            if(implicit_type){
                                v->data.type = e->data.type;
                            }else if(e->data.type != v->data.type){
                                //check that the type of the expression's value has a conversion to the type of the variable
                                if(!type_conversion(v->data.type, e->data.type, &e->data)){
                                    perror(curt, "there is no known conversion from ", type_token_to_str(e->data.type), " to ", (v->decl.token_start + 2)->raw);
                                    return 0;
                                }
                            }
                        } else if(implicit_type) perror(curt, "Expected a type specifier or assignment after ':' in declaration of variable '", id, "'");
                        insert_last(node, &v->decl.node);
                        return &v->decl.node;
                    } else perror(curt, "Expected a ':' after identifier for variable decalration.");
                }break;
            }
        }break;

        case psStatement:{ //---------------------------------------------------------------------------------------Statement
            switch(curt->type){
                case Token_OpenBrace:{
                    define(node, psScope);
                }break;

                case Token_Directive_Import:
                case Token_Directive_Include:
                case Token_Directive_Internal:
                case Token_Directive_Run:{
                    //not sure yet when this will happen and how it will be handled
                    //the only place this should be encountered is #run, probably
                    TestMe;
                }break;

                case Token_Using:{
                    curt++;
                    expect(Token_Identifier){
                        Declaration* d = resolve_identifier(curt);
                        if(!d) perror_ret(curt, "unknown identifier '", curt->raw, "' used as identifier of using statement.");
                        if(d->type != Declaration_Variable) perror_ret(curt, "attempted to use an identifier that does not belong to a variable as identifier for using.");
                        Variable* v = VariableFromDeclaration(d);
                        if(v->data.type!=Token_Struct) perror_ret(curt, "attempt to use using on a type that is not a struct.");
                        //expand the variable's vars into the scope
                        forI(v->data.struct_type->members.data.count){
                            stacks.known_declarations.add(v->data.struct_type->members.data[i].second);
                        }
                        curt++;
                        expect(Token_Semicolon){curt++;}
                        else perror_ret(curt, "expected a semicolon after using statement");
                    } else perror_ret(curt, "expected a variable identifier after 'using'.");
                }break;

                case Token_Return:{
                    Statement* s = arena.make_statement(curt->raw);
                    s->type = Statement_Return;
                    s->token_start = curt;
                    insert_last(node, &s->node);
                    curt++;
                    expect(Token_Semicolon){
                        if(current.function->data_type != Token_Void){
                            perror_ret(curt, "attempt to return nothing, but the function's return type is not void.");
                        }
                        else{curt++;}
                    }else{
                        if(current.function->data_type == Token_Void)
                            perror_ret(curt, "attempt to return a value, but the function's return type is void.");
                        TNode* n = define(&s->node, psExpression);
                        if(!n) return 0;
                        Expression* e = ExpressionFromNode(n);
                        if(e->data.type != current.function->data_type){
                            str8 etypename;
                            str8 ftypename;
                            if(e->data.type == Token_Struct){
                                etypename = e->data.struct_type->decl.identifier;
                            }else{
                                etypename = type_token_to_str(e->data.type);
                            }
                            if(current.function->data_type == Token_Struct){
                                ftypename = current.function->struct_data->decl.identifier;
                            }else{
                                ftypename = type_token_to_str(current.function->data_type);
                            }
                            perror_ret(curt, "expression does not evaluate to the function's return type. expression returns '", etypename, "', but function's return type is '", ftypename, "'.");
                        }
                        insert_last(&s->node, &e->node);
                    }
                }break;

                default:{
                    //this is probably just some expression
                    Statement* s = arena.make_statement();
                    push_statement(s);
                    s->token_start = curt;
                    insert_last(node, &s->node);
                    TNode* ret = define(node, psExpression);
                    if(!ret) return 0;
                    curt++;
                    expect(Token_Semicolon){}
                    else perror_ret(curt, "expected a ; after statement.");
                    pop_statement();
                    s->token_end = curt;
                }break;

            }
        }break;

        case psExpression:{ //-------------------------------------------------------------------------------------Expression
            
            expect(Token_Identifier){
                Declaration* d = resolve_identifier(curt);
                if(!d) perror_ret(curt, "unknown identifier '", curt->raw, "'.");
                if(d->type == Declaration_Variable){
                    Expression* e = arena.make_expression(curt->raw);
            
                    e->expr_type = Expression_IdentifierLHS;
                    e->token_start = curt;
                
                    curt++;
                    expect(Token_Assignment){
                        Expression* op = arena.make_expression(curt->raw);
                        insert_last(&op->node, &e->node);
                        curt++;
                        TNode* ret = define(&op->node, psExpression);
                        insert_last(node, &op->node);
                        return &op->node;                                                
                    }else{
                        TNode* n = define(&e->node, psConditional);
                        if(!n) return 0;
                        insert_last(node, n);
                    }
                }else if(d->type == Declaration_Function){

                }else if(d->type == Declaration_Structure){

                }
               
            }else{
                Expression* e = ExpressionFromNode(define(node, psConditional));
                return &e->node;
            }

        }break;

        case psConditional:{ //-----------------------------------------------------------------------------------Conditional
            expect(Token_If){
                NotImplemented;
            }else{
                Expression* e = ExpressionFromNode(define(node, psLogicalOR));
                return &e->node;
            }
        }break;

        case psLogicalOR:{ //---------------------------------------------------------------------------------------LogicalOR
            Expression* e = ExpressionFromNode(define(node, psLogicalAND));
            return binop_parse(node, &e->node, psLogicalAND, Token_OR);
        }break;

        case psLogicalAND:{ //-------------------------------------------------------------------------------------LogicalAND
            Expression* e = ExpressionFromNode(define(node, psBitwiseOR));
            return binop_parse(node, &e->node, psBitwiseOR, Token_AND);
        }break;

        case psBitwiseOR:{ //---------------------------------------------------------------------------------------BitwiseOR
            Expression* e = ExpressionFromNode(define(node, psBitwiseXOR));
            return binop_parse(node, &e->node, psBitwiseXOR, Token_BitOR);
        }break;

        case psBitwiseXOR:{ //-------------------------------------------------------------------------------------BitwiseXOR
            Expression* e = ExpressionFromNode(define(node, psBitwiseAND));
            return binop_parse(node, &e->node, psBitwiseAND, Token_BitXOR);
        }break;

        case psBitwiseAND:{ //-------------------------------------------------------------------------------------BitwiseAND
            Expression* e = ExpressionFromNode(define(node, psEquality));
            return binop_parse(node, &e->node, psEquality, Token_BitAND);
        }break;

        case psEquality:{ //-----------------------------------------------------------------------------------------Equality
            Expression* e = ExpressionFromNode(define(node, psRelational));
            return binop_parse(node, &e->node, psRelational, Token_NotEqual, Token_Equal);
        }break;

        case psRelational:{ //-------------------------------------------------------------------------------------Relational
            Expression* e = ExpressionFromNode(define(node, psBitshift));
            return binop_parse(node, &e->node, psBitshift, Token_LessThan, Token_GreaterThan, Token_LessThanOrEqual, Token_GreaterThanOrEqual);
        }break;

        case psBitshift:{ //-----------------------------------------------------------------------------------------Bitshift
            Expression* e = ExpressionFromNode(define(node, psAdditive));
            return binop_parse(node, &e->node, psAdditive, Token_BitShiftLeft, Token_BitShiftRight);
        }break;

        case psAdditive:{ //-----------------------------------------------------------------------------------------Additive
            Expression* e = ExpressionFromNode(define(node, psTerm));
            return binop_parse(node, &e->node, psTerm, Token_Plus, Token_Negation);
        }break;

        case psTerm:{ //-------------------------------------------------------------------------------------------------Term
            Expression* e = ExpressionFromNode(define(node, psFactor));
            return binop_parse(node, &e->node, psFactor, Token_Multiplication, Token_Division, Token_Modulo);
        }break;

        case psFactor:{ //---------------------------------------------------------------------------------------------Factor
            switch(curt->type){
                case Token_LiteralFloat:{
                    Expression* e = arena.make_expression(curt->raw);
                    e->expr_type = Expression_Literal;
                    e->data.type = Token_Float64;
                    e->data.float64 = curt->f64_val;
                    insert_last(node, &e->node);
                    return &e->node;
                }break;

                case Token_LiteralInteger:{
                    Expression* e = arena.make_expression(curt->raw);
                    e->expr_type = Expression_Literal;
                    e->data.type = Token_Signed64;
                    e->data.int64 = curt->s64_val;
                    insert_last(node, &e->node);
                    return &e->node;
                }break;

                case Token_OpenParen:{
                    curt++;
                    Token* start = curt;
                    TNode* ret = define(node, psExpression);
                    curt++;
                    expect(Token_CloseParen){
                        return ret;
                    }else perror(curt, "expected a ) to close ( in line ", start->l0, " on column ", start->c0);
                }break;

                case Token_Identifier:{

                }break;
            }
        }break;

    }


    return 0;
}

void Parser::parse(){DPZoneScoped;
    Stopwatch time = start_stopwatch();
    sufile->logger.log(Verbosity_Stages, "Parsing...");

    threads = array<ParserThread>(deshi_allocator);
    pending_globals.init();

    //stacks.known_declarations_pushed.add(0);
   
    sufile->logger.log(Verbosity_StageParts, "Checking that imported files are parsed");

    {// make sure that all imported modules have been parsed before parsing this one
     // then gather the defintions from them that this file wants to use
        forI(sufile->lexer.imports.count){
            Token* itok = sufile->lexer.tokens.readptr(sufile->lexer.imports[i]);
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
        pt->init();
        pt->cv.init();
        pt->curt = sufile->lexer.tokens.readptr(idx);
        pt->parser = this;
        pt->node = &sufile->parser.base;
        pt->stage = psDeclaration;
        pt->is_internal = 1;
        switch(sufile->lexer.tokens[idx].decl_type){
            case Declaration_Function:{
                Function* f = arena.make_function(pt->curt->raw);
                f->decl.working_thread = pt;
                str8 fname = suStr8(sufile->lexer.tokens[idx].raw, sufile->lexer.tokens[idx].l0, sufile->lexer.tokens[idx].c0);
                pending_globals.add(fname, &f->decl);
            }break;
            case Declaration_Variable:{
                Variable* v = arena.make_variable(pt->curt->raw);
                v->decl.working_thread = pt;
                sufile->parser.internal_decl.add(sufile->lexer.tokens[idx].raw, &v->decl);
            }break;
            case Declaration_Structure:{
                Struct* s = arena.make_struct(pt->curt->raw);
                s->decl.working_thread = pt;
                sufile->parser.internal_decl.add(sufile->lexer.tokens[idx].raw, &s->decl);
            }break;
        }
        DeshThreadManager->add_job({&parse_threaded_stub, pt});
    }

    for(u32 idx : sufile->preprocessor.exported_decl){
        threads.add(ParserThread());
        ParserThread* pt = threads.last;
        pt->init();
        pt->cv.init();
        pt->curt = sufile->lexer.tokens.readptr(idx);
        pt->parser = this;
        pt->node = &sufile->parser.base;
        pt->stage = psDeclaration;
        pt->is_internal = 0;
        //TODO(sushi) this is probably where we want to look out for variable/struct name conflicts 
        //            functions dont matter because of overloading
        switch(sufile->lexer.tokens[idx].decl_type){
            case Declaration_Function:{
                Function* f = arena.make_function(pt->curt->raw);
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
                Variable* v = arena.make_variable(pt->curt->raw);
                v->decl.working_thread = pt;
                v->decl.type = Declaration_Variable;
                pending_globals.add(sufile->lexer.tokens[idx].raw, &v->decl);
            }break;
            case Declaration_Structure:{
                Struct* s = arena.make_struct(pt->curt->raw);
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
    
    sufile->logger.log(Verbosity_Stages, VTS_GreenFg, "Finished parsing in ", peek_stopwatch(time), " ms", VTS_Default);
}