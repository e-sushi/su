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

//checks the known stack to see if there are any variable name conflicts in the same scope
b32 Validator::check_shadowing(Declaration* d){DPZoneScoped;
    suLogger& logger = sufile->logger;
    forI(stacks.known_declarations.count - stacks.known_declarations_scope_begin_offsets[stacks.known_declarations_scope_begin_offsets.count-1]){
        Declaration* dk = stacks.known_declarations[stacks.known_declarations.count - 1 - i];
        if(dk==d) continue;
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

b32 Validator::can_type_convert(TypedValue* from, TypedValue* to){
    if(from->structure==to->structure) return true;
    return from->structure->conversions.has(to->structure->decl.identifier);
}

//NOTE(sushi) this is special because locally defined functions that overload something in a lesser scope
//            need to remove themselves from the overload tree that connects them
void Validator::pop_known_decls(){
    u32 n = stacks.known_declarations.count - stacks.known_declarations_scope_begin_offsets.pop();
    forI(n){
        Declaration* d = stacks.known_declarations.pop();
        if(d->type == Declaration_Function){
            Function* f = FunctionFromDeclaration(d);
        }
    }
}

suNode* Validator::validate(suNode* node){DPZoneScoped;
    suLogger& logger = sufile->logger;
    //TODO(sushi) we could possibly just store a validated flag on suNode 
    if(match_any(node->type, NodeType_Variable, NodeType_Structure, NodeType_Function) &&
       DeclarationFromNode(node)->validated){
        //early out if this declaration has already been validated
        
        return node;
    }
    if(globals.verbosity == Verbosity_Debug){
        switch(node->type){
            case NodeType_Expression: logger.log(Verbosity_Debug, "validating expression '", ExpressionFromNode(node)->token_start->raw, "' of type ", ExTypeStrings[ExpressionFromNode(node)->type]); break;
            case NodeType_Function:   logger.log(Verbosity_Debug, "validating function ", FunctionFromNode(node)->decl.identifier); break;
            case NodeType_Scope:      logger.log(Verbosity_Debug, "validating scope"); break;
            case NodeType_Statement:  logger.log(Verbosity_Debug, "validating statement '", StatementFromNode(node)->token_start->raw, "'"); break;
            case NodeType_Structure:  logger.log(Verbosity_Debug, "validating struct '", StructFromNode(node)->decl.identifier, "'"); break;
            case NodeType_Variable:   logger.log(Verbosity_Debug, "validating variable '", VariableFromNode(node)->decl.identifier, "'"); break;

        }
    }
    switch(node->type){
        case NodeType_Variable:{
            Variable* v = VariableFromNode(node);

            push_variable(v);
            defer{pop_variable();};

            if(!check_shadowing(&v->decl)) return 0;
            stacks.known_declarations.add(&v->decl);
            //if this variable's structure is null, then it is a user defined struct that we must look for
            if(!v->data.structure){
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
                v->data.structure = StructFromDeclaration(d);
            }
            
            if(v->data.implicit){
                Expression* e = ExpressionFromNode(validate(v->decl.node.first_child));
                if(!e) return 0;
                v->data = e->data;
            }else if(v->decl.node.child_count){
                Expression* e = ExpressionFromNode(validate(v->decl.node.first_child));
                if(!e) return 0;
                if(v->data.structure != e->data.structure){
                    if(!e->data.structure->conversions.at(v->data.structure->decl.identifier)){
                        logger.error(v->decl.token_start, "no known conversion from ", get_typename(e), " to ", get_typename(v));
                        logger.note(v->decl.token_start, ErrorFormat("(Not Implemented)"), "for implicit conversion define implicit(name:", get_typename(e), ") : ", get_typename(v));
                        return 0;
                    }
                    //if types are compatible just inject a conversion node 
                    //we dont inject the actual conversion process because that would require us to copy nodes
                    //instead we just put a cast node here indicating which type to convert to and its handled later
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
                        return 0;
                    }

                    forI(stacks.nested.structs.count){
                        if(stacks.nested.structs[i]==v->data.structure){
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
                    if(!validate(&v->data.structure->decl.node)) return 0;
                }
                current.structure->size += v->data.structure->size;
            }
            v->decl.validated = 1;
            return &v->decl.node;
        }break;

        case NodeType_Function:{
            Function* f = FunctionFromNode(node);
            push_function(f);
            defer{pop_function();};

            stacks.known_declarations.add(&f->decl);
            stacks.known_declarations_scope_begin_offsets.add(stacks.known_declarations.count);
            
            Declaration* d = DeclarationFromNode(sufile->validator.functions.at(f->decl.identifier));
            if(!d){
                //this is the first occurance of this function that we know of so we add it to the function map
                d = DeclarationFromNode(sufile->validator.functions.atIdx(sufile->validator.functions.add(f->decl.identifier, &arena.make_function(suStr8("funcgroup of ", f->decl.identifier))->decl.node)));
                d->identifier = f->decl.identifier;
            }        
            
            
            Function* fg = FunctionFromDeclaration(d);
            fg->overloads.add(f);

            suNode* def = 0;
            suNode* otreecur = &d->node;
            b32 found_nonpos = 0; //set when we find an argument that has a default value
            for_node(f->decl.node.first_child){
                if(it->type == NodeType_Scope) { def = it; continue; }
                if(!validate(it)) return 0;
                //search overload node children for variables that are the same type as this one
                //and just walk down it if it exists, otherwise make a new node representing this type
                Variable* arg = VariableFromNode(it);
                if(it->child_count){
                    //this is an arg with a default value 
                    found_nonpos = 1;
                }else if(found_nonpos){
                    logger.error(arg->decl.token_start, "positional arguments cannot follow defaulted arguments.");
                    return 0;
                }

                b32 found = 0;
                for_nodeX(it2, otreecur->first_child){
                    Arg* a = ArgFromNode(it2);
                    if(a->val.structure == arg->data.structure){
                        otreecur = it2;
                        a->vars.add(arg);
                        found = 1;
                        break;
                    }
                }
                if(!found){
                    Arg* nua = arena.make_arg();
                    nua->node.debug = get_typename(arg);
                    if(found_nonpos) nua->defaulted = 1;
                    nua->val = arg->data;
                    nua->vars.add(arg);
                    insert_last(otreecur, &nua->node);
                    otreecur = &nua->node;
                }
                if(it->next->type == NodeType_Scope){
                    //we've reached the end of arguments and must check that this func does not conflict with any others
                    if(ArgFromNode(otreecur)->f){
                        logger.error(f->decl.token_start, "function overloads must differ by type.");
                        logger.note(ArgFromNode(otreecur)->f->decl.token_start, "see conflicting function.");
                        f->decl.validated = 1; //if we dont set this here its possible to infinitely loop back to this point and repeatedly print this error message
                        return 0;
                    }else ArgFromNode(otreecur)->f = f;
                }
            }

            //TODO(sushi) setup a system for testing if this overload makes calls to any other overload ambiguous
            //            or if any other function makes calls to this one ambiguous
            //            currently ambiguity is found when an ambiguous call is made, like with msvc's cl
            //            but this isnt ideal since it only errors when a call to an ambiguous func is made

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
            Struct* s = StructFromNode(node);
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
            Scope* s = ScopeFromNode(node);
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
            Statement* s = StatementFromNode(node);
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
                    }
                }break;
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

                case Expression_FunctionCall:{
                    //validate call id
                    Declaration* d = DeclarationFromNode(sufile->validator.functions.at(e->token_start->raw));
                    //this is kind of ugly                    
                    if(!d){
                        //its possible this function has not been validated yet so we look for it again in our known decls list
                        //and if its a function run validate on it 
                        d = find_decl(e->token_start->raw);
                        if(d){
                            if(d->type == Declaration_Function){
                                if(!validate(&d->node)) return 0;
                                d = DeclarationFromNode(sufile->validator.functions.at(e->token_start->raw));

                            }else{
                                logger.error(e->token_start, "attempt to call '", e->token_start->raw, "', but it is not a function.");
                                logger.note(e->token_start, e->token_start->raw, " is a ", (d->type == Declaration_Structure ? "structure." : "variable."));
                                return 0;    
                            }
                        }else{
                            logger.error(e->token_start, "attempt to call unknown identifier '", e->token_start->raw, "'");
                            return 0;
                        }
                    }
                    Function* fg = FunctionFromDeclaration(d);
                    //validate arguments and match them to overloads
                    enum{
                        Matched               = 1 << 0,
                        MatchedNameButNotType = 1 << 1,
                        CanConvert            = 1 << 2,
                    };
                    suNode* otreecur = &d->node;
                    u32 remaining_args = e->node.child_count;
                    b32 named = 0; //set when a named call arg is found
                    b32 second_pass = 0; //set when we are performing a second pass on a function arg to check for conversions
                    u32 second_pass_matches = 0; //incremented for each match we find on a second pass
                    //function arguments that candidates for being chosen as our next branch
                    suArena<suNode*> farg_candidates; farg_candidates.init();
                    //call arguments that are candidates for being matched against function arguments
                    suArena<suNode*> carg_candidates; carg_candidates.init();
                    //nodes that matched the name, but not type of an argument and are able to convert to its type
                    suArena<suNode*> mncc_candidates; mncc_candidates.init();
                    //positional nodes that can convert to the current positional argument 
                    suArena<suNode*> cc_candidates;   cc_candidates.init();   
                    
                    
                    for_node(otreecur->first_child){
                        farg_candidates.add(it);
                    }
                    b32 found_named = 0;
                    u32 n_pos_args = 0;
                    for_node(e->node.first_child){
                        if(ExpressionFromNode(it)->type == Expression_BinaryOpAssignment){
                            found_named = 1;
                        }else if(found_named){
                            logger.error(ExpressionFromNode(it)->token_start, "positional arguments cannot come after named arguments.");
                            return 0;
                        }else{
                            n_pos_args++;
                        }
                        carg_candidates.add(it);
                    }



                    //initially get positional args out of the way since they are simpler to handle
                    //NOTE(sushi) this does not choose a branch to go down, it only removes branches we cant go down
                    b32 matching_positional_args = 1;
                    while(matching_positional_args){
                        forX(ci, carg_candidates.count){
                            suNode* cn = carg_candidates[ci];
                            if(ExpressionFromNode(cn)->type == Expression_BinaryOpAssignment){
                                matching_positional_args = 0;
                                break;
                            }
                            Expression* carg = ExpressionFromNode(validate(cn));
                            b32 matched = 0;
                            forX_reverse(fi, farg_candidates.count){
                                suNode* fn = farg_candidates[fi];
                                Arg*  farg = ArgFromNode(fn);
                                u32 counter = ci;
                                if(!can_type_convert(&carg->data, &farg->vars[0]->data)){
                                    //if we cant convert to this type, then just remove it 
                                    farg_candidates.remove(fi);
                                }
                            }
                            if(matched){
                                //if we matched just remove this argument and continue
                                carg_candidates.remove(ci);
                                ci--;
                            }else if(!farg_candidates.count){
                                //it is possible we didnt match anything any we didnt find any branch we could convert to either
                                logger.error(carg->token_start, "unable to match argument ")
                            }
                        }
                    }
                    

                    b32 walking = 1;  
                    b32 matched = 0;
                    u32 matched_index = 0;
                    while(walking){
                        forX_reverse(fi, farg_candidates.count){
                            //iterate backwards so we dont mess up indexing when removing an element
                            suNode* fn = farg_candidates[fi];
                            Arg* farg = ArgFromNode(fn);
                            forX_reverse(ci, carg_candidates.count){
                                suNode* cn = carg_candidates[ci];
                                Expression* carg = ExpressionFromNode(cn);
                                if(carg->type == Expression_BinaryOpAssignment){
                                    named = 1;
                                    if(second_pass && HasAllFlags(cn->flags, MatchedNameButNotType | CanConvert)){
                                        second_pass_matches++;
                                        break;
                                    }
                                    Expression* lhs = ExpressionFromNode(carg->node.first_child);
                                    if(lhs->type != Expression_Identifier){
                                        logger.error(lhs->token_start, "expected an identifier as lhs of named argument.");
                                        return 0;
                                    }
                                    Expression* rhs = ExpressionFromNode(validate(carg->node.last_child));
                                    if(!rhs) return 0;
                                    //find the name. if we dont find it, just move on and try again later
                                    forI(farg->vars.count){
                                        Variable* v = farg->vars[i];
                                        if(str8_equal_lazy(v->decl.identifier, lhs->token_start->raw)){
                                            if(types_match(v,rhs)){
                                                matched = 1;
                                                matched_index = fi;
                                                carg_candidates.remove(ci);
                                                AddFlag(cn->flags, Matched);
                                            }else{
                                                AddFlag(cn->flags, MatchedNameButNotType);
                                                if(can_type_convert(&rhs->data, &v->data)){
                                                    AddFlag(cn->flags, CanConvert);
                                                }
                                            }
                                            break;
                                        }
                                    }
                                }else{
                                    if(named){
                                        logger.error(carg->token_start, "positional arguments cannot follow named arguments.");
                                        return 0;
                                    }
                                    carg = ExpressionFromNode(validate(cn));
                                    if(!carg) return 0;
                                    if(second_pass){
                                        if(HasFlag(cn->flags, CanConvert)){
                                            second_pass_matches++;
                                        }else{
                                            farg_candidates.remove(fi);
                                        }
                                        break;
                                    }
                                    if(types_match(carg, farg->vars[0])){
                                        matched = 1;
                                        matched_index = fi;
                                        carg_candidates.remove(ci);
                                        AddFlag(cn->flags, Matched);
                                    }else if(can_type_convert(&carg->data, &farg->vars[0]->data)){
                                        AddFlag(cn->flags, CanConvert);
                                    }else{
                                        //its not possible for it to be this branch because a positional argument does not match, so remove it from the list
                                        farg_candidates.remove(fi);
                                    }
                                    //since this is a positional arg we must skip the rest
                                    break;
                                }
                            }
                            if(matched) break;
                        }

                        if(matched){
                            otreecur = farg_candidates[matched_index];
                            farg_candidates.clear();
                            for_node(otreecur->first_child){
                                farg_candidates.add(it);
                            }
                            matched = 0;
                        }else if(second_pass){
                            if(second_pass_matches == 1){

                            }else if(named && second_pass_matches != 1){
                                //we are checking named arguments in a second pass that had no matches
                                //so we must trim the branches by checking each one for each name
                                Expression* invalidating_exp = 0; 
                                forX(ci, carg_candidates.count){
                                    suNode* cn = carg_candidates[ci];
                                    Expression* carg = ExpressionFromNode(cn);
                                    Expression* lhs = ExpressionFromNode(carg->node.first_child);
                                    Expression* rhs = ExpressionFromNode(carg->node.last_child);
                                    
                                    auto check_branch = [&](suNode* node, auto&& check_branch) -> b32 {
                                        Arg* farg = ArgFromNode(node);
                                        forI(farg->vars.count){
                                            Variable* v = farg->vars[i];
                                            if(str8_equal_lazy(v->decl.identifier, lhs->token_start->raw)){
                                                //return true here even if types dont match up
                                                return true;
                                            }
                                        }
                                        for_node(node->first_child){
                                            if(check_branch(it, check_branch)) return true;
                                        }
                                        return false;
                                    };

                                    forX_reverse(fi, farg_candidates.count){
                                        suNode* fn = farg_candidates[fi];
                                        if(!check_branch(fn, check_branch)) farg_candidates.remove(fi);
                                        if(!farg_candidates.count) invalidating_exp = lhs;
                                    }
                                }

                                if(!farg_candidates.count){
                                    //if we have no more candidates then the call is invalid
                                    //TODO(sushi) better erroring here
                                    logger.error(invalidating_exp->token_start, "no overload of function '", fg->decl.identifier, "' matches given named arg '", invalidating_exp->token_start->raw, "'. TODO(sushi) better erroring");
                                    return 0;
                                }
                                //if we still have more than one argument we need to check if exactly one of them is a defaulted arg
                                //if so we can just go down that branch, otherwise the call is ambiguous
                                if(farg_candidates.count > 1){
                                    Arg* defaulted = 0;
                                    forI(farg_candidates.count){
                                        Arg* a = ArgFromNode(farg_candidates[i]);
                                        if(a->defaulted){
                                            if(defaulted){
                                                //if we find more than one default then the call is ambiguous
                                                logger.error(e->token_start, "call to '", e->token_start->raw, "' is ambiguous. TODO(sushi) better erroring here");
                                                return 0;
                                            }else{
                                                defaulted = a;
                                            }
                                        }
                                    }
                                    if(!defaulted){
                                        //in this case the user must be missing a positional argument
                                        logger.error(e->token_start, "missing positional argument in function call.");
                                        return 0;
                                    }else{
                                        //we can safely choose to go down this branch
                                        farg_candidates.clear();
                                        for_node(defaulted->node.first_child){
                                            farg_candidates.add(it);
                                        }                                        
                                        second_pass = 0;
                                    }
                                }
                            }else{
                                logger.error(e->token_start, "invalid function call.");
                                return 0;
                            }
                        }else{
                            second_pass = 1;
                        }
                    }

                    // u64 argpos = 0; //keeps track of where in positional arguments we are 
                    // b32 named = 0; //set true when we come across a named argument
                    // //the possible functions are built off of this node
                    // suNode base;
                    // b32 default_found = 0;
                    // u32 args_consumed = 0;
                    // //walk the overload tree and match func arguments to call arguments as we go
                    // //NOTE(sushi) this is easily the most complex thing in amu atm (2022/08/02) and it could use some clean up
                    // b32 walking = 1;
                    // enum{
                    //     Matched = 1 << 0,
                    //     DidntMatchTypeButCanConvert = 1 << 1,
                    // };
                    // b32 validating_positional_args = 1;
                    // suNode* candidate_for_conversion;

                    // b32 do_conversion = 0;
                    // while(walking){
                    //     for_nodeX(fn, otreecur->first_child){
                    //         Arg* a = ArgFromNode(fn);
                    //         b32 match = 0;
                    //         for_nodeX(cn, e->node.first_child){
                    //             //if we have already matched this node, skip it
                    //             if(HasFlag(cn->flags, Matched)) continue;
                    //             Expression* arg = ExpressionFromNode(cn);
                    //             if(arg->type == Expression_BinaryOpAssignment){
                    //                 named = 1;
                    //                 Expression* lhs = ExpressionFromNode(arg->node.first_child);
                    //                 if(lhs->type != Expression_Identifier){
                    //                     logger.error(lhs->token_start, "expected an identifier as lhs of named argument.");
                    //                     return 0;
                    //                 }
                    //                 Expression* rhs = ExpressionFromNode(validate(arg->node.last_child));
                    //                 //match names
                    //                 forI(a->vars.count){
                    //                     Variable* v = a->vars[i];
                    //                     if(str8_equal_lazy(v->decl.identifier, lhs->token_start->raw)){
                    //                         match = 1;
                    //                         if(!types_match(rhs,v)){
                    //                             if(!can_type_convert(&rhs->data, &v->data)){
                    //                                 logger.error(lhs->token_start, "no known conversion from ", get_typename(v), " to ", get_typename(lhs));
                    //                                 return 0;
                    //                             }
                    //                             Expression* cast = arena.make_expression(STR8("cast"));
                    //                             cast->type = Expression_CastImplicit;
                    //                             cast->token_start = v->decl.token_start;
                    //                             cast->token_end = v->decl.token_end;
                    //                             cast->data = v->data;
                    //                             insert_last(rhs->node.parent, &cast->node);
                    //                             change_parent(&cast->node, &e->node);
                    //                         }
                    //                         break;
                    //                     }
                    //                 }
                    //             }else{
                    //                 //NOTE(sushi) we dont do this initially since its possible this was a named argument
                    //                 arg = ExpressionFromNode(validate(cn));
                    //                 if(!arg) return 0;
                    //                 if(named){
                    //                     logger.error(arg->token_start, "positional arguments cannot follow named arguments.");
                    //                     return 0;
                    //                 }

                    //                 if(a->val.structure != arg->data.structure){
                    //                     if(do_conversion && can_type_convert(&a->val, &arg->data)){
                    //                         Expression* cast = arena.make_expression(STR8("cast"));
                    //                         cast->type = Expression_CastImplicit;
                    //                         cast->token_start = arg->token_start;
                    //                         cast->token_end = arg->token_end;
                    //                         cast->data = v->data;
                    //                         insert_last(rhs->node.parent, &cast->node);
                    //                         change_parent(&cast->node, &e->node);
                    //                     }
                    //                 }

                    //             }
                    //         }
                    //         if(!match){
                    //             if(a->defaulted){
                    //                 //if there was no match, we must check a few things
                    //                 //first we must check if there are several possible paths to take from this point 
                    //                 //if so, then we know the call is ambiguous
                    //                 if(fn->child_count > 1){
                    //                     //if there is more than one child and we are dealing with a named argument
                    //                     //then we must check if this named argument exists anywhere in the tree

                    //                     logger.error(e->token_start, "call to ", e->token_start->raw, " is ambiguous.");
                    //                     return 0;
                    //                 }
                    //             }else{
                    //                 //otherwise the user missed a positional argument
                    //                 if(!do_conversion){
                    //                     //if we didnt match any positional arguments on the first pass
                    //                     //we go over the possible arguments again, but this time allowing conversions
                    //                     do_conversion = true;
                    //                     continue;
                    //                 } 
                    //                 logger.error(e->token_start, "missing positional argument in call to ", e->token_start->raw);
                    //                 return 0;
                    //             }
                    //         }else{
                    //             otreecur = fn;
                    //             break;
                    //         }
                    //     }
                    //     if(!otreecur->first_child) break;
                    // }
                    // //after all of this, reset the function call's expressions flags to 0
                    // for_node(e->node.first_child){
                    //     it->flags = 0;
                    // }

                    // for_nodeX(cn, e->node.first_child){
                    //     Expression* callarg = ExpressionFromNode(cn);
                    //     if(callarg->type == Expression_BinaryOpAssignment){
                    //         named = 1;
                    //         //NOTE(sushi) we do not call validate on this node because it will be an identifier unknown to find_decl
                    //         Expression* lhs = ExpressionFromNode(callarg->node.first_child);
                    //         if(lhs->type != Expression_Identifier){
                    //             logger.error(lhs->token_start, "expected an identifier as lhs of named argument.");
                    //             return 0;
                    //         }
                    //         Expression* rhs = ExpressionFromNode(validate(callarg->node.last_child));

                    //     }
                    // }
                }break;

                //initial binary op cases that lead into another set of cases for doing specific work with them
                //this kind of sucks, but I don't want to duplicate validating lhs and rhs throughout every single case
                case Expression_BinaryOpBitOR:
                case Expression_BinaryOpBitXOR:
                case Expression_BinaryOpBitShiftLeft:
                case Expression_BinaryOpBitShiftRight:
                case Expression_BinaryOpBitAND:
                case Expression_BinaryOpPlus:
                case Expression_BinaryOpMinus:
                case Expression_BinaryOpMultiply:
                case Expression_BinaryOpDivision:
                case Expression_BinaryOpAND:
                case Expression_BinaryOpOR:
                case Expression_BinaryOpModulo:
                case Expression_BinaryOpLessThan:
                case Expression_BinaryOpGreaterThan:
                case Expression_BinaryOpLessThanOrEqual:
                case Expression_BinaryOpGreaterThanOrEqual:
                case Expression_BinaryOpEqual:
                case Expression_BinaryOpNotEqual:
                case Expression_BinaryOpAs:
                case Expression_BinaryOpMemberAccess:
                case Expression_BinaryOpAssignment:
                Expression* lhs = ExpressionFromNode(validate(e->node.first_child));
                if(!lhs) return 0;
                Expression* rhs = ExpressionFromNode(validate(e->node.last_child));
                if(!rhs) return 0;
                //temporary erroring until we implement operator overloading for structs
                if(e->type != Expression_BinaryOpAssignment && (!(lhs->data.structure->type || rhs->data.structure->type))){
                    logger.error(lhs->token_start, "operator overloading for user-defined structures is not supported yet.");
                    logger.note(lhs->token_start, "when trying to use operator ", ExTypeStrings[e->type], " between ", get_typename(lhs), " and ", get_typename(rhs));
                    return 0;
                }
                switch(e->type){    
                    //NOTE(sushi) performing bitwise operations on floats without reinterpretting is allowed, but they are always coerced to int
                    //bit binary ops are special because they will always return an integer
                    case Expression_BinaryOpBitOR:
                    case Expression_BinaryOpBitXOR:
                    case Expression_BinaryOpBitShiftLeft:
                    case Expression_BinaryOpBitShiftRight:
                    case Expression_BinaryOpBitAND:{
                    }break;
                    
                    case Expression_BinaryOpPlus:
                    case Expression_BinaryOpMinus:
                    case Expression_BinaryOpMultiply:
                    case Expression_BinaryOpDivision:
                    case Expression_BinaryOpAND:
                    case Expression_BinaryOpOR:
                    case Expression_BinaryOpModulo:{
                    }break;

                    case Expression_BinaryOpLessThan:
                    case Expression_BinaryOpGreaterThan:
                    case Expression_BinaryOpLessThanOrEqual:
                    case Expression_BinaryOpGreaterThanOrEqual:
                    case Expression_BinaryOpEqual:
                    case Expression_BinaryOpNotEqual:{

                    }break;

                    case Expression_BinaryOpAs:
                    case Expression_BinaryOpMemberAccess:{

                    }break;

                    case Expression_BinaryOpAssignment:{

                        // if(lhs->data.type != rhs->data.type || 
                        // lhs->data.type == Token_Struct && rhs->data.type == Token_Struct &&
                        // lhs->data.struct_type != rhs->data.struct_type){
                        //     if(!rhs->data.struct_type->conversions.has(get_typename(lhs))){
                        //         logger.error(lhs->token_start, "no known conversion from ", get_typename(rhs), " to ", get_typename(lhs));
                        //         logger.note(lhs->token_start, ErrorFormat("(Not Implemented)"), "for implicit conversion define implicit(name:", get_typename(rhs), ") : ", get_typename(lhs));
                        //         return 0;
                        //     }
                        // }


                    }break;
                }
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

    //wait for imported files to finish validation
    logger.log(Verbosity_StageParts, "Waiting for imported files to finish validation.");
    for(suFile* sf : sufile->preprocessor.imported_files){
        while(sf->stage < FileStage_Validator){
            sf->cv.validate.wait();
        }
    }
    
    stacks.known_declarations_scope_begin_offsets.add(0);

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
                //copy function map as well
                //this kind of sucks and i want to make it better eventually
                forI(sufileex->validator.functions.data.count){
                    sufile->validator.functions.add(DeclarationFromNode(sufileex->validator.functions.atIdx(i))->identifier, sufileex->validator.functions.atIdx(i));
                }
            }
        }
    }



    for_node(sufile->parser.base.first_child){
        //add global declarations to known declarations
        if(match_any(it->type, NodeType_Function, NodeType_Variable, NodeType_Structure)){
            //NOTE(sushi) we dont return on fail here because we still want to validate other things 
            //check_shadowing(DeclarationFromNode(it));
            stacks.known_declarations.add(DeclarationFromNode(it));
        }
    }


    //stacks.known_declarations_scope_begin_offsets.add(stacks.known_declarations.count);

    for_node(sufile->parser.base.first_child){
        //NOTE(sushi) we dont return on 0 here so we can gather as many errors as we can 
        validate(it);
    }
    
    sufile->stage = FileStage_Validator;
    sufile->cv.validate.notify_all();

    logger.log(Verbosity_Stages, "Validating finished in ", peek_stopwatch(validate_time), " ms.");
}