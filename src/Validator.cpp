/*

    TODO(sushi) because the AST will be able to be modified directly in the language
                we have to do a lot more validation on the tree than I am currently doing
                we have to make sure the entire structure of the tree is correct and 
                not just assume that it is correct because it is coming from the Parser!

*/

namespace amu {
namespace validator {

Validator* 
create() {
    Validator* out = pool::add(compiler::instance.storage.validators);
    return out;
}

void
destroy(Validator* v) {
    pool::remove(compiler::instance.storage.validators, v);
}

namespace internal {

#define announce_stage(n) messenger::dispatch(message::make_debug(message::verbosity::debug, String("validating "), String(__func__), String(": "), (String)to_string((TNode*)n, true)))

Array<LabelTable*> table_stack = array::init<LabelTable*>();
LabelTable* current_table = 0;

void push_table(LabelTable* lt) {
    array::push(table_stack, current_table);
    current_table = lt;
}

void pop_table() {
    current_table = array::pop(table_stack);
}

b32 label(TNode* node);
b32 expr(Expression* e);

b32
statement(Statement* s) { announce_stage(s);
    switch(s->kind) {
        case statement::label: return label(s->node.first_child);
        case statement::expression: return expr((Expression*)s->node.first_child);
    }
    return false;
}

b32
block(BlockExpression* e) { announce_stage(e);
    push_table(&e->table);
    for(TNode* n = e->node.first_child; n; n = n->next) {
        if(!statement((Statement*)n)) return false;
    }
    pop_table();
    return true;
}

b32 
func_ret(TNode* n) { announce_stage(n);
    if(n->kind == node::tuple) {

    }else if(n->kind == node::expression) {
        auto e = (Expression*)n;
        if(e->kind != expression::typeref) {
            diagnostic::validator::
                func_ret_expected_typeref(e->node.start);
            return false;
        }
        return true;
    }

    return false;
}

b32 
func_arg_tuple(Tuple* t) { announce_stage(t);
    if(!t->node.child_count) return true;
    for(TNode* n = t->node.first_child; n; n = n->next) {
        if(n->kind == node::expression) {
            if(!expr((Expression*)n)) return false;
        } else {
            if(!label(n)) return false;
        }
    }
    return true;
}

b32 
call(CallExpression* e) {
    FunctionType* f = e->callee->type;
    if(e->arguments->node.child_count > f->parameters->child_count) {
        diagnostic::validator::
            too_many_arguments(e->node.start, e->callee->label->node.start->raw);
        return false;
    } 

    if(f->parameters->child_count > e->arguments->node.child_count) {
        diagnostic::validator::
            not_enough_arguments(e->node.start, e->callee->label->node.start->raw);
        return false;
    }

    TNode* func_arg = f->parameters->first_child;
    TNode* call_arg = e->arguments->node.first_child ;
    while(func_arg && call_arg) {
        if(!type::can_coerce(type::resolve(func_arg), type::resolve(call_arg))) {
            diagnostic::validator::
                mismatch_argument_type(call_arg->start, 
                    type::resolve(call_arg), 
                    type::resolve(func_arg), 
                    label::resolve(func_arg)->node.start->raw, 
                    e->callee->label->node.start->raw);
            return false;
        }
        func_arg = func_arg->next;
        call_arg = call_arg->next;
    }
    return true;
} 

b32
typeref(Expression* e) { announce_stage(e);
    switch(e->type->kind) {
        case type::kind::scalar: {
            return true;
        } break;
        case type::kind::function: {
            auto type = (FunctionType*)e->type;
            if(!func_arg_tuple((Tuple*)type->parameters) ||
               !func_ret(type->returns) || 
               !block((BlockExpression*)e->node.last_child)) return false;
            return true;
        } break;
        case type::kind::structured: {
            return true;
        } break;
        case type::kind::pointer: {
            return true;
        } break;
    }
    return false;
}

b32 
access(Expression* e, Expression* lhs, Type* lhs_type, Expression* rhs) {
    switch(lhs_type->kind) {
        case type::kind::scalar: {
            diagnostic::validator::
                cannot_access_members_scalar_type(lhs->node.start);
            return false;
        } break;
        case type::kind::array: {
            switch(rhs->node.start->hash) {
                case string::static_hash("ptr"):
                case string::static_hash("count"): return true;
                default: {
                    diagnostic::validator::
                        array_types_dont_have_member(rhs->node.start, rhs->node.start->raw);
                    return false;
                }
            }
        } break;
        case type::kind::function: {
            diagnostic::validator::
                cannot_access_members_of_function_type(lhs->node.start);
            return false;
        } break;
        case type::kind::pointer: {
            auto ptype = (PointerType*)lhs_type;
            if(ptype->type->kind == type::kind::pointer) {
                diagnostic::validator::
                    too_many_levels_of_indirection_for_access(lhs->node.start);
            }
            return access(e, lhs, ptype->type, rhs);
        } break;
        case type::kind::tuple: {
            auto ttype = (TupleType*)lhs_type;
            NotImplemented;
        } break;
        case type::kind::structured: {
            auto stype = (StructuredType*)lhs_type;
            auto [idx, found] = map::find(stype->structure->table.map, rhs->node.start->raw);
            if(!found) {
                diagnostic::validator::
                    unknown_member(rhs->node.start, (Type*)stype, rhs->node.start->raw);
                return false;
            }
            Label* l = array::read(stype->structure->table.map.values, idx);
            e->type = type::resolve((TNode*)l->entity);
            return true;
        } break;
    }
    return false;
}

b32 
expr(Expression* e) { announce_stage(e);
    switch(e->kind) {
        case expression::unary_comptime: return expr((Expression*)e->node.first_child);
        case expression::unary_assignment: return expr((Expression*)e->node.first_child);
        case expression::unary_reference: {
            
        } break;
        case expression::call: {
            return call((CallExpression*)e);
        } break;
        case expression::typeref: return typeref(e);
        case expression::binary_assignment: {
            auto lhs = (Expression*)e->node.first_child;
            auto rhs = (Expression*)e->node.last_child;
            if(!expr(lhs)) return false;
            if(!expr(rhs)) return false;

            // TODO(sushi) type coersion
            if(!type::can_coerce(lhs->type, rhs->type)) {
                diagnostic::validator::
                    cannot_implict_coerce(lhs->node.start, rhs->type, lhs->type);
                return false;
            }

            return true;
        } break;

        case expression::binary_access: {
            auto lhs = (Expression*)e->node.first_child;
            auto rhs = (Expression*)e->node.last_child;
            if(!expr(lhs)) return false;
            Type* lhs_type = type::resolve((TNode*)lhs);
            if(!lhs_type) {
                 diagnostic::validator::
                    invalid_type_lhs_access(lhs->node.start);
                return false;
            }

            return access(e, lhs, lhs_type, rhs);
        } break;

        case expression::binary_plus: {
            auto lhs = (Expression*)e->node.first_child;
            auto rhs = (Expression*)e->node.last_child;
            if(!expr(lhs)) return false;
            if(!expr(rhs)) return false;

            if(lhs->type == rhs->type) { 
                if(!type::is_scalar(lhs->type)) {
                    // NOTE(sushi) temp error until traits are implemented
                    Message m = message::init(String("cannot perform addition between non-scalar types yet!"));
                    m.kind = message::error;
                    m = message::attach_sender(e->node.start, m);
                    messenger::dispatch(m);
                    return false;
                }
                e->type = lhs->type;
            } else if(type::is_scalar(lhs->type) && type::is_scalar(rhs->type)) {
                // take the larger of the two, and prefer float > signed > unsigned
                auto l = (ScalarType*)lhs->type, 
                     r = (ScalarType*)rhs->type;
                
                Expression* cast = expression::create();
                cast->kind = expression::cast;
                b32 take_left = l->kind > r->kind;
                cast->type = (take_left? l : r);
                node::insert_above((take_left? (TNode*)rhs : (TNode*)lhs), (TNode*)cast);
                e->type = cast->type;
            } else {
                diagnostic::validator::
                    cant_find_binop_trait(e->node.start, "Add", lhs->type, rhs->type);
                return false;
            }
            return true;

            // TODO(sushi) Add trait implemented here
        } break;
        case expression::identifier:
        case expression::literal: {
            return true;
        } break;
        
    }

    return false;
}

b32
label(TNode* node) { announce_stage(node);
    return expr((Expression*)node->last_child);
}

b32
module(TNode* node) { announce_stage(node);
    Module* m = (Module*)node;
    push_table(&m->table);
    for(TNode* n = node->first_child; n; n = n->next) {
        if(!label(n)) return false;
    }  
    pop_table();
    return true;
}

} // namespace internal

void
execute(Code* code) {
    // TODO(sushi) this will be incorrect when Code becomes more general
    if(!internal::module((TNode*)code->source->module)) {
        util::println("validation failed");
    }
}

} // namespace validator
} // namespace amu