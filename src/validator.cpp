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

//we know that we can convert any scalar type to another scalar type, so just return true for all of them
#define ConvertGroupFromBuiltin(a,b)           \
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
    case Token_Struct:     {  return false; } \
}

//we need to check if we can convert from a user defined type to any other type
#define ConvertGroupFromUser                   \
switch(to){                                    \
    case Token_Signed8:    {  return false; }   \
    case Token_Signed16:   {  return false; }   \
    case Token_Signed32:   {  return false; }   \
    case Token_Signed64:   {  return false; }   \
    case Token_Unsigned8:  {  return false; }   \
    case Token_Unsigned16: {  return false; }   \
    case Token_Unsigned32: {  return false; }   \
    case Token_Unsigned64: {  return false; }   \
    case Token_Float32:    {  return false; }   \
    case Token_Float64:    {  return false; }   \
    case Token_Struct:     {  return false; } \
}

//checks if a conversion from one type to another is valid
b32 type_conversion(Type to, Type from){
    switch(from){
        case Token_Signed8:    ConvertGroupFromBuiltin( s8,    int8); break;
		case Token_Signed16:   ConvertGroupFromBuiltin(s16,   int16); break;
		case Token_Signed32:   ConvertGroupFromBuiltin(s32,   int32); break;
		case Token_Signed64:   ConvertGroupFromBuiltin(s64,   int64); break;
		case Token_Unsigned8:  ConvertGroupFromBuiltin( u8,   uint8); break;
		case Token_Unsigned16: ConvertGroupFromBuiltin(u16,  uint16); break;
		case Token_Unsigned32: ConvertGroupFromBuiltin(u32,  uint32); break;
		case Token_Unsigned64: ConvertGroupFromBuiltin(u64,  uint64); break;
		case Token_Float32:    ConvertGroupFromBuiltin(f32, float32); break;
		case Token_Float64:    ConvertGroupFromBuiltin(f64, float64); break;
		case Token_Struct:     ConvertGroupFromUser; break;
    }
    return false;
}

//checks the known stack ConvertGroupFromUserto see if there are any variable name conflicts in the same scope
b32 Validator::check_shadowing(Declaration* d){DPZoneScoped;
    suLogger& logger = sufile->logger;
    forI(stacks.known_declarations.count - stacks.known_declarations_scope_begin_offsets[stacks.known_declarations_scope_begin_offsets.count-1]){
        Declaration* dk = stacks.known_declarations[stacks.known_declarations.count - 1 - i];
        if(dk==d) continue;
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
            logger.note(dk->token_start, "see original declaration of '", dk->identifier, "'");
            return 0;
        }
    }
    return 1;
}

Declaration* Validator::find_decl(str8 id){
    forI(stacks.known_declarations.count){
        Declaration* d = stacks.known_declarations[stacks.known_declarations.count-1-i];
        if(str8_equal_lazy(d->identifier, id)){
            return d;
        }
    }
    return 0;
}

