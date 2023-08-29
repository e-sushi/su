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

b32 label(Code* code, Label* l);
b32 expr(Code* code, Expr* e);

b32
statement(Code* code, Stmt* s) { announce_stage(s);
    switch(s->kind) {
        case statement::label: return label(code, s->first_child<Label>());
        case statement::block_final:
        case statement::expression: return expr(code, s->first_child<Expr>());
    }
    return false;
}

b32
block(Code* code, Block* e) { announce_stage(e);
    for(auto n = e->first_child<Stmt>(); n; n = n->next<Stmt>()) {
        if(!statement(code, n)) return false;
    }

    // resolve the type of this block
    auto last = e->last_child<Stmt>();
    if(!last || last->kind != statement::block_final) {
        e->type = &type::void_;
    } else {
        e->type = last->resolve_type();
    }
    return true;
}

b32 
func_ret(Code* code, ASTNode* n) { announce_stage(n);
    if(n->is<Tuple>()) {
        if(n->is(statement::label)) {

        }
    }else if(n->is<Expr>()) {
        auto e = n->as<Expr>();
        if(e->kind != expr::typeref) {
            diagnostic::sema::
                func_ret_expected_typeref(e->start);
            return false;
        }
        return true;
    }
    return false;
}

b32 
func_arg_tuple(Code* code, Tuple* t) { announce_stage(t);
    if(!t->child_count) return true;
    for(ASTNode* n = t->first_child(); n; n = n->next()) {
        if(n->is<Expr>()) {
            if(!expr(code, n->as<Expr>())) return false;
        } else {
            if(!label(code, n->as<Label>())) return false;
        }
    }
    return true;
}

b32 
call(Code* code, Call* e) {
    FunctionType* f = e->callee->type;
    if(e->arguments->child_count > f->parameters->child_count) {
        diagnostic::sema::
            too_many_arguments(e->start, e->callee->label->start->raw);
        return false;
    } 

    if(f->parameters->child_count > e->arguments->child_count) {
        diagnostic::sema::
            not_enough_arguments(e->start, e->callee->label->start->raw);
        return false;
    }

    ASTNode* func_arg = f->parameters->first_child();
    ASTNode* call_arg = e->arguments->first_child();
    while(func_arg && call_arg) {
        auto func_arg_t = func_arg->resolve_type();
        auto call_arg_t = call_arg->resolve_type();
        if(!func_arg_t->can_cast_to(call_arg_t)) {
            diagnostic::sema::
                mismatch_argument_type(call_arg->start, 
                    call_arg_t, 
                    func_arg_t, 
                    label::resolve(func_arg)->start->raw, 
                    e->callee->label->start->raw);
            return false;
        }
        func_arg = func_arg->next();
        call_arg = call_arg->next();
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
            auto be = e->last_child<Block>();
            if(!func_arg_tuple(code, (Tuple*)type->parameters) ||
               !func_ret(code, type->returns) || 
               !block(code, be)) return false;
            if(!type->return_type->can_cast_to(be->type)) {
                diagnostic::sema::
                    return_value_of_func_block_cannot_be_coerced_to_func_return_type(
                        be->last_child()->start, type->return_type, be->type);
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
                cannot_access_members_scalar_type(lhs->start);
            return false;
        } break;
        case type::kind::static_array: {
            switch(rhs->start->hash) {
                case string::static_hash("ptr"):
                case string::static_hash("count"): return true;
                default: {
                    diagnostic::sema::
                        array_types_dont_have_member(rhs->start, rhs->start->raw);
                    return false;
                }
            }
        } break;
        case type::kind::function: {
            diagnostic::sema::
                cannot_access_members_of_function_type(lhs->start);
            return false;
        } break;
        case type::kind::pointer: {
            auto ptype = (Pointer*)lhs_type;
            if(ptype->type->kind == type::kind::pointer) {
                diagnostic::sema::
                    too_many_levels_of_indirection_for_access(lhs->start);
            }
            return access(code, e, lhs, ptype->type, rhs);
        } break;
        case type::kind::tuple: {
            auto ttype = (TupleType*)lhs_type;
            NotImplemented;
        } break;
        case type::kind::structured: {
            auto stype = (Structured*)lhs_type;
            auto [idx, found] = map::find(stype->structure->table.map, rhs->start->raw);
            if(!found) {
                diagnostic::sema::
                    unknown_member(rhs->start, stype, rhs->start->raw);
                return false;
            }
            Label* l = array::read(stype->structure->table.map.values, idx);
            e->type = l->entity->resolve_type();
            return true;
        } break;
    }
    return false;
}

