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
#define push_statement(in)  {stacks.nested.statements.add(current.statement); current.statement = in;}
#define pop_statement()     {current.statement = stacks.nested.statements.pop();}

#define ConvertGroup(a,b)                      \
switch(to){                                    \
    case Token_Signed8:    {  return true; }   \
    case Token_Signed16:   {  return true; }   \
    case Token_Signed32:   {  return true; }   \
    case Token_Signed64:   {  return true; }   \
    case Token_Unsigned8:  {  return true; }   \
    case Token_Unsigned16: {  return true; }   \
    case Token_Unsigned32: {  return true; }   \
    case Token_Unsigned64: {  return true; }   \
    case Token_Float32:    {  return true; }   \
    case Token_Float64:    {  return true; }   \
    case Token_Struct:     { NotImplemented; } \
}

//checks if a conversion from one type to another is valid
b32 type_conversion(Type to, Type from){
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

//checks the known stack to see if there are any variable name conflicts in the same scope
b32 Validator::check_shadowing(Declaration* d){DPZoneScoped;
    suLogger& logger = sufile->logger;
    forI(stacks.known_declarations.count - stacks.known_declarations_scope_begin_offsets[stacks.known_declarations_scope_begin_offsets.count-1]){
        Declaration* dk = stacks.known_declarations[stacks.known_declarations.count - 1 - i];
        if(d->type == Declaration_Function && dk->type == Declaration_Function){
            //ignore, because function overloading
            continue;
        }
        if(str8_equal_lazy(d->identifier, dk->identifier)){

            logger.error(d->token_start, 
            "declaration of ", 
            (d->type == Declaration_Variable ? "variable" : (d->type == Declaration_Function ? "function" : "structure")),
            " '", d->identifier, "' overrides declaration of ",
            (dk->type == Declaration_Variable ? "variable" : (dk->type == Declaration_Function ? "function" : "structure")),
            " '", d->identifier, "'");
            logger.note(dk->token_start, "see declaration of '", dk->identifier, "'");
            return 0;
        }
    }
    return 1;
}

TNode* Validator::validate(TNode* node){DPZoneScoped;
    suLogger& logger = sufile->logger;
    switch(node->type){
        case NodeType_Variable:{
            Variable* v = VariableFromNode(node);
            push_variable(v);
            if(!check_shadowing(&v->decl)) return 0;
            stacks.known_declarations.add(&v->decl);

            if(v->data.implicit){
                //if this variable's type is implicit, then there will be an expression after it
                //that we use to determine its type
                //there should only be one child node and it should resolve to an expression
                Expression* ret = ExpressionFromNode(validate(v->decl.node.first_child));
                v->data = ret->data;
            }else if(v->data.type == Token_Struct){
                //we must determine what struct this varaible
            }

            pop_variable();
        }break;

        case NodeType_Function:{
            Function* f = FunctionFromNode(node);
            push_function(f);
            if(!check_shadowing(&f->decl)) return 0;
            stacks.known_declarations.add(&f->decl);

            stacks.known_declarations_scope_begin_offsets.add(stacks.known_declarations.count);

            for_node(f->decl.node.first_child){
                validate(it);
            }

            stacks.known_declarations.pop(stacks.known_declarations.count - stacks.known_declarations_scope_begin_offsets.pop());

            pop_function();
        }break;

        case NodeType_Structure:{
            Struct* s = StructFromNode(node);
            push_struct(s);
            if(!check_shadowing(&s->decl)) return 0;
            stacks.known_declarations.add(&s->decl);

            stacks.known_declarations_scope_begin_offsets.add(stacks.known_declarations.count);

            for_node(s->decl.node.first_child){
                validate(it);
            }

            stacks.known_declarations.pop(stacks.known_declarations.count - stacks.known_declarations_scope_begin_offsets.pop());

            pop_struct();
        }break;

        case NodeType_Scope:{
            Scope* s = ScopeFromNode(node);
            push_scope(s);

            stacks.known_declarations_scope_begin_offsets.add(stacks.known_declarations.count);

            for_node(s->node.first_child){
                validate(it);
            }

            stacks.known_declarations.pop(stacks.known_declarations.count - stacks.known_declarations_scope_begin_offsets.pop());

            pop_scope();

        }break;

        case NodeType_Statement:{
            Statement* s = StatementFromNode(node);
            push_statement(s);

            pop_statement();
        }break;

        case NodeType_Expression:{
            Expression* e = ExpressionFromNode(node);
            push_expression(e);
            
            pop_expression();
        }break;
    }
    return 0;
}

void Validator::start(){DPZoneScoped;
    stacks.known_declarations_scope_begin_offsets.add(0);
    //gather global declarations into our known array
    //TODO(sushi) this could possibly be a separate map from our known decls array
    //            because globals dont allow shadowing
    // for_node(sufile->parser.base.first_child){
    //     stacks.known_declarations.add(DeclarationFromNode(it));
    // }

    for_node(sufile->parser.base.first_child){
        validate(it);
    }

}