suNode* Validator::validate(suNode* node){DPZoneScoped;
    suLogger& logger = sufile->logger;
    //TODO(sushi) we could possibly just store a validated flag on suNode 
    if(match_any(node->type, NodeType_Variable, NodeType_Structure, NodeType_Function) &&
       DeclarationFromNode(node)->validated){
        //early out if this declaration has already been validated
        return node;
    }
    switch(node->type){
        case NodeType_Variable:{
            Variable* v = VariableFromNode(node);

            push_variable(v);
            defer{pop_variable();};

            if(!check_shadowing(&v->decl)) return 0;
            stacks.known_declarations.add(&v->decl);

            if(v->data.type == Token_Struct){
                //we must determine if the struct this variable wants to use is valid
                Token* typespec = v->decl.token_start + 2;
                Declaration* d = find_decl(typespec->raw);
                if(!d){
                    logger.error(typespec, "unknown identifier '", typespec->raw, "'");
                    logger.note(typespec, "used as type specifier for '", v->decl.identifier, "'");
                    return 0;
                }
                if(d->type != Declaration_Structure){
                    logger.error(typespec, "identifier '", typespec->raw, "' does not represent a struct");
                    logger.note(typespec, "used as type specifier for '", v->decl.identifier, "'");
                    return 0;
                }
                v->data.struct_type = StructFromDeclaration(d);
            }
            
            if(v->data.implicit){
                Expression* e = ExpressionFromNode(validate(v->decl.node.first_child));
                if(!e) return 0;
                v->data = e->data;
            }else if(v->decl.node.child_count){
                Expression* e = ExpressionFromNode(validate(v->decl.node.first_child));
                if(!e) return 0;
                if(v->data.type != e->data.type || v->data.type == Token_Struct && e->data.type == Token_Struct){
                    //we must see if these types are compatible
                    if(e->data.type == Token_Struct){
                        //if the expression represents a structure, we must check if there is any conversion from 
                        //it to the variable
                        if(!e->data.struct_type->conversions.has(v->data.struct_type->decl.identifier)){
                            logger.error(v->decl.token_start, "no known conversion from ", get_typename(e), " to ", get_typename(v));
                            logger.note(v->decl.token_start, "you must define implicit(name:", get_typename(v), ") : ", get_typename(e));
                            return 0;
                        }
                        //TODO(sushi) inject conversion function
                    }
                    if(!type_conversion(v->data.type, e->data.type)){
                        logger.error(v->decl.token_start, "no known conversion from ", get_typename(e), " to ", get_typename(v));
                        return 0;
                    }
                    //if they are we inject a conversion node above the expression
                    Expression* cast = arena.make_expression(STR8("cast"));
                    cast->type = Expression_CastImplicit;
                    //NOTE(sushi) its possible that this could lead to misleading information if for some reason
                    //            something errors on these nodes that we make here. this should not happen because 
                    //            these nodes should always be correct, but something to keep in mind
                    cast->token_start = v->decl.token_start;
                    cast->token_end = v->decl.token_end;
                    cast->data = v->data;
                    insert_last(e->node.parent, &cast->node);
                    change_parent(&cast->node, &e->node);
                }
            }

            if(current.structure){
                //if we are currently validating a struct, we need to check a few things
                if(v->data.type==Token_Struct){
                    //check for self referencial defintion
                    if(v->data.struct_type == current.structure){
                        logger.error(v->decl.token_start, "a structure cannot contain a variable whose type is that structure.");
                        logger.note(v->decl.token_start, "use a pointer instead.");
                        return 0;
                    }
                    //check for circular defintions
                    forI(stacks.nested.structs.count){
                        if(stacks.nested.structs[i]==v->data.struct_type){
                            logger.error(v->decl.token_start, "circular definition.");
                            str8b b;
                            global_mem_lock.lock();
                            str8_builder_init(&b, current.structure->decl.identifier, deshi_temp_allocator);
                            str8_builder_append(&b, STR8(" -> "));
                            forX(j, stacks.nested.structs.count - i){
                                str8_builder_append(&b, stacks.nested.structs[j+i]->decl.identifier);
                                str8_builder_append(&b, STR8(" -> "));
                            }
                            str8_builder_append(&b, current.structure->decl.identifier);
                            global_mem_lock.unlock();
                            logger.note(v->decl.token_start, b.fin);
                            //NOTE(sushi) we do not free the builder here because of post-logging
                            return 0;
                        }
                    }
                    //pass the struct to validate to make sure it has been validated and has a size
                    if(!validate(&v->data.struct_type->decl.node)) return 0;
                    current.structure->size += v->data.struct_type->size;
                }else{
                    current.structure->size += builtin_sizes(v->data.type);
                }
            }
            v->decl.validated = 1;
            return &v->decl.node;
        }break;

        case NodeType_Function:{
            Function* f = FunctionFromNode(node);
            push_function(f);
            defer{pop_function();};

            if(!check_shadowing(&f->decl)) return 0;
            stacks.known_declarations.add(&f->decl);

            stacks.known_declarations_scope_begin_offsets.add(stacks.known_declarations.count);

            for_node(f->decl.node.first_child){
                if(!validate(it)) return 0;
            }

            stacks.known_declarations.pop(stacks.known_declarations.count - stacks.known_declarations_scope_begin_offsets.pop());

            f->decl.validated = 1;
            return &f->decl.node;
        }break;

        case NodeType_Structure:{
            Struct* s = StructFromNode(node);
            push_struct(s);
            defer{pop_struct();};
            if(!check_shadowing(&s->decl)) return 0;
            stacks.known_declarations.add(&s->decl);

            stacks.known_declarations_scope_begin_offsets.add(stacks.known_declarations.count);

            for_node(s->decl.node.first_child){
                if(!validate(it)) return 0;
            }

            stacks.known_declarations.pop(stacks.known_declarations.count - stacks.known_declarations_scope_begin_offsets.pop());

            s->decl.validated = 1;
            logger.log(Verbosity_Debug, "struct '", s->decl.identifier, "' validated with ", s->members.data.count, " members and has size ", s->size); 
            return &s->decl.node;
        }break;

        case NodeType_Scope:{
            Scope* s = ScopeFromNode(node);
            push_scope(s);
            defer{pop_scope();};

            stacks.known_declarations_scope_begin_offsets.add(stacks.known_declarations.count);

            for_node(s->node.first_child){
                if(!validate(it)) return 0;
            }

            stacks.known_declarations.pop(stacks.known_declarations.count - stacks.known_declarations_scope_begin_offsets.pop());
                
            return &s->node;
        }break;

        case NodeType_Statement:{
            Statement* s = StatementFromNode(node);
            push_statement(s);
            defer{pop_statement();};

            switch(s->type){
                
            }

            return &s->node;
        }break;

        case NodeType_Expression:{
            Expression* e = ExpressionFromNode(node);
            push_expression(e);
            defer{pop_expression();};
            
            switch(e->type){
                case Expression_Cast:{

                }break;
                case Expression_Reinterpret:{
                    //will have one node that is the typename we want to reinterpret to
                    Expression* type_name = ExpressionFromNode(e->node.first_child);
                    
                    Declaration* d = find_decl(type_name->token_start->raw);
                    if(!d){
                        logger.error(type_name->token_start, "unknown identifier '", type_name->token_start->raw, "'");
                        return 0;
                    }
                    if(d->type != Declaration_Structure){
                        logger.error(type_name->token_start, "identifier '", type_name->token_start->raw, "' is not a typename");
                        return 0;
                    }
                }break;
                case Expression_Identifier:{
                    Declaration* d = find_decl(e->token_start->raw);
                    if(!d){
                        logger.error(e->token_start, "unknown identifier '", e->token_start->raw, "'");
                        return 0;
                    }
                    //make sure this declaration is validated so we can get type information from it
                    if(!validate(&d->node)) return 0;
                    switch(d->type){
                        case Declaration_Variable:{
                            Variable* v = VariableFromDeclaration(d);
                            e->data = v->data;
                        }break;
                    }
                    
                }break;
                case Expression_BinaryOpAssignment:{

                }break;
            }

            return &e->node;
        }break;
    }
    return 0;
}

