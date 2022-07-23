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


TNode* Validator::validate(TNode* node){DPZoneScoped;
    switch(node->type){
        case NodeType_Variable:{
            Variable* v = VariableFromNode(node);
            push_variable(v);

            

                        

            pop_variable();
        }break;
        case NodeType_Function:{
            Function* f = FunctionFromNode(node);
            push_function(f);

            pop_function();
        }break;
        case NodeType_Structure:{
            Struct* s = StructFromNode(node);
            push_struct(s);

            pop_struct();
        }break;
        case NodeType_Scope:{
            Scope* s = ScopeFromNode(node);
            push_scope(s);

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
    for_node(sufile->parser.base.first_child){
        stacks.known_declarations.add(DeclarationFromNode(it));
    }

    for_node(sufile->parser.base.first_child){
        validate(it);
    }

}