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

TNode* Validator::validate(TNode* node){
    switch(node->type){
        case NodeType_Variable:{
            Variable* v = VariableFromNode(node);
            
        }break;
        case NodeType_Function:{
            Function* f = FunctionFromNode(node);
        }break;
        case NodeType_Structure:{
            Struct* s = StructFromNode(node);
        }break;
        case NodeType_Scope:{
            Scope* s = ScopeFromNode(node);
        }break;
        case NodeType_Statement:{
            Statement* s = StatementFromNode(node);
        }break;
        case NodeType_Expression:{
            Expression* e = ExpressionFromNode(node);
        }break;
    }

}

void Validator::start(){
    //gather global declarations into our known array
    //TODO(sushi) this could possibly be a separate map from our known decls array
    //            because globals dont allow shadowing
    for_node(sufile->parser.base.first_child){
        known_decls.add(DeclarationFromNode(it));
    }

    for_node(sufile->parser.base.first_child){
        validate_declaration(it);
    }

}