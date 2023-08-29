/*

    TODO(sushi) because the AST will be able to be modified directly in the language
                we have to do a lot more validation on the tree than I am currently doing
                we have to make sure the entire structure of the tree is correct and 
                not just assume that it is correct because it is coming from the Parser!

*/

namespace amu {
namespace sema {

Sema* 
create() {
    Sema* out = pool::add(compiler::instance.storage.semas);
    return out;
}

void
destroy(Sema* v) {
    pool::remove(compiler::instance.storage.semas, v);
}

namespace internal {

u64 layers = 0;

#define announce_stage(n) \
    DString indentation = dstring::init(); \
    forI(layers) dstring::append(indentation, " "); \
    messenger::dispatch(message::make_debug(message::verbosity::debug, String(indentation), String("validating "), String(__func__), String(": "), (String)to_string((TNode*)n, true))); \
    layers++; \
    defer { layers--; }

// Array<LabelTable*> table_stack = array::init<LabelTable*>();
// LabelTable* current_table = 0;

// void push_table(LabelTable* lt) {
//     array::push(table_stack, current_table);
//     current_table = lt;
// }

// void pop_table() {
//     current_table = array::pop(table_stack);
// }

b32 label(Code* code, TNode* node);
b32 expr(Code* code, Expr* e);

b32
statement(Code* code, Statement* s) { announce_stage(s);
    switch(s->kind) {
        case statement::label: return label(code, s->node.first_child);
        case statement::block_final:
        case statement::expression: return expr(code, (Expr*)s->node.first_child);
    }
    return false;
}

b32
block(Code* code, Block* e) { announce_stage(e);
    //push_table(&e->table);
    for(TNode* n = e->node.first_child; n; n = n->next) {
        if(!statement(code, (Statement*)n)) return false;
    }

    // resolve the type of this block
    auto last = (Statement*)e->node.last_child;
    if(!last || last->kind != statement::block_final) {
        e->type = &type::void_;
    } else {
        e->type = Type::resolve((TNode*)last);
    }
    //`pop_table();
    return true;
}

b32 
func_ret(Code* code, TNode* n) { announce_stage(n);
    if(n->kind == node::tuple) {

    }else if(n->kind == node::expression) {
        auto e = (Expr*)n;
        if(e->kind != expr::typeref) {
            diagnostic::sema::
                func_ret_expected_typeref(e->node.start);
            return false;
        }
        return true;
    }
    return false;
}

b32 
func_arg_tuple(Code* code, Tuple* t) { announce_stage(t);
    if(!t->node.child_count) return true;
    for(TNode* n = t->node.first_child; n; n = n->next) {
        if(n->kind == node::expression) {
            if(!expr(code, (Expr*)n)) return false;
        } else {
            if(!label(code, n)) return false;
        }
    }
    return true;
}

b32 
call(Code* code, Call* e) {
    FunctionType* f = e->callee->type;
    if(e->arguments->node.child_count > f->parameters->child_count) {
        diagnostic::sema::
            too_many_arguments(e->node.start, e->callee->label->node.start->raw);
        return false;
    } 

    if(f->parameters->child_count > e->arguments->node.child_count) {
        diagnostic::sema::
            not_enough_arguments(e->node.start, e->callee->label->node.start->raw);
        return false;
    }

    TNode* func_arg = f->parameters->first_child;
    TNode* call_arg = e->arguments->node.first_child ;
    while(func_arg && call_arg) {
        if(!Type::resolve(func_arg)->can_cast_to(Type::resolve(call_arg))) {
            diagnostic::sema::
                mismatch_argument_type(call_arg->start, 
                    Type::resolve(call_arg), 
                    Type::resolve(func_arg), 
                    label::resolve(func_arg)->node.start->raw, 
                    e->callee->label->node.start->raw);
            return false;
        }
        func_arg = func_arg->next;
        call_arg = call_arg->next;
    }
    e->type = f->return_type;
    return true;
} 


// TODO(sushi) need to separate a function's definition from its Type
//             really just need to not use the AST here and use the stuff on FunctionType
//             instead
b32
typeref(Code* code, Expr* e) { announce_stage(e);
    switch(e->type->kind) {
        case type::kind::scalar: {
        } break;
        case type::kind::function: {
            auto type = (FunctionType*)e->type;
            if(!func_arg_tuple(code, (Tuple*)type->parameters) ||
               !func_ret(code, type->returns) || 
               !block(code, (Block*)e->node.last_child)) return false;
            auto be = (Block*)e->node.last_child;
            if(!type->return_type->can_cast_to(be->type)) {
                diagnostic::sema::
                    return_value_of_func_block_cannot_be_coerced_to_func_return_type(
                        be->node.last_child->start, type->return_type, be->type);
                return false;
            }
        } break;
        case type::kind::structured: {
        } break;
        case type::kind::pointer: {
        } break;
    }
    return true;
}

b32 
access(Code* code, Expr* e, Expr* lhs, Type* lhs_type, Expr* rhs) {
    switch(lhs_type->kind) {
        case type::kind::scalar: {
            diagnostic::sema::
                cannot_access_members_scalar_type(lhs->node.start);
            return false;
        } break;
        case type::kind::array: {
            switch(rhs->node.start->hash) {
                case string::static_hash("ptr"):
                case string::static_hash("count"): return true;
                default: {
                    diagnostic::sema::
                        array_types_dont_have_member(rhs->node.start, rhs->node.start->raw);
                    return false;
                }
            }
        } break;
        case type::kind::function: {
            diagnostic::sema::
                cannot_access_members_of_function_type(lhs->node.start);
            return false;
        } break;
        case type::kind::pointer: {
            auto ptype = (Pointer*)lhs_type;
            if(ptype->type->kind == type::kind::pointer) {
                diagnostic::sema::
                    too_many_levels_of_indirection_for_access(lhs->node.start);
            }
            return access(code, e, lhs, ptype->type, rhs);
        } break;
        case type::kind::tuple: {
            auto ttype = (TupleType*)lhs_type;
            NotImplemented;
        } break;
        case type::kind::structured: {
            auto stype = (Structured*)lhs_type;
            auto [idx, found] = map::find(stype->structure->table.map, rhs->node.start->raw);
            if(!found) {
                diagnostic::sema::
                    unknown_member(rhs->node.start, (Type*)stype, rhs->node.start->raw);
                return false;
            }
            Label* l = array::read(stype->structure->table.map.values, idx);
            e->type = Type::resolve((TNode*)l->entity);
            return true;
        } break;
    }
    return false;
}

b32 
expr(Code* code, Expr* e) { announce_stage(e);
    switch(e->kind) {
        case expr::unary_comptime: return expr(code, (Expr*)e->node.first_child);
        case expr::unary_assignment: {
            if(!expr(code, (Expr*)e->node.first_child)) return false;
            e->type = Type::resolve(e->node.first_child);
            return true;
        } break;
        case expr::block: {
            return block(code, (Block*)e);
        } break;
        case expr::unary_reference: {
            return false;
        } break;
        case expr::call: {
            return call(code, (Call*)e);
        } break;
        case expr::typeref: return typeref(code, e);
        case expr::binary_assignment: {
            auto lhs = (Expr*)e->node.first_child;
            auto rhs = (Expr*)e->node.last_child;
            if(!expr(code, lhs)) return false;
            if(!expr(code, rhs)) return false;

            // TODO(sushi) type coersion
            if(!lhs->type->can_cast_to(rhs->type)) {
                diagnostic::sema::
                    cannot_implict_coerce(lhs->node.start, rhs->type, lhs->type);
                return false;
            }

            return true;
        } break;

        case expr::binary_access: {
            auto lhs = (Expr*)e->node.first_child;
            auto rhs = (Expr*)e->node.last_child;
            if(!expr(code, lhs)) return false;
            auto lhs_type = Type::resolve((TNode*)lhs);
            if(!lhs_type) {
                 diagnostic::sema::
                    invalid_type_lhs_access(lhs->node.start);
                return false;
            }

            return access(code, e, lhs, lhs_type, rhs);
        } break;

        case expr::binary_plus: {
            auto lhs = (Expr*)e->node.first_child;
            auto rhs = (Expr*)e->node.last_child;
            if(!expr(code, lhs)) return false;
            if(!expr(code, rhs)) return false;

            if(lhs->type == rhs->type) { 
                if(!lhs->type->is_scalar()) {
                    // NOTE(sushi) temp error until traits are implemented
                    Message m = message::init(String("cannot perform addition between non-scalar types yet!"));
                    m.kind = message::error;
                    m = message::attach_sender(e->node.start, m);
                    messenger::dispatch(m);
                    return false;
                }
                e->type = lhs->type;
            } else if(lhs->type->is_scalar() && rhs->type->is_scalar()) {
                // take the larger of the two, and prefer float > signed > unsigned
                auto l = (ScalarType*)lhs->type, 
                     r = (ScalarType*)rhs->type;
                
                auto cast = Expr::create(expr::cast);
                b32 take_left = l->kind > r->kind;
                cast->type = (take_left? l : r);
                node::insert_above((take_left? (TNode*)rhs : (TNode*)lhs), (TNode*)cast);
                e->type = cast->type;
            } else {
                diagnostic::sema::
                    cant_find_binop_trait(e->node.start, "Add", lhs->type, rhs->type);
                return false;
            }
            return true;

            // TODO(sushi) Add trait implemented here
        } break;

        case expr::binary_minus: {
            auto lhs = (Expr*)e->node.first_child;
            auto rhs = (Expr*)e->node.last_child;
            if(!expr(code, lhs)) return false;
            if(!expr(code, rhs)) return false;

            if(lhs->type == rhs->type) { 
                if(!lhs->type->is_scalar()) {
                    // NOTE(sushi) temp error until traits are implemented
                    Message m = message::init(String("cannot perform subtraction between non-scalar types yet!"));
                    m.kind = message::error;
                    m = message::attach_sender(e->node.start, m);
                    messenger::dispatch(m);
                    return false;
                }
                e->type = lhs->type;
            } else if(lhs->type->is_scalar() && rhs->type->is_scalar()) {
                // take the larger of the two, and prefer float > signed > unsigned
                auto l = (ScalarType*)lhs->type, 
                     r = (ScalarType*)rhs->type;
                
                Expr* cast = Expr::create(expr::cast);
                b32 take_left = l->kind > r->kind;
                cast->type = (take_left? l : r);
                node::insert_above((take_left? (TNode*)rhs : (TNode*)lhs), (TNode*)cast);
                e->type = cast->type;
            } else {
                diagnostic::sema::
                    cant_find_binop_trait(e->node.start, "Sub", lhs->type, rhs->type);
                return false;
            }
            return true;
        } break;

        case expr::binary_multiply: {
            auto lhs = (Expr*)e->node.first_child;
            auto rhs = (Expr*)e->node.last_child;
            if(!expr(code, lhs)) return false;
            if(!expr(code, rhs)) return false;

            if(lhs->type == rhs->type) { 
                if(!lhs->type->is_scalar()) {
                    // NOTE(sushi) temp error until traits are implemented
                    Message m = message::init(String("cannot perform multiplication between non-scalar types yet!"));
                    m.kind = message::error;
                    m = message::attach_sender(e->node.start, m);
                    messenger::dispatch(m);
                    return false;
                }
                e->type = lhs->type;
            } else if(lhs->type->is_scalar() && rhs->type->is_scalar()) {
                // take the larger of the two, and prefer float > signed > unsigned
                auto l = (ScalarType*)lhs->type, 
                     r = (ScalarType*)rhs->type;
                
                Expr* cast = Expr::create(expr::cast);
                b32 take_left = l->kind > r->kind;
                cast->type = (take_left? l : r);
                node::insert_above((take_left? (TNode*)rhs : (TNode*)lhs), (TNode*)cast);
                e->type = cast->type;
            } else {
                diagnostic::sema::
                    cant_find_binop_trait(e->node.start, "Mul", lhs->type, rhs->type);
                return false;
            }
            return true;
        } break;

        case expr::binary_division: {
            auto lhs = (Expr*)e->node.first_child;
            auto rhs = (Expr*)e->node.last_child;
            if(!expr(code, lhs)) return false;
            if(!expr(code, rhs)) return false;

            if(lhs->type == rhs->type) { 
                if(!lhs->type->is_scalar()) {
                    // NOTE(sushi) temp error until traits are implemented
                    Message m = message::init(String("cannot perform division between non-scalar types yet!"));
                    m.kind = message::error;
                    m = message::attach_sender(e->node.start, m);
                    messenger::dispatch(m);
                    return false;
                }
                e->type = lhs->type;
            } else if(lhs->type->is_scalar() && rhs->type->is_scalar()) {
                // take the larger of the two, and prefer float > signed > unsigned
                auto l = (ScalarType*)lhs->type, 
                     r = (ScalarType*)rhs->type;
                
                Expr* cast = Expr::create(expr::cast);
                b32 take_left = l->kind > r->kind;
                cast->type = (take_left? l : r);
                node::insert_above((take_left? (TNode*)rhs : (TNode*)lhs), (TNode*)cast);
                e->type = cast->type;
            } else {
                diagnostic::sema::
                    cant_find_binop_trait(e->node.start, "Div", lhs->type, rhs->type);
                return false;
            }
            return true;
        } break;

        case expr::literal: {
            return true;
        } break;

        case expr::placeref: {
            auto pr = (PlaceRef*)e;
            e->type = pr->place->type;
            return true;
        } break;

        case expr::conditional: {
            if(!expr(code, (Expr*)e->node.first_child)) return false;
            
            Type* first_type = 0;
            for(TNode* n = e->node.first_child->next; n; n = n->next) {
                auto branch = (Expr*)n;
                if(!expr(code, branch)) return false;
                if(!first_type) first_type = branch->type;
                else {
                    if(!first_type->can_cast_to(branch->type)) {
                        diagnostic::sema::
                            if_mismatched_types_cannot_coerce(branch->node.start, branch->type, first_type);
                        return false;
                    }
                }
            }

            e->type = first_type;

            return true;
        } break;
        
    }

    return false;
}

b32
label(Code* code, TNode* node) { announce_stage(node);
    if(!expr(code, (Expr*)node->last_child)) return false;

    auto l = (Label*)node;
    switch(l->entity->node.kind) {
        case node::place: {
            auto p = (Place*)l->entity;
            p->type = Type::resolve(l->node.last_child);
            if(p->type == &type::void_) {
                diagnostic::sema::cannot_have_a_variable_of_void_type(l->node.last_child->start);
            }
        } break;
    }
    return true;
}

b32
module(Code* code, TNode* node) { announce_stage(node);
    Module* m = (Module*)node;
    //push_table(&m->table);
    for(TNode* n = node->first_child; n; n = n->next) {
        if(!label(code, n)) return false;
    }  
    //pop_table();
    return true;
}

b32 
start(Code* code) {
    switch(code->parser->root->kind) {
        case node::module: {
            if(!module(code, code->parser->root)) return false;
        } break;
    }
    return true;
}

} // namespace internal

namespace table {

void
push(Code* code);

void
pop(Code* code);

} // namespace table 

b32
analyze(Code* code) {
    if(!code->sema) code->sema = sema::create();
    return internal::start(code);
}

} // namespace sema
} // namespace amu