b32 
expr(Code* code, Expr* e) { announce_stage(e);
    switch(e->kind) {
        case expr::unary_comptime: return expr(code, e->first_child<Expr>());
        case expr::unary_assignment: {
            if(!expr(code, e->first_child<Expr>())) return false;
            e->type = e->first_child()->resolve_type();
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
            auto lhs = e->first_child<Expr>();
            auto rhs = e->last_child<Expr>();
            if(!expr(code, lhs)) return false;
            if(!expr(code, rhs)) return false;

            // TODO(sushi) type coersion
            if(!lhs->type->can_cast_to(rhs->type)) {
                diagnostic::sema::
                    cannot_implict_coerce(lhs->start, rhs->type, lhs->type);
                return false;
            }

            return true;
        } break;

        case expr::binary_access: {
            auto lhs = e->first_child<Expr>();
            auto rhs = e->last_child<Expr>();
            if(!expr(code, lhs)) return false;
            auto lhs_type = lhs->resolve_type();
            if(!lhs_type) {
                 diagnostic::sema::
                    invalid_type_lhs_access(lhs->start);
                return false;
            }

            return access(code, e, lhs, lhs_type, rhs);
        } break;

        case expr::binary_plus: {
            auto lhs = e->first_child<Expr>();
            auto rhs = e->last_child<Expr>();
            if(!expr(code, lhs)) return false;
            if(!expr(code, rhs)) return false;

            if(lhs->type == rhs->type) { 
                if(!lhs->type->is_scalar()) {
                    // NOTE(sushi) temp error until traits are implemented
                    Message m = message::init(String("cannot perform addition between non-scalar types yet!"));
                    m.kind = message::error;
                    m = message::attach_sender(e->start, m);
                    messenger::dispatch(m);
                    return false;
                }
                e->type = lhs->type;
            } else if(lhs->type->is_scalar() && rhs->type->is_scalar()) {
                // take the larger of the two, and prefer float > signed > unsigned
                auto l = (Scalar*)lhs->type, 
                     r = (Scalar*)rhs->type;
                
                auto cast = Expr::create(expr::cast);
                b32 take_left = l->kind > r->kind;
                cast->type = (take_left? l : r);
                node::insert_above((take_left? (TNode*)rhs : (TNode*)lhs), (TNode*)cast);
                e->type = cast->type;
            } else {
                diagnostic::sema::
                    cant_find_binop_trait(e->start, "Add", lhs->type, rhs->type);
                return false;
            }
            return true;

            // TODO(sushi) Add trait implemented here
        } break;

        case expr::binary_minus: {
            auto lhs = e->first_child<Expr>();
            auto rhs = e->last_child<Expr>();
            if(!expr(code, lhs)) return false;
            if(!expr(code, rhs)) return false;

            if(lhs->type == rhs->type) { 
                if(!lhs->type->is_scalar()) {
                    // NOTE(sushi) temp error until traits are implemented
                    Message m = message::init(String("cannot perform subtraction between non-scalar types yet!"));
                    m.kind = message::error;
                    m = message::attach_sender(e->start, m);
                    messenger::dispatch(m);
                    return false;
                }
                e->type = lhs->type;
            } else if(lhs->type->is_scalar() && rhs->type->is_scalar()) {
                // take the larger of the two, and prefer float > signed > unsigned
                auto l = (Scalar*)lhs->type, 
                     r = (Scalar*)rhs->type;
                
                Expr* cast = Expr::create(expr::cast);
                b32 take_left = l->kind > r->kind;
                cast->type = (take_left? l : r);
                node::insert_above((take_left? (TNode*)rhs : (TNode*)lhs), (TNode*)cast);
                e->type = cast->type;
            } else {
                diagnostic::sema::
                    cant_find_binop_trait(e->start, "Sub", lhs->type, rhs->type);
                return false;
            }
            return true;
        } break;

        case expr::binary_multiply: {
            auto lhs = e->first_child<Expr>();
            auto rhs = e->last_child<Expr>();
            if(!expr(code, lhs)) return false;
            if(!expr(code, rhs)) return false;

            if(lhs->type == rhs->type) { 
                if(!lhs->type->is_scalar()) {
                    // NOTE(sushi) temp error until traits are implemented
                    Message m = message::init(String("cannot perform multiplication between non-scalar types yet!"));
                    m.kind = message::error;
                    m = message::attach_sender(e->start, m);
                    messenger::dispatch(m);
                    return false;
                }
                e->type = lhs->type;
            } else if(lhs->type->is_scalar() && rhs->type->is_scalar()) {
                // take the larger of the two, and prefer float > signed > unsigned
                auto l = (Scalar*)lhs->type, 
                     r = (Scalar*)rhs->type;
                
                Expr* cast = Expr::create(expr::cast);
                b32 take_left = l->kind > r->kind;
                cast->type = (take_left? l : r);
                node::insert_above((take_left? (TNode*)rhs : (TNode*)lhs), (TNode*)cast);
                e->type = cast->type;
            } else {
                diagnostic::sema::
                    cant_find_binop_trait(e->start, "Mul", lhs->type, rhs->type);
                return false;
            }
            return true;
        } break;

        case expr::binary_division: {
            auto lhs = e->first_child<Expr>();
            auto rhs = e->last_child<Expr>();
            if(!expr(code, lhs)) return false;
            if(!expr(code, rhs)) return false;

            if(lhs->type == rhs->type) { 
                if(!lhs->type->is_scalar()) {
                    // NOTE(sushi) temp error until traits are implemented
                    Message m = message::init(String("cannot perform division between non-scalar types yet!"));
                    m.kind = message::error;
                    m = message::attach_sender(e->start, m);
                    messenger::dispatch(m);
                    return false;
                }
                e->type = lhs->type;
            } else if(lhs->type->is_scalar() && rhs->type->is_scalar()) {
                // take the larger of the two, and prefer float > signed > unsigned
                auto l = (Scalar*)lhs->type, 
                     r = (Scalar*)rhs->type;
                
                Expr* cast = Expr::create(expr::cast);
                b32 take_left = l->kind > r->kind;
                cast->type = (take_left? l : r);
                node::insert_above((take_left? (TNode*)rhs : (TNode*)lhs), (TNode*)cast);
                e->type = cast->type;
            } else {
                diagnostic::sema::
                    cant_find_binop_trait(e->start, "Div", lhs->type, rhs->type);
                return false;
            }
            return true;
        } break;

        case expr::literal: {
            return true;
        } break;

        case expr::varref: {
            e->type = e->as<VarRef>()->var->type;
            return true;
        } break;

        case expr::conditional: {
            if(!expr(code, e->first_child<Expr>())) return false;
            
            Type* first_type = 0;
            for(auto branch = e->first_child()->next<Expr>(); branch; branch = branch->next<Expr>()) {
                if(!expr(code, branch)) return false;
                if(!first_type) first_type = branch->type;
                else {
                    if(!first_type->can_cast_to(branch->type)) {
                        diagnostic::sema::
                            if_mismatched_types_cannot_coerce(branch->start, branch->type, first_type);
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
label(Code* code, Label* node) { announce_stage(node);
    if(!expr(code, node->last_child<Expr>())) return false;

    auto l = (Label*)node;
    switch(l->entity->kind) {
        case entity::var: {
            auto v = l->entity->as<Var>();
            v->type = l->last_child()->resolve_type();
            if(v->type == &type::void_) {
                diagnostic::sema::
                    cannot_have_a_variable_of_void_type(l->last_child()->start);
                return false;
            }
        } break;
    }
    return true;
}

b32
module(Code* code, ASTNode* node) { announce_stage(node);
    Module* m = node->as<Module>();
    for(Label* l = node->first_child<Label>(); l; l = l->next<Label>()) {
        if(!label(code, l)) return false;
    }  
    return true;
}

b32 
start(Code* code) {
    switch(code->parser->root->kind) {
        case ast::module: {
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