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
#define push_module(in)     {stacks.nested.modules.add(current.module); current.module = in;}
#define pop_module()        {current.module = stacks.nested.modules.pop();}

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
    case Token_Struct:     {  return false; }   \
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

//checks the known stack to see if there are any name conflicts in the same scope
b32 SemanticAnalyzer::check_shadowing(Declaration* d){DPZoneScoped;
    // amuLogger& logger = amufile->logger;
    // forI(stacks.known_declarations.count - stacks.known_declarations_scope_begin_offsets[stacks.known_declarations_scope_begin_offsets.count-1]){
    //     Declaration* dk = stacks.known_declarations[stacks.known_declarations.count - 1 - i];
    //     if(dk==d) continue;
    //     if(str8_equal_lazy(d->identifier, dk->identifier)){
    //         logger.error(d->token_start, 
    //         "declaration of ", 
    //         (d->type == Declaration_Variable ? "variable" : (d->type == Declaration_Function ? "function" : "structure")),
    //         " '", d->identifier, "' overrides declaration of ",
    //         (dk->type == Declaration_Variable ? "variable" : (dk->type == Declaration_Function ? "function" : "structure")),
    //         " '", d->identifier, "'");
    //         logger.note(dk->token_start, "see original declaration of '", dk->identifier, "'");
    //         amufile->semantic_analyzer.failed = 1;
    //         return 0;
    //     }
    // }
    return 1;
}

Declaration* SemanticAnalyzer::find_decl(str8 id){
    // forI(stacks.known_declarations.count){
    //     Declaration* d = stacks.known_declarations[stacks.known_declarations.count-1-i];
    //     if(str8_equal_lazy(d->identifier, id)){
    //         return d;
    //     }
    // }
    return 0;
}

b32 SemanticAnalyzer::can_type_convert(TypedValue* from, TypedValue* to){
    if(from->structure==to->structure) return true;
    return from->structure->conversions.has(get_typename(to->structure));
}

//NOTE(sushi) this is special because locally defined functions that overload something in a lesser scope
//            need to remove themselves from the overload tree that connects them
void SemanticAnalyzer::pop_known_decls(){
    // u32 n = stacks.known_declarations.count - stacks.known_declarations_scope_begin_offsets.pop();
    // forI(n){
    //     Declaration* d = stacks.known_declarations.pop();
    //     if(d->type == Declaration_Function){
    //         Function* f = (Function*)d;
    //     }
    // }
}