void Validator::start(){DPZoneScoped;
    suLogger& logger = sufile->logger;
    Stopwatch validate_time = start_stopwatch();
    logger.log(Verbosity_Stages, "Validating...");
    SetThreadName("Validating ", sufile->file->name);

    stacks.known_declarations_scope_begin_offsets.add(0);

    //wait for imported files to finish validation
    for(suFile* sf : sufile->preprocessor.imported_files){
        while(sf->stage < FileStage_Validator){
            sf->cv.validate.wait();
        }

    }

    for(Statement* s : sufile->parser.import_directives){
        for_node(s->node.first_child){
            Expression* e = ExpressionFromNode(it);
            //TODO(sushi) we already do this in Compiler::compile, so find a nice way to cache this if possible
            str8 filepath = e->token_start->raw;
            //TODO(sushi) it may be better to just interate the string backwards and look for / or \ instead
            u32 last_slash = str8_find_last(filepath, '/');
            if(last_slash == npos) last_slash = str8_find_last(filepath, '\\');
            if(last_slash == npos) last_slash = 0;
            str8 filename = str8{filepath.str+last_slash+1, filepath.count-(last_slash+1)};

            suFile* sufileex = compiler.files.atPtrVal(filename);

            if(!sufileex) logger.error("INTERNAL: an imported file was not processed in the compiler before reaching validator");

            if(e->node.child_count){

            }else{
                //we are just importing everything
                for(Declaration* d : sufileex->parser.imported_decl){
                    change_parent(&sufile->parser.base, &d->node);
                    move_to_parent_first(&d->node);
                    sufile->parser.imported_decl.add(d);
                }
                for(Declaration* d : sufileex->parser.exported_decl){
                    change_parent(&sufile->parser.base, &d->node);
                    move_to_parent_first(&d->node);
                    sufile->parser.imported_decl.add(d);
                }
            }
        }
    }

    for_node(sufile->parser.base.first_child){
        //add global declarations to known declarations
        if(match_any(it->type, NodeType_Function, NodeType_Variable, NodeType_Structure)){
            stacks.known_declarations.add(DeclarationFromNode(it));
        }
    }
    stacks.known_declarations_scope_begin_offsets.add(stacks.known_declarations.count);

    for_node(sufile->parser.base.first_child){
        validate(it);
    }

    
    
    sufile->stage = FileStage_Validator;
    sufile->cv.validate.notify_all();

    logger.log(Verbosity_Stages, "Validating finished in ", peek_stopwatch(validate_time), " ms.");
}