amuNode* SemanticAnalyzer::validate(amuNode* node){DPZoneScoped;
    /*amuLogger& logger = amufile->logger;
    //TODO(sushi) we could possibly just store a validated flag on amuNode 
    if(match_any(node->type, NodeType_Variable, NodeType_Structure, NodeType_Function) &&
       ((Declaration*)node)->validated){
        //early out if this declaration has already been validated
        
        return node;
    }
    if(globals.verbosity == Verbosity_Debug){
        switch(node->type){
            case NodeType_Expression: logger.log(Verbosity_Debug, "validating expression '", ((Expression*)node)->token_start->raw, "' of type ", ExTypeStrings[((Expression*)node)->type]); break;
            case NodeType_Function:   logger.log(Verbosity_Debug, "validating function ", ((Function*)node)->decl.identifier); break;
            case NodeType_Scope:      logger.log(Verbosity_Debug, "validating scope"); break;
            case NodeType_Statement:  logger.log(Verbosity_Debug, "validating statement '", ((Statement*)node)->token_start->raw, "'"); break;
            case NodeType_Structure:  logger.log(Verbosity_Debug, "validating struct '", ((Struct*)node)->decl.identifier, "'"); break;
            case NodeType_Variable:   logger.log(Verbosity_Debug, "validating variable '", ((Variable*)node)->decl.identifier, "'"); break;
            case NodeType_Module:     logger.log(Verbosity_Debug, "validating module '", ((Module*)node)->decl.identifier, "'."); break;

        }
    }
    switch(node->type){
        
        case NodeType_Module:{
            Module* m = (Module*)node;

            push_module(m);
            defer{pop_module();};

            if(!check_shadowing((Declaration*)m)) return 0;
            stacks.known_declarations.add((Declaration*)m);
            stacks.known_declarations_scope_begin_offsets.add(stacks.known_declarations.count);

            for_node(((amuNode*)m)->first_child){
                validate(it); 
            }

            pop_known_decls();

            m->decl.validated = 1;
            logger.log(Verbosity_Debug, "module '", m->decl.identifier, "' validated with ", m->decl.node.child_count, " members.");
            return (amuNode*)m;

        }break;

        case NodeType_Variable:{
            Variable* v = (Variable*)node;

            push_variable(v);
            defer{pop_variable();};

            if(!check_shadowing(&v->decl)) return 0;
            stacks.known_declarations.add(&v->decl);
            //if this variable's structure is null, then it is a user defined struct that we must look for
            if(!v->data.implicit && !v->data.structure){
                Token* typespec = v->decl.token_start + 2;
                Declaration* d = find_decl(typespec->raw);
                if(!d){
                    logger.error(typespec, "unknown identifier '", typespec->raw, "'");
                    logger.note(typespec, "used as type specifier for '", v->decl.identifier, "'");
                    amufile->semantic_analyzer.failed = 1;
                    return 0;
                }
                if(d->type != Declaration_Structure){
                    logger.error(typespec, "identifier '", typespec->raw, "' does not represent a struct.");
                    logger.note(typespec, "used as type specifier for '", v->decl.identifier, "'");
                    amufile->semantic_analyzer.failed = 1;
                    return 0;
                }
                v->data.structure = (Struct*)d;
            }
            
            if(v->data.implicit){
                Expression* e = (Expression*)validate(v->decl.node.first_child);
                if(!e) return 0;
                v->data = e->data;
            }else if(v->decl.node.child_count){
                Expression* e = (Expression*)validate(((amuNode*)v)->first_child);
                if(!e) return 0;
                if(v->data.structure != e->data.structure){
                    if(!e->data.structure->conversions.at(v->data.structure->decl.identifier)){
                        logger.error(v->decl.token_start, "no known conversion from ", get_typename(e), " to ", get_typename(v));
                        logger.note(v->decl.token_start, ErrorFormat("(Not Implemented)"), "for implicit conversion define implicit(name:", get_typename(e), ") : ", get_typename(v));
                        amufile->semantic_analyzer.failed = 1;
                        return 0;
                    }
                    //if types are compatible just inject a conversion node 
                    //we dont inject the actual conversion process because that would require us to copy nodes
                    //instead we just put a cast node here indicating which type to convert to and it's handled later
                    Expression* cast = arena.make_expression(STR8("cast"));
                    cast->type = Expression_CastImplicit;
                    cast->token_start = v->decl.token_start;
                    cast->token_end = v->decl.token_end;
                    cast->data = v->data;
                    insert_last(e->node.parent, &cast->node);
                    change_parent(&cast->node, &e->node);
                }
            }

            if(current.structure){
                //if we are currently validating a struct, we need to check a few things
                //these checks are only necessary if the type is not built in
                if(!is_builtin_type(v)){
                    //check for self referencial definition
                    if(v->data.structure == current.structure){
                        logger.error(v->decl.token_start, "a structure cannot contain a variable whose type is that structure.");
                        logger.note(v->decl.token_start, "use a pointer instead.");
                        amufile->semantic_analyzer.failed = 1;
                        return 0;
                    }

                    forI(stacks.nested.structs.count){
                        if(stacks.nested.structs[i]==v->data.structure){
                            logger.error(v->decl.token_start, "circular definition.");
                            str8b b;
                            mutex_lock(&global_mem_lock);
                            str8_builder_init(&b, current.structure->decl.identifier, deshi_temp_allocator);
                            str8_builder_append(&b, STR8(" -> "));
                            forX(j, stacks.nested.structs.count - i){
                                str8_builder_append(&b, stacks.nested.structs[j+i]->decl.identifier);
                                str8_builder_append(&b, STR8(" -> "));
                            }
                            str8_builder_append(&b, current.structure->decl.identifier);
                            mutex_unlock(&global_mem_lock);
                            logger.note(v->decl.token_start, b.fin);
                            //NOTE(sushi) we do not free the builder here because of post-logging
                            amufile->semantic_analyzer.failed = 1;
                            return 0;
                        }
                    }

                    //pass the struct to validate to make sure it has been validated and has a size
                    if(!validate(&v->data.structure->decl.node)) return 0;
                }
                current.structure->size += v->data.structure->size;
            }
            v->decl.validated = 1;
            return &v->decl.node;
        }break;

        case NodeType_Function:{
            Function* f = (Function*)node;
            push_function(f);
            defer{pop_function();};

            stacks.known_declarations.add(&f->decl);
            stacks.known_declarations_scope_begin_offsets.add(stacks.known_declarations.count);
            
            Declaration* d = (Declaration*)current.module->functions.at(f->decl.identifier);
            if(!d){
                //this is the first occurance of this function that we know of so we add it to the function map
                d = (Declaration*)current.module->functions.atIdx(current.module->functions.add(f->decl.identifier, &arena.make_function(amuStr8("funcgroup of ", f->decl.identifier))->decl.node));
                d->identifier = f->decl.identifier;
                ((Function*)d)->overloads.init();
            }        
            
            Function* fg = (Function*)d;
            fg->overloads.add(f);

            if(!f->data.structure){
                Token* typespec = f->decl.token_start + 2;
                Declaration* d = find_decl(typespec->raw);
                if(!d){
                    logger.error(typespec, "unknown identifier '", typespec->raw, "'");
                    logger.note(typespec, "used as type specifier for '", f->decl.identifier, "'");
                    return 0;
                }
                if(d->type != Declaration_Structure){
                    logger.error(typespec, "identifier '", typespec->raw, "' does not represent a struct");
                    logger.note(typespec, "used as type specifier for '", f->decl.identifier, "'");
                    amufile->semantic_analyzer.failed = 1;
                    return 0;
                }
                f->data.structure = (Struct*)d;
            }

            b32 found_nonpos = 0; //set when we find an argument that has a default value
            amuNode* def = 0;

            for_node(f->decl.node.first_child){
                //we must save the defintion and validate it later for reasons explained below
                if(it->type == NodeType_Scope) {def = it; continue;}
                if(!validate(it)) return 0;

                Variable* arg = (Variable*)it;
                f->args.add(arg);
                
                if(it->child_count){
                    //this is an arg with a default value 
                    found_nonpos = 1;
                }else if(found_nonpos){
                    logger.error(arg->decl.token_start, "positional arguments cannot follow defaulted arguments.");
                    amufile->semantic_analyzer.failed = 1;
                    return 0;
                }
            }
            
            if(fg->overloads.count > 1)
            forI(fg->overloads.count){
                Function* ol = fg->overloads[i];
                if(f==ol) continue;
                forX(j, f->args.count){
                    Variable* myarg = f->args[j];
                    Variable* olarg = ol->args[j];
                    if(!types_match(myarg, olarg)) break;

                    if(j == f->args.count - 1){
                        if(ol->args.count > f->args.count){
                            if(ol->args[j+1]->decl.node.first_child){
                                //AddFlag(f->decl.node.flags, FunctionFlag_ERRORED);
                                logger.error(f->decl.token_start, "calls to this function will be ambiguous.");
                                logger.note(ol->decl.token_start, "see function that makes it ambiguous.");
                                amufile->semantic_analyzer.failed = 1;
                            }
                        }else{
                            //AddFlag(f->decl.node.flags, FunctionFlag_ERRORED);
                            logger.error(f->decl.token_start, "overloads must differ by type.");
                            logger.note(ol->decl.token_start, "see conflicting function.");
                            amufile->semantic_analyzer.failed = 1;
                        }
                    }else if(j == ol->args.count - 1){
                        if(f->args[j+1]->decl.node.first_child){
                            //AddFlag(ol->decl.node.flags, FunctionFlag_ERRORED);
                            logger.error(ol->decl.token_start, "calls to this function will be ambiguous.");
                            logger.note(f->decl.token_start, "see function that makes it ambiguous.");
                            amufile->semantic_analyzer.failed = 1;
                        }
                        break;
                    }

                }
            }

            f->decl.validated = 1;

            //this is very silly
            //in order to ensure that all of a function's overloads are validated before they can ever be used
            //we iterate the entire stacks array looking for functions of the same name to validate
            //before we validate this function's scope 
            //this probably wastes a lot of time and a better solution should be looked into
            forI(stacks.known_declarations.count){
                Declaration* de = stacks.known_declarations[i];
                if(str8_equal_lazy(de->identifier, d->identifier)){
                    validate(&de->node);
                }
            }

            if(!validate(def)) return 0;
            
            pop_known_decls();

            return &f->decl.node;
        }break;

        case NodeType_Structure:{
            Struct* s = (Struct*)node;
            push_struct(s);
            defer{pop_struct();};
            if(!check_shadowing(&s->decl)) return 0;

            stacks.known_declarations.add(&s->decl);
            stacks.known_declarations_scope_begin_offsets.add(stacks.known_declarations.count);

            for_node(s->decl.node.first_child){
                validate(it);
            }

            pop_known_decls();

            s->decl.validated = 1;
            logger.log(Verbosity_Debug, "struct '", s->decl.identifier, "' validated with ", s->members.data.count, " members and has size ", s->size); 
            return &s->decl.node;
        }break;

        case NodeType_Scope:{
            Scope* s = (Scope*)node;
            push_scope(s);
            defer{pop_scope();};

            stacks.known_declarations_scope_begin_offsets.add(stacks.known_declarations.count);

            for_node(s->node.first_child){
                if(!validate(it)) return 0;
            }

            pop_known_decls();
                
            return &s->node;
        }break;

        case NodeType_Statement:{
            Statement* s = (Statement*)node;
            push_statement(s);
            defer{pop_statement();};

            switch(s->type){
                case Statement_Expression:{
                    for_node(s->node.first_child){
                        //NOTE(sushi) we do not early out on 0 here so we dont just quit on the first error
                        //            statements should have enough separation to not mess up probably
                        validate(it);
                    }
                }break;
                case Statement_Return:{
                    if(!current.function){
                        logger.error(s->token_start, "cannot use a return statement outside of a function.");
                        amufile->semantic_analyzer.failed = 1;
                    }
                    for_node(s->node.first_child){
                        Expression* e = (Expression*)validate(it);
                        if(!e) return 0;

                    }
                }break;
                case Statement_Import:{
                    for_node(s->node.first_child){
                        Expression* e = (Expression*)it;
                        switch(e->type){
                            case Expression_BinaryOpAs:{
                                // probably not necessary to validate
                                Expression* lhs = (Expression*)validate(e->node.first_child);
                                if(!lhs) return 0;
                                Expression* rhs = (Expression*)e->node.last_child;
                                
                                Module* m = arena.make_module(rhs->token_start->raw);
                                m->decl.type = Declaration_Module;
                                m->decl.token_start = rhs->token_start;
                                m->decl.token_end = rhs->token_start;
                                m->decl.identifier = rhs->token_start->raw;
                                m->decl.declared_identifier = rhs->token_start->raw;
                                m->functions.init();

                                str8 filepath = lhs->token_start->raw;
                                str8 filename = {0};
                                //TODO(sushi) figure out if unicode messes with doing this with normal count
                                forI_reverse(filepath.count){
                                    if(filepath.str[i] == '\\' || filepath.str[i] == '/'){
                                        filename = str8{filepath.str+i+1, filepath.count-i-1};
                                        break;
                                    }
                                }

                                amuFile* sufileex = compiler.files.atPtrVal(filename);

                                if(!sufileex){
                                    logger.error("INTERNAL: an imported file was not processed in the compiler before reaching validator");
                                    amufile->semantic_analyzer.failed = 1;
                                } 

                                if(lhs->node.child_count){
                                    // we are selectively importing things
                                    for_node(lhs->node.first_child){
                                        Expression* e = (Expression*)it;
                                        // this sucks, maybe store the declarations in a map
                                        Declaration* found = 0;
                                        for(Declaration* d : sufileex->syntax_analyzer.imported_decl){
                                            if(str8_equal(d->identifier, e->token_start->raw)){
                                                found = d;
                                                break;
                                            }
                                        }

                                        if(!found) for(Declaration* d : sufileex->syntax_analyzer.exported_decl){
                                            if(str8_equal(d->identifier, e->token_start->raw)){
                                                found = d;
                                                break;
                                            }
                                        }

                                        if(!found){
                                            logger.error(e->token_start, "identifier '", e->token_start->raw, "' either does not exist or is not exported from ", filename, ".");
                                            amufile->semantic_analyzer.failed = 1;
                                            continue;
                                        }

                                        change_parent((amuNode*)m, &found->node);
                                        move_to_parent_first(&found->node);
                                    }
                                }else{
                                    //we are just importing everything
                                    for(Declaration* d : sufileex->syntax_analyzer.imported_decl){
                                        change_parent((amuNode*)m, &d->node);
                                        move_to_parent_first(&d->node);
                                    }
                                    for(Declaration* d : sufileex->syntax_analyzer.exported_decl){
                                        change_parent((amuNode*)m, &d->node);
                                        move_to_parent_first(&d->node);
                                    }
                                    //copy function map as well
                                    //this kind of sucks and i want to make it better eventually
                                    forI(sufileex->module->functions.data.count){
                                        m->functions.add(((Declaration*)sufileex->module->functions.atIdx(i))->identifier, sufileex->module->functions.atIdx(i));
                                    }
                                }

                                if(!validate((amuNode*)m)) return 0;

                            }break;
                        }
                    }
                  
                    
                    
                    // //TODO(sushi) we already do this in Compiler::compile, so find a nice way to cache this if possible
                    // str8 filepath = e->token_start->raw;
                    // //TODO(sushi) it may be better to just interate the string backwards and look for / or \ instead
                    // u32 last_slash = str8_find_last(filepath, '/');
                    // if(last_slash == npos) last_slash = str8_find_last(filepath, '\\');
                    // if(last_slash == npos) last_slash = 0;
                    // str8 filename = str8{filepath.str+last_slash+1, filepath.count-(last_slash+1)};

                    // amuFile* sufileex = compiler.files.atPtrVal(filename);

                    // if(!sufileex){
                    //     logger.error("INTERNAL: an imported file was not processed in the compiler before reaching validator");
                    //     amufile->validator.failed = 1;
                    // } 

                

                    // if(e->node.child_count){
                        
                    // }else{
                    //     //we are just importing everything
                    //     for(Declaration* d : sufileex->syntax_analyzer.imported_decl){
                    //         change_parent(&amufile->syntax_analyzer.base, &d->node);
                    //         move_to_parent_first(&d->node);
                    //         amufile->syntax_analyzer.imported_decl.add(d);
                    //     }
                    //     for(Declaration* d : sufileex->syntax_analyzer.exported_decl){
                    //         change_parent(&amufile->syntax_analyzer.base, &d->node);
                    //         move_to_parent_first(&d->node);
                    //         amufile->syntax_analyzer.imported_decl.add(d);
                    //     }
                    //     //copy function map as well
                    //     //this kind of sucks and i want to make it better eventually
                    //     forI(sufileex->validator.functions.data.count){
                    //         amufile->validator.functions.add(((Declaration*)sufileex->validator.functions.atIdx(i))->identifier, sufileex->validator.functions.atIdx(i));
                    //     }
                    // }
                }break;
            }

            return &s->node;
        }break;

        case NodeType_Expression:{
            Expression* e = (Expression*)node;
            push_expression(e);
            defer{pop_expression();};
            
            switch(e->type){
                case Expression_Cast:{

                }break;
                case Expression_Reinterpret:{
                    //will have one node that is the typename we want to reinterpret to
                    Expression* type_name = (Expression*)e->node.first_child;
                    
                    Declaration* d = find_decl(type_name->token_start->raw);
                    if(!d){
                        logger.error(type_name->token_start, "unknown identifier '", type_name->token_start->raw, "'");
                        amufile->semantic_analyzer.failed = 1;
                        return 0;
                    }
                    if(d->type != Declaration_Structure){
                        logger.error(type_name->token_start, "identifier '", type_name->token_start->raw, "' is not a typename");
                        amufile->semantic_analyzer.failed = 1;
                        return 0;
                    }
                }break;
                case Expression_Identifier:{
                    Declaration* d = find_decl(e->token_start->raw);
                    if(!d){
                        logger.error(e->token_start, "unknown identifier '", e->token_start->raw, "'.");
                        amufile->semantic_analyzer.failed = 1;
                        return 0;
                    }
                    //make sure this declaration is validated so we can get type information from it
                    if(!validate(&d->node)) return 0;
                    switch(d->type){
                        case Declaration_Variable:{
                            Variable* v = (Variable*)d;
                            e->data = v->data;
                            e->type = Expression_IdentifierVariable;
                        }break;
                        case Declaration_Module:{
                            e->type = Expression_IdentifierModule;
                            e->data.structure = compiler.builtin.types.void_;
                        }break;
                        case Declaration_Structure:{
                            e->type = Expression_IdentifierStruct;
                            e->data.structure = compiler.builtin.types.void_;
                        }break;
                        case Declaration_Function:{
                            e->type = Expression_IdentifierFunction;
                            e->data.structure = compiler.builtin.types.void_;
                        }break;
                    }
                    // TODO(sushi) re-do how this works, there must be a better way to communicate this situation.
                    if(e->node.child_count == 1){
                        if(e->node.first_child->type == NodeType_Expression){
                            if(((Expression*)e->node.first_child)->type == Expression_InitializerList){
                                if(d->type != Declaration_Structure){
                                    logger.error(e->token_start, "an identifier preceeding an initializer list must be a typename, found the name of a ", (d->type == Declaration_Function ? "function" : "variable"), " instead.");
                                    amufile->semantic_analyzer.failed = 1;
                                    return 0;
                                }
                                if(!validate(e->node.first_child)) return 0;
                                
                                e->data = ((Expression*)e->node.first_child)->data;
                            }else{
                                logger.error(e->token_start, "INTERNAL ERROR: an Expression_Identifier has a child node that is not an initializer list, this shouldn't happen. NOTE(sushi) tell me about this.");
                                amufile->semantic_analyzer.failed = 1;
                                return 0;
                            } 
                        }else{
                            logger.error(d->token_start, "INTERNAL ERROR: an Expression_Identifier has a child node that is not an expression, this shouldn't happen. NOTE(sushi) tell me about this.");
                            amufile->semantic_analyzer.failed = 1;
                            return 0;
                        }
                    }
                }break;
                case Expression_FunctionCall:{
                    //TODO(sushi)
                    //validate call id
                    // an entire function must be represented as a module
                    Declaration* d = (Declaration*)current.module->functions.at(e->token_start->raw);
                    //this is kind of ugly                    
                    if(!d){
                        //its possible this function has not been validated yet so we look for it again in our known decls list
                        //and if its a function run validate on it 
                        d = find_decl(e->token_start->raw);
                        if(d){
                            if(d->type == Declaration_Function){
                                if(!validate(&d->node)) return 0;
                                d = (Declaration*)current.module->functions.at(e->token_start->raw);
                            }else{
                                logger.error(e->token_start, "attempt to call '", e->token_start->raw, "', but it is not a function.");
                                logger.note(e->token_start, e->token_start->raw, " is a ", (d->type == Declaration_Structure ? "structure." : "variable."));
                                amufile->semantic_analyzer.failed = 1;
                                return 0;    
                            }
                        }else{
                            logger.error(e->token_start, "attempt to call unknown identifier '", e->token_start->raw, "'");
                            amufile->semantic_analyzer.failed = 1;
                            return 0;
                        }
                    }
                    Function* fg = (Function*)d;
                    
                    //we need to match this call to an overload
                    //we start by gathering the arguments this call has as well as counting how many positional arguments there are
                    //we also do our check for using a positional argument after a named arg here
                    amuArena<Expression*> arguments; arguments.init(); defer{arguments.deinit();};
                    u32 n_pos_args = 0;
                    b32 found_named = 0;
                    for_node(e->node.first_child){
                        if(((Expression*)it)->type == Expression_BinaryOpAssignment){
                            found_named = 1;
                        }else if(found_named){
                            logger.error(((Expression*)it)->token_start, "positional arguments cannot come after named arguments.");
                            amufile->semantic_analyzer.failed = 1;
                            return 0;
                        }else{
                            n_pos_args++;
                        }
                        if(!found_named && !validate(it)) return 0;
                        arguments.add((Expression*)it);
                    }

                    // we remove the argument nodes from the call expression because we will be adding them 
                    // back in the correct order later
                    forI(arguments.count){
                        change_parent(0, &arguments[i]->node);
                    }
                    
                    //next we store all the overloads in a separate array so we can trim it
                    //we can filter out any functions whose argument count is less than whats in the call
                    //TODO(sushi) it may be more efficient to use a view of the original array and moving its elements around rather than
                    //            copying potentially the entire thing
                    u32 min_req_args = -1; // the minimum amount of args required to call this overload 
                    amuArena<Function*> overloads; overloads.init(); defer{overloads.deinit();};
                    forI(fg->overloads.count){
                        Function* ol = fg->overloads[i];
                        min_req_args = Min(min_req_args, ol->args.count - ol->default_count);
                        if(ol->args.count >= arguments.count && ol->args.count-ol->default_count <= arguments.count){
                            overloads.add(ol);
                        }
                    }

                    // it's possible we figure it out immediately, so just skip all the filtering
                    if(overloads.count == 1) goto skip_checks;

                    if(arguments.count < min_req_args){
                        logger.error(e->token_start, "no overload of ", fg->decl.identifier, " takes just ", arguments.count, " arguments. The least arguments acceptable is ", min_req_args, ".");
                        amufile->semantic_analyzer.failed = 1;
                        return 0;
                    }

                    //early check that the user doesnt give too many arguments for all overloads
                    if(!overloads.count){
                        logger.error(e->token_start, "too many arguments given in call to '", e->token_start->raw, "'.");
                        amufile->semantic_analyzer.failed = 1;
                        return 0;
                    }

                    //now we trim the list using positional args
                    //in order to remove a function, the type of the positional arg has to be different and not convertable to the function's argument
                    forX(ci, n_pos_args){
                        Expression* carg = arguments[ci];
                        //NOTE(sushi) iterate in reverse so when we remove an element we dont have to adjust the index
                        forX_reverse(fi, overloads.count){
                            Variable* farg = overloads[fi]->args[ci];
                            if(!can_type_convert(&farg->data, &carg->data)){
                                overloads.remove(fi);
                                if(!overloads.count){
                                    logger.error(e->token_start, "unable to match positional arguments to any overload of '", e->token_start->raw, "'.");
                                    amufile->semantic_analyzer.failed = 1;
                                    //TODO(sushi) there is more information we can give here
                                    return 0;
                                }
                            }
                        }
                    }

                    //now we trim the list using named arguments 
                    //in order to remove a function, it has to not have any arguments that match the name
                    //or the named argument's type doesnt match and isnt convertable
                    forX(ci, arguments.count-n_pos_args){
                        Expression* carg = arguments[ci+n_pos_args];
                        Expression* lhs = (Expression*)carg->node.first_child;
                        if(lhs->type != Expression_Identifier){
                            logger.error(lhs->token_start, "expected an identifier for lhs of named argument.");
                            amufile->semantic_analyzer.failed = 1;
                            return 0;
                        }
                        Expression* rhs = (Expression*)validate(carg->node.last_child);
                        if(!rhs) return 0;
                        forX_reverse(fi, overloads.count){
                            Function* ol = overloads[fi];
                            b32 found = 0;
                            forI(ol->args.count - n_pos_args){
                                Variable* farg = ol->args[i+n_pos_args];
                                if(str8_equal_lazy(farg->decl.identifier, lhs->token_start->raw)){
                                    if(!can_type_convert(&farg->data, &rhs->data)) break;
                                    found = 1;
                                    break;
                                }
                            }
                            if(!found){
                                //we need to make sure that the user isn't trying to name a positional argument that was already set
                                forI(n_pos_args){
                                    Variable* farg = ol->args[i];
                                    if(str8_equal_lazy(farg->decl.identifier, lhs->token_start->raw)){
                                        logger.error(lhs->token_start, "argument '", lhs->token_start->raw, "' has already been specified by a positional argument.");
                                        amufile->semantic_analyzer.failed = 1;
                                    }
                                }
                                overloads.remove(fi);
                            }
                        }
                    }

                    //if there is still more than one overload in the list we need to be more strict about types, filtering out overloads who 
                    //dont have any types matching passed types. As long as one of an overload's argument's types matches a call argument it can stay
                    if(overloads.count > 1){
                        forX_reverse(fi, overloads.count){
                            Function* ol = overloads[fi];
                            b32 pos_has_matching = 0;
                            forI(n_pos_args){
                                Variable* farg = ol->args[i];
                                Expression* carg = arguments[i];
                                if(types_match(farg, carg)){
                                    pos_has_matching = 1; break;
                                }else{
                                    //we have a special case here where we allow a function to pass based on scalar types ignoring size.
                                    //so if an argument doesnt match because it's size is different, but it's underlying type 
                                    //(signed, unsigned, float) is the same, then we allow it
                                    #define subcase(b0,b1,b2,b3,b4,b5,b6,b7,b8,b9)\
                                    switch(carg->data.structure->type){\
                                        case DataType_Signed8:    if(b0)pos_has_matching=1; break;\
                                        case DataType_Signed16:   if(b1)pos_has_matching=1; break;\
                                        case DataType_Signed32:   if(b2)pos_has_matching=1; break;\
                                        case DataType_Signed64:   if(b3)pos_has_matching=1; break;\
                                        case DataType_Unsigned8:  if(b4)pos_has_matching=1; break;\
                                        case DataType_Unsigned16: if(b5)pos_has_matching=1; break;\
                                        case DataType_Unsigned32: if(b6)pos_has_matching=1; break;\
                                        case DataType_Unsigned64: if(b7)pos_has_matching=1; break;\
                                        case DataType_Float32:    if(b8)pos_has_matching=1; break;\
                                        case DataType_Float64:    if(b9)pos_has_matching=1; break;\
                                    }
                                    switch(farg->data.structure->type){
                                        case DataType_Signed8:    subcase(1,1,1,1,1,1,1,1,0,0); break; 
                                        case DataType_Signed16:   subcase(1,1,1,1,1,1,1,1,0,0); break;
                                        case DataType_Signed32:   subcase(1,1,1,1,1,1,1,1,0,0); break;
                                        case DataType_Signed64:   subcase(1,1,1,1,1,1,1,1,0,0); break;
                                        case DataType_Unsigned8:  subcase(1,1,1,1,1,1,1,1,0,0); break;
                                        case DataType_Unsigned16: subcase(1,1,1,1,1,1,1,1,0,0); break;
                                        case DataType_Unsigned32: subcase(1,1,1,1,1,1,1,1,0,0); break;
                                        case DataType_Unsigned64: subcase(1,1,1,1,1,1,1,1,0,0); break;
                                        case DataType_Float32:    subcase(0,0,0,0,0,0,0,0,1,1); break;
                                        case DataType_Float64:    subcase(0,0,0,0,0,0,0,0,1,1); break;
                                    }
                                    #undef subcase
                                }
                            }
                            b32 non_pos_has_matching = 0;
                            forX(ci, arguments.count - n_pos_args){
                                Expression* carg = arguments[ci+n_pos_args];
                                Expression* lhs = (Expression*)carg->node.first_child;
                                Expression* rhs = (Expression*)carg->node.last_child;
                                b32 match = 0;
                                forI(ol->args.count - n_pos_args){
                                    Variable* farg = ol->args[i];
                                    if(!str8_equal_lazy(lhs->token_start->raw, farg->decl.identifier)) continue;
                                    if(types_match(rhs, farg)) break;
                                    non_pos_has_matching = 1; break;
                                }
                            }
                            if(!(non_pos_has_matching || pos_has_matching)){
                                overloads.remove(fi);
                            } 
                        }
                    }

                    if(!overloads.count){
                        logger.error(e->token_start, "unable to match function call to any overload of ", e->token_start->raw);
                        amufile->semantic_analyzer.failed = 1;
                        return 0;
                    }else if(overloads.count > 1){
                        logger.error(e->token_start, "call to ", e->token_start->raw, " is ambiguous.");
                        //TODO(sushi) remove this following line or make it more useful by showing what types the arguments were
                        logger.note(e->token_start, "call was ", show(e));
                        forI(overloads.count){
                            logger.note(overloads[i]->decl.token_start, "could be ", show(overloads[i]));
                        }
                        amufile->semantic_analyzer.failed = 1;
                        return 0;
                    }
skip_checks:
                    //now we only have 1 overload chosen so we must change the call to directly represent it
                    //we reorganize the named arguments to be in order and remove the assignment, so it appears 
                    //as though this was a normal function call to later stages
                    Function* pick = overloads[0];
                    logger.log(Verbosity_Debug, "call to ", e->token_start->raw, " picked overload ", show(pick));
                    forI(pick->args.count){
                        Variable* farg = pick->args[i];
                        b32 found = 0;
                        if(i<n_pos_args){
                            Expression* carg = arguments[i];
                            if(farg->data.structure->size < carg->data.structure->size){
                                logger.warn(carg->token_start, "argument passed to ", pick->decl.identifier, " is larger than the size of the parameter. The value may be narrowed.");
                            } 

                            if(!types_match(farg, carg)){
                                Expression* cast = arena.make_expression(STR8("cast"));
                                cast->type = Expression_CastImplicit;
                                cast->data = farg->data;
                                insert_last(&cast->node, &carg->node);
                                insert_last(&e->node, &cast->node);   
                            }else{
                                insert_last(&e->node, &carg->node);   
                            }
                            found = 1;
                        }else{
                            forX_reverse(j,arguments.count-n_pos_args){
                                Expression* carg = arguments[arguments.count-1-j];
                                Expression* lhs = (Expression*)carg->node.first_child;
                                if(!lhs) continue;
                                if(str8_equal_lazy(lhs->token_start->raw, farg->decl.identifier)){
                                    if(!types_match(farg, carg)){
                                        Expression* cast = arena.make_expression(STR8("cast"));
                                        cast->type = Expression_CastImplicit;
                                        cast->data = farg->data;
                                        insert_last(&cast->node, carg->node.last_child);
                                        insert_last(&e->node, &cast->node);
                                    }else{
                                        insert_last(&e->node, carg->node.last_child);
                                    }
                                    arguments.remove(arguments.count-1-j);
                                    found = 1;
                                    break;
                                }
                            }
                        }
                        if(!found && !farg->initialized){
                            logger.error(e->token_start, "missing argument ", farg->decl.identifier, " in call to ", pick->decl.identifier);
                            amufile->semantic_analyzer.failed = 1;
                        }
                    }

                    e->data = pick->data;
                }break;
                case Expression_UnaryOpNegate:{
                    Expression* u = (Expression*)validate(((amuNode*)e)->first_child);
                    if(!u) return 0;
                    e->data = u->data;
                }break;
                //NOTE(sushi) performing bitwise operations on floats without reinterpretting is allowed, but they are always coerced to int
                //bit binary ops are special because they will always return an integer
                case Expression_BinaryOpBitOR:
                case Expression_BinaryOpBitXOR:
                case Expression_BinaryOpBitShiftLeft:
                case Expression_BinaryOpBitShiftRight:
                case Expression_BinaryOpBitAND:{
                    Expression* lhs = (Expression*)validate(e->node.first_child);
                    Expression* rhs = (Expression*)validate(e->node.last_child);
                    logger.log(Verbosity_Debug, "  validating bitwise binary operator");
                    if(is_float(rhs)){
                        logger.log(Verbosity_Debug, "    right hand side is float, inserting reinterpret cast");
                        Expression* rein = arena.make_expression(STR8("reinterpret : s64"));
                        rein->type = Expression_Reinterpret;
                        rein->token_start = rhs->token_start;
                        rein->token_end = rhs->token_end;
                        rein->data.structure = compiler.builtin.types.signed64;
                        insert_above(&rhs->node, &rein->node);
                    }else if(!is_int(rhs)){
                        logger.error(e->token_start, "the operator ", ExTypeStrings[e->type], " is not defined between types ", get_typename(lhs), " and ", get_typename(rhs), ".");
                        amufile->semantic_analyzer.failed = 1;
                        return 0;
                    }
                    if(is_float(lhs)){
                        logger.log(Verbosity_Debug, "    left hand side is float, inserting reinterpret cast");
                        Expression* rein = arena.make_expression(STR8("reinterpret : s64"));
                        rein->type = Expression_Reinterpret;
                        rein->token_start = lhs->token_start;
                        rein->token_end = lhs->token_end;
                        rein->data.structure = compiler.builtin.types.signed64;
                        insert_above(&lhs->node, &rein->node);
                    }else if(!is_int(lhs)){
                        logger.error(e->token_start, "the operator ", ExTypeStrings[e->type], " is not defined between types ", get_typename(lhs), " and ", get_typename(rhs), ".");
                        amufile->semantic_analyzer.failed = 1;
                        return 0;
                    }
                    // a bitwise operation always returns a signed 64 bit integer
                    // TODO(sushi) should it?
                    e->data.structure = compiler.builtin.types.signed64;
                }break;
                
                case Expression_BinaryOpPlus:
                case Expression_BinaryOpMinus:
                case Expression_BinaryOpMultiply:
                case Expression_BinaryOpDivision:{
                    Expression* lhs = (Expression*)validate(e->node.first_child);
                    Expression* rhs = (Expression*)validate(e->node.last_child);
                    logger.log(Verbosity_Debug, "  validating basic arithmatic operation");
                    if(rhs->data.structure->type > lhs->data.structure->type || rhs->data.structure == lhs->data.structure){
                        logger.log(Verbosity_Debug, "    the result of the operation will take the type of the rhs: ", get_typename(rhs->data.structure));
                        e->data.structure = rhs->data.structure;
                    }else if(rhs->data.structure->type < lhs->data.structure->type){
                        logger.log(Verbosity_Debug, "    the result of the operation will take the type of the lhs: ", get_typename(lhs->data.structure));
                        e->data.structure = lhs->data.structure;
                    }
                }break;

                case Expression_BinaryOpAND:
                case Expression_BinaryOpOR:{
                    Expression* lhs = (Expression*)validate(e->node.first_child);
                    Expression* rhs = (Expression*)validate(e->node.last_child);
                    NotImplemented;
                }break;

                case Expression_BinaryOpModulo:{
                    Expression* lhs = (Expression*)validate(e->node.first_child);
                    Expression* rhs = (Expression*)validate(e->node.last_child);
                    NotImplemented;
                }break;

                case Expression_BinaryOpLessThan:
                case Expression_BinaryOpGreaterThan:
                case Expression_BinaryOpLessThanOrEqual:
                case Expression_BinaryOpGreaterThanOrEqual:
                case Expression_BinaryOpEqual:
                case Expression_BinaryOpNotEqual:{
                    Expression* lhs = (Expression*)validate(e->node.first_child);
                    Expression* rhs = (Expression*)validate(e->node.last_child);
                    NotImplemented;
                }break;

                case Expression_BinaryOpAs:{
                    Expression* lhs = (Expression*)validate(e->node.first_child);
                    Expression* rhs = (Expression*)validate(e->node.last_child);
                    NotImplemented;
                }break;

                case Expression_BinaryOpAssignment:{
                    Expression* lhs = (Expression*)validate(e->node.first_child);
                    Expression* rhs = (Expression*)validate(e->node.last_child);
                    if(!types_match(lhs, rhs)){
                        if(!can_type_convert(&rhs->data, &lhs->data)){
                            logger.error(rhs->token_start, "no known conversion from ", get_typename(rhs), " to ", get_typename(lhs));
                            amufile->semantic_analyzer.failed = 1;
                            return 0;
                        }
                        Expression* cast = arena.make_expression(amuStr8("cast ", get_typename(rhs), " to ", get_typename(lhs)));
                        cast->type = Expression_CastImplicit;
                        cast->data = lhs->data;
                        insert_above((amuNode*)rhs, (amuNode*)cast);
                    }else{
                        e->data = rhs->data;
                    }
                }break;
                
                case Expression_BinaryOpMemberAccess:{
                    logger.log(Verbosity_Debug, "  validating member access");
                    Expression* lhs = (Expression*)validate(e->node.first_child);
                    Expression* rhs = (Expression*)e->node.last_child;
                    if(!lhs) return 0;
                    

                    switch(lhs->type){
                        case Expression_IdentifierFunction:{
                            logger.error("invalid use of member access. the identifier '", lhs->token_start->raw, "' belongs to a function.");
                            amufile->semantic_analyzer.failed = 1;
                            return 0;
                        }break;
                        case Expression_IdentifierModule:{
                            
                        }break;
                        case Expression_IdentifierVariable:{
                            Struct* structure = lhs->data.structure;
                            if(!validate((amuNode*)structure)) return 0;
                            Variable* member = (Variable*)structure->members.at(rhs->token_start->raw);
                            if(!member){
                                logger.error(rhs->token_start, "'", structure->decl.identifier, "' has no member '", rhs->token_start->raw, "'.");
                                amufile->semantic_analyzer.failed = 1;
                                return 0;
                            }
                            e->data.structure = member->data.structure;
                        }break;
                    }
                    
                }break;

                case Expression_InitializerList:{
                    logger.log(Verbosity_Debug, "  validating initializer list");
                    Expression* parent = (Expression*)e->node.parent;
                    
                    //figure out what structure we're working with
                    Struct* structure = 0;
                    if(parent->type == Expression_Identifier){
                        // in this case, the type of the initializer list is specified
                        Declaration* d = find_decl(parent->token_start->raw);
                        if(d->type != Declaration_Structure){
                            logger.error(parent->token_start, "an identifier preceding an initializer list must be a typename, found the name of a ", (d->type == Declaration_Function ? "function" : "variable"), " instead.");
                            amufile->semantic_analyzer.failed = 1;
                            return 0;
                        }
                        structure = (Struct*)d;
                        
                    }else{
                        // the initializer list does not have an identifier preceeding it, so we must figure out where we are using it
                        // there are a couple places where this is okay:
                        //   as an argument for another initializer list
                        //   when declaring a non-implicit variable.
                        // there may be more, im not sure yet
                        if(current.variable){
                            Variable* v = current.variable;
                            if(v->data.implicit){
                                logger.error(e->token_start, "expected a typename before initializer list for implicitly typed variable '", v->decl.identifier, "'.");
                                amufile->semantic_analyzer.failed = 1;
                                return 0;
                            }
                            structure = v->data.structure;
                        }else if(stacks.nested.expressions.count > 1){
                            Expression* prev = stacks.nested.expressions[stacks.nested.expressions.count-1];
                            if(prev->type == Expression_FunctionCall){
                                logger.error(e->token_start, "an initializer list used as a function argument must specify its type.");
                                amufile->semantic_analyzer.failed = 1;
                                return 0;
                            }
                        }
                    }
                    
                    //the type of the variable needs to have been validated
                    if(!validate((amuNode*)structure)) return 0;
                    e->data.structure = structure;

                    // we handle this in the same way as function arguments
                    // first, gather all arguments, counting how many are positional, then loop over 
                    // the members of the struct to find matches 
                    amuArena<Expression*> arguments; arguments.init(); defer{arguments.deinit();};
                    u32 n_pos_args = 0;
                    b32 found_named = 0;
                    for_node(e->node.first_child){
                        if(((Expression*)it)->type == Expression_BinaryOpAssignment){
                            found_named = 1;
                        }else if(found_named){
                            logger.error(((Expression*)it)->token_start, "positional arguments cannot come after named arguments.");
                            amufile->semantic_analyzer.failed = 1;
                            return 0;
                        }else{
                            n_pos_args++;
                        }
                        if(!found_named && !validate(it)) return 0;
                        arguments.add(((Expression*)it));
                    }

                    if(arguments.count > structure->members.hashes.count){
                        logger.error(e->token_start, "too many arguments given in initializer list. ", structure->decl.identifier, " only has ", structure->members.hashes.count, (structure->members.hashes.count == 1 ? " member" : " members"), ", but ", arguments.count, (arguments.count == 1 ? " argument" : " arguments"), " are given.");
                        amufile->semantic_analyzer.failed = 1;
                        return 0;
                    }

                    // we remove the argument nodes from the initializer list expression because we will be adding them 
                    // back in the correct order later
                    forI(arguments.count){
                        change_parent(0,&arguments[i]->node);
                    }
                    // TODO(sushi) add type narrowing warnings
                    u32 i = 0;
                    for_node(((amuNode*)structure)->first_child){
                        b32 filled = 0;
                        Variable* member = (Variable*)it;
                        if(i<n_pos_args){
                            Expression* arg = arguments[i];
                            if(!types_match(arg, member)){
                                if(!can_type_convert(&arg->data, &member->data)){
                                    logger.error(arg->token_start, "no known conversion exists from ", get_typename(arg), " to ", get_typename(member));
                                    amufile->semantic_analyzer.failed = 1;
                                    return 0;
                                }
                                Expression* cast = arena.make_expression(amuStr8("cast ", get_typename(arg), " to ", get_typename(member)));
                                cast->type = Expression_CastImplicit;
                                cast->data = member->data;
                                insert_last(&cast->node, &arg->node);
                                insert_last(&e->node, &cast->node);
                            }else{
                                insert_last(&e->node, &arg->node);
                            }
                            filled = 1;
                        }else{
                            forX_reverse(j,arguments.count-n_pos_args){
                                Expression* arg = arguments[arguments.count-1-j];
                                Expression* lhs = (Expression*)arg->node.first_child;
                                Expression* rhs = (Expression*)arg->node.last_child;//(Expression*)validate(arg->node.last_child);
                                if(!rhs || !lhs) continue;
                                if(str8_equal_lazy(lhs->token_start->raw, member->decl.identifier)){

                                    push_variable(member);
                                    rhs = (Expression*)validate((amuNode*)rhs);
                                    if(!rhs) return 0;
                                    pop_variable();

                                    if(!types_match(member, arg)){
                                        Expression* cast = arena.make_expression(amuStr8("cast ", get_typename(rhs), " to ", get_typename(member)));
                                        cast->type = Expression_CastImplicit;
                                        cast->data = member->data;
                                        insert_last(&cast->node, &rhs->node);
                                        insert_last(&e->node, &cast->node);
                                    }else{
                                        insert_last(&e->node, &rhs->node);
                                    }
                                    arguments.remove(arguments.count-1-j);
                                    filled = 1;
                                    break;
                                }
                            }
                        }
                        if(!filled){
                            // when a variable is not filled, we just add a placeholder that later stages deal wtih later.
                            Expression* placeholder = arena.make_expression(STR8("placeholder"));
                            placeholder->type = Expression_Literal;
                            placeholder->data.pointer_depth = -1;
                            insert_last(&e->node, &placeholder->node);
                        }
                        i++;
                    }
                }break;
            }
            Assert(e->data.structure, "an expression is required to have a structure assigned to it!");            
            return &e->node;
        }break;
    }
    */
    return 0;
}

void SemanticAnalyzer::start(){DPZoneScoped;
    amuLogger& logger = amufile->logger;
    Stopwatch validate_time = start_stopwatch();
    logger.log(Verbosity_Stages, "Validating...");
    SetThreadName("Validating ", amufile->file->name);

    //wait for imported files to finish validation
    logger.log(Verbosity_StageParts, "Waiting for imported files to finish validation.");
    for(amuFile* sf : amufile->preprocessor.imported_files){
        while(sf->stage < FileStage_Validator){
            condition_variable_wait(&sf->cv.validate);
            if(sf->stage == FileStage_ERROR) return;
        }
    }
    
    stacks.known_declarations_scope_begin_offsets.add(0);

    validate((amuNode*)amufile->module);

    amufile->stage = FileStage_Validator;
    condition_variable_notify_all(&amufile->cv.validate);

    logger.log(Verbosity_Stages, "Validating finished in ", peek_stopwatch(validate_time), " ms.");
}