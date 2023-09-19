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

#define announce_stage(n) 
    // DString indentation = DString::create(); \
    // forI(layers) indentation->append(" "); \
    // messenger::dispatch(message::make_debug(message::verbosity::debug, String(indentation), String("validating "), String(__func__), String(": "), (String)to_string((TNode*)n, true))); \
    // layers++; \
    // defer { layers--; }

// Array<LabelTable*> table_stack = Array<LabelTable*>::create();
// LabelTable* current_table = 0;

// void push_table(LabelTable* lt) {
//     table_stack.push(current_table);
//     current_table = lt;
// }

// void pop_table() {
//     current_table = table_stack.pop();
// }

b32 label(Code* code, Label* l);
b32 expr(Code* code, Expr* e, b32 is_lvalue);

b32
statement(Code* code, Stmt* s) { announce_stage(s);
    switch(s->kind) {
        case stmt::label: return label(code, s->first_child<Label>());
        case stmt::block_final:
        case stmt::expression: return expr(code, s->first_child<Expr>(), false);
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
    if(!last || last->kind != stmt::block_final) {
        e->type = &type::void_;
    } else {
        e->type = last->first_child<Expr>()->type;
    }
    return true;
}

b32 
func_ret(Code* code, ASTNode* n) { announce_stage(n);
    if(n->is<Tuple>()) {
        if(n->is(stmt::label)) {
            
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
            if(!expr(code, n->as<Expr>(), false)) return false;
        } else {
            if(!label(code, n->as<Label>())) return false;
        }
    }
    return true;
}

b32
function(Code* code, Function* f) {
    auto type = f->type;
    auto be = f->last_child<Block>();
    if(!func_arg_tuple(code, type->parameters->as<Tuple>()) ||
        !func_ret(code, type->returns) || 
        !block(code, be)) return false;
    // TODO(sushi) this only handles functions that return at the very end!
    //             return statements need to be aware of what function they are returning from
    //             so they can handle the casting themselves
    if(type->return_type != be->type) {
        if(!type->return_type->can_cast_to(be->type)) {
            diagnostic::sema::
                return_value_of_func_block_cannot_be_coerced_to_func_return_type(
                    be->last_child()->start, type->return_type, be->type);
            return false;
        }

        if(!(type->return_type->is<Scalar>() && be->type->is<Scalar>())) {
            diagnostic::sema::
                casting_between_non_scalar_types_not_supported(be->last_child()->start);
            return false;                
        }
        
        auto retexpr = be->last_child()->last_child<Expr>();

        if(retexpr->is<ScalarLiteral>()) {
            retexpr->as<ScalarLiteral>()->cast_to(type->return_type);
            return true;
        }

        auto cast = Expr::create(expr::cast);
        cast->type = type->return_type;
        cast->start = retexpr->start;
        cast->end = retexpr->end;
        node::insert_above(retexpr, cast);
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
        if(!expr(code, call_arg->as<Expr>(), false)) return false;
        auto func_arg_t = func_arg->resolve_type();
        auto call_arg_t = call_arg->resolve_type();

        if(call_arg_t != func_arg_t) {
            if(!func_arg_t->can_cast_to(call_arg_t)) {
                diagnostic::sema::
                    mismatch_argument_type(call_arg->start, 
                        call_arg_t, 
                        func_arg_t, 
                        func_arg->display(), 
                        e->callee->label->start->raw);
                return false;
            }
            if(call_arg->is<ScalarLiteral>()) {
                // if this is a scalar literal just cast it in place 
                call_arg->as<ScalarLiteral>()->cast_to(func_arg_t->as<Scalar>()->kind);
            } else {
                auto cast = Expr::create(expr::cast);
                cast->type = func_arg_t;
                cast->start = call_arg->start;
                cast->end = call_arg->end;
                node::insert_above(call_arg, cast);
                call_arg = cast;
            }
        }

        func_arg = func_arg->next();
        call_arg = call_arg->next();
    }
    e->type = f->return_type;
    return true;
} 

b32
typedef_(Code* code, Expr* e) {
    switch(e->type->kind) {
        case type::kind::structured: {
            auto s = e->type->as<Structured>()->structure;
            for(Member* m = e->first_child<Member>(); m; m = m->next<Member>()) {
                if(!expr(code, m->last_child<Expr>(), false)) return false;
                m->type = m->last_child<Expr>()->type;
                m->offset = s->size;
                // if size is 0, then we must have ran into a case where we don't know the size of something yet
                // and we need to handle parsing dependencies first 
                Assert(m->type->size()); 
                s->size += m->type->size();
            }
            s->first_member = e->first_child<Member>();
        } break;
    }
    return true;
}

b32
typeref(Code* code, Expr* e) { announce_stage(e);
    switch(e->type->kind) {
        case type::kind::scalar: {
        } break;
        case type::kind::function: {
        } break;
        case type::kind::structured: {
            auto st = e->type->as<Structured>();
            if(st->is<StaticArray>()) {
                auto sa = st->as<StaticArray>();

            }
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
        case type::kind::function: {
            diagnostic::sema::
                cannot_access_members_of_function_type(lhs->start);
            return false;
        } break;
        case type::kind::pointer: {
            auto ptype = (Pointer*)lhs_type;
            if(ptype->type->is<Pointer>()) {
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
            auto stype = lhs_type->as<Structured>();
            if(stype->is<StaticArray>()) {
                if(rhs->start->raw.equal("count")) {
                    auto nu = ScalarLiteral::create();
                    nu->value = stype->as<StaticArray>()->count;
                    nu->type = &scalar::_u64;
                    node::insert_above(e, nu);
                    e->type = nu->type;
                    // TODO(sushi) clear up the weird behavoir here
                    //             even though we replace e with nu, the caller doesn't know that
                    //             for now I just reset lhs and rhs where this is called (one case)
                    //             so eventually I'll need to handle all the cases
                } else if(rhs->start->raw.equal("data")) {
                    auto nu = Expr::create(expr::unary_reference);
                    node::change_parent(nu, e->first_child());
                    node::insert_above(e, nu);
                    nu->type = Pointer::create(stype->as<StaticArray>()->type);
                } else {
                    diagnostic::sema::
                        static_array_invalid_member(rhs->start, rhs->start->raw);
                    return false;
                }
                return true;
            }

            Member* m = stype->structure->find_member(rhs->start->raw);
            if(!m) {
                diagnostic::sema::
                    unknown_member(rhs->start, stype, rhs->start->raw);
                return false;
            }
            e->member = m;
            e->type = m->type;
            return true;
        } break;
    }
    return false;
}

b32 
expr(Code* code, Expr* e, b32 is_lvalue) { announce_stage(e);
    defer {
        if(is_lvalue) e->lvalue = true;
    };
    switch(e->kind) {
        case expr::unary_comptime: {
            return expr(code, e->first_child<Expr>(), is_lvalue);
        } break;
        case expr::unary_assignment: {
            if(!expr(code, e->first_child<Expr>(), is_lvalue)) return false;
            e->type = e->first_child()->resolve_type();
        } break;
        case expr::block: {
            return block(code, (Block*)e);
        } break;
        case expr::unary_reference: {
            if(!expr(code, e->first_child<Expr>(), is_lvalue)) return false;

            if(e->first_child<Expr>()->is_not<VarRef>()) {
                diagnostic::sema::
                    cant_take_reference_of_value(e->start);
                return false;
            } 
           
            e->type = Pointer::create(e->first_child<Expr>()->type);
        } break;
        case expr::unary_dereference: {
            if(!expr(code, e->first_child<Expr>(), true)) return false;

            if(e->first_child<Expr>()->type->is_not<Pointer>()) {
                diagnostic::sema::
                    dereference_operator_only_works_on_pointers(e->start);
                return false;
            }

            e->type = e->first_child<Expr>()->type->as<Pointer>()->type;
            is_lvalue = true;
        } break;
        case expr::call: {
            return call(code, (Call*)e);
        } break;
        case expr::typeref: return typeref(code, e);
        case expr::function: return function(code, e->as<Function>());
        case expr::typedef_: return typedef_(code, e);
        case expr::binary_assignment: {
            auto lhs = e->first_child<Expr>();
            auto rhs = e->last_child<Expr>();
            if(!expr(code, lhs, true)) return false;
            if(!expr(code, rhs, is_lvalue)) return false;

            if(rhs->type->is<Whatever>()) {
                diagnostic::sema::cant_use_whatever_as_value(rhs->start);
                return false;
            }

            if(lhs->type != rhs->type) {
                if(!lhs->type->can_cast_to(rhs->type)) {
                    diagnostic::sema::
                        cannot_implict_coerce(lhs->start, rhs->type, lhs->type);
                    return false;
                }
                
                if(lhs->type->is<Scalar>() && rhs->type->is<Scalar>()) {
                    // always take the left because we need to match the type that we are assigning to
                    if(rhs->is<ScalarLiteral>()) {
                        // if we're dealing with a literal, we just have to ask it to cast its value
                        // for us, no cast node is needed
                        rhs->as<ScalarLiteral>()->cast_to(lhs->type->as<Scalar>()->kind);
                        e->type = rhs->type;
                    } else {
                        // otherwise we have to insert an actual cast
                        auto cast = Expr::create(expr::cast);
                        cast->type = lhs->type;
                        cast->start = e->start;
                        cast->end = e->end;
                        node::insert_above(rhs, cast);
                        e->type = cast->type;
                    }
                } else if(lhs->type->is<StaticArray>() && rhs->type->is<StaticArray>()) {
                    auto lsa = lhs->type->as<StaticArray>(),
                         rsa = rhs->type->as<StaticArray>();

                    if(lsa->type->is<Scalar>() && rsa->type->is<Scalar>()) {
                        // TODO(sushi) this incorrectly assumes that rhs is an array literal
                        rhs->as<ArrayLiteral>()->cast_to(lsa->type);
                    } else {
                        TODO("bulk cast with non-scalar arrays");
                    }
                } else {
                    TODO("handle other type casts in assignment");                    
                }
            } else {
                e->type = lhs->type;
            }

            lhs->lvalue = true;
        } break;

        case expr::binary_access: {
            auto lhs = e->first_child<Expr>();
            auto rhs = e->last_child<Expr>();
            if(!expr(code, lhs, is_lvalue)) return false;
            auto lhs_type = lhs->resolve_type();
            if(!lhs_type) {
                 diagnostic::sema::
                    invalid_type_lhs_access(lhs->start);
                return false;
            }

            return access(code, e, lhs, lhs_type, rhs);
        } break;

        case expr::binary_plus:
        case expr::binary_minus:
        case expr::binary_division: 
        case expr::binary_multiply:
        case expr::binary_modulo:
        case expr::binary_equal: 
        case expr::binary_not_equal: 
        case expr::binary_less_than:
        case expr::binary_less_than_or_equal:
        case expr::binary_greater_than:
        case expr::binary_greater_than_or_equal:  {
            auto lhs = e->first_child<Expr>();
            auto rhs = e->last_child<Expr>();
            if(!expr(code, lhs, is_lvalue)) return false;
            if(!expr(code, rhs, is_lvalue)) return false;
            lhs = e->first_child<Expr>();
            rhs = e->last_child<Expr>();

            // Rules define different patterns of the form 
            //     <lhs_type> <op> <rhs_type>
            // that are iterated and checked so we can handle
            // certain cases such as handling arithmetic between
            // any kind of scalar, checking for use of void/whatever
            // etc. If type::kind::null or expr::null are used, then
            // the Rule will match anything in that position.
            //
            // This is experimental to see if it's nicer to write than
            // what I was doing previously and the fact that the rule
            // list is iterated linearly probably makes this very 
            // inefficient
            struct Rule {
                type::kind lhs_type;
                type::kind rhs_type;
                expr::kind op;

                b32 (*action)(Expr*,Expr*,Expr*);
            };

            Rule rules[] = {
                
                {type::kind::whatever, type::kind::null, expr::null, 
                [](Expr* root, Expr* lhs, Expr* rhs)->b32 {
                    diagnostic::sema::cant_use_whatever_as_value(lhs->start);
                    return false;
                }},
                
                {type::kind::null, type::kind::whatever, expr::null, 
                [](Expr* root, Expr* lhs, Expr* rhs)->b32 {
                    diagnostic::sema::cant_use_whatever_as_value(rhs->start);
                    return false;
                }},

                {type::kind::scalar, type::kind::scalar, expr::null,
                [](Expr* root, Expr* lhs, Expr* rhs)->b32 {
                    auto l = lhs->type->as<Scalar>(), 
                         r = rhs->type->as<Scalar>();
                    
                    switch(root->kind) {
                        case expr::binary_modulo: {
                            if(l->is_float() || r->is_float()) {
                                diagnostic::sema::
                                    modulo_not_defined_on_floats(lhs->start);
                                return false;
                            }
                        } break;
                    }
                    
                    if(l == r) {
                        root->type = l;
                        return true;
                    }


                    b32 take_left = l->kind > r->kind;
                    if(take_left) {
                        if(rhs->is<ScalarLiteral>()) {
                            rhs->as<ScalarLiteral>()->cast_to(l->kind);
                            root->type = l;
                            return true;
                        }
                    } else {
                        if(lhs->is<ScalarLiteral>()) {
                            lhs->as<ScalarLiteral>()->cast_to(r->kind);
                            root->type = r;
                            return true;
                        }
                    }

                    auto cast = Expr::create(expr::cast);
                    cast->type = (take_left? l : r);
                    cast->start = root->start;
                    cast->end = root->end;
                    node::insert_above((take_left? rhs : lhs), cast);
                    root->type = cast->type;

                    return true;
                }},

                {type::kind::pointer, type::kind::scalar, expr::null,
                [](Expr* root, Expr* lhs, Expr* rhs)->b32 {
                    if(rhs->type->as<Scalar>()->is_float()) {
                        diagnostic::sema::
                            float_in_pointer_arithmetic(rhs->start);
                        return false;
                    }
                    switch(root->kind) {
                        case expr::binary_plus: 
                        case expr::binary_minus: {
                            root->type = lhs->type;
                        } break;    

                        case expr::binary_less_than:
                        case expr::binary_greater_than:
                        case expr::binary_less_than_or_equal:
                        case expr::binary_greater_than_or_equal: {
                            diagnostic::sema::
                                comparison_between_pointer_and_scalar(root->start);
                            return false;
                        } break;

                        case expr::binary_equal: 
                        case expr::binary_not_equal: {
                            if(!rhs->is<ScalarLiteral>() || rhs->as<ScalarLiteral>()->value._u64 != 0) {
                                diagnostic::sema::
                                    pointer_equality_non_zero_integer(rhs->start);
                                return false;
                            }
                        } break;

                        default: {
                            diagnostic::sema::
                                pointer_arithmetic_not_additive(root->start);
                            return false;
                        } break;
                    }
                    return true;
                }},

                {type::kind::scalar, type::kind::pointer, expr::null,
                [](Expr* root, Expr* lhs, Expr* rhs)->b32 {
                    diagnostic::sema::
                        pointer_on_rhs_of_arithmetic(root->start);
                    return false;
                }},
            };

            forI(sizeof(rules)/sizeof(Rule)) {
                Rule r = rules[i];
                if(r.lhs_type != type::kind::null && lhs->type->is_not(r.lhs_type)) continue;
                if(r.rhs_type != type::kind::null && rhs->type->is_not(r.rhs_type)) continue;
                if(r.op != expr::null && e->is_not(r.op)) continue;

                return r.action(e, lhs, rhs);
            }

            if(lhs->type == rhs->type) { 
                if(!lhs->type->is<Scalar>()) {
                    // NOTE(sushi) temp error until traits are implemented
                    Message m = message::init(String("cannot perform arithmetic between non-scalar types yet!"));
                    m.kind = message::error;
                    m = message::attach_sender(e->start, m);
                    messenger::dispatch(m);
                    return false;
                }
                e->type = lhs->type;
            } else {
                diagnostic::sema::
                    cant_find_binop_trait(e->start, // C++ IS SO GARBAGE
                      (e->kind == expr::binary_plus                  ? string::init("Add") :
                       e->kind == expr::binary_minus                 ? string::init("Sub") :
                       e->kind == expr::binary_multiply              ? string::init("Mul") :
                       e->kind == expr::binary_division              ? string::init("Div") :
                       e->kind == expr::binary_modulo                ? string::init("Mod") :
                       e->kind == expr::binary_equal                 ? string::init("Equal") : 
                       e->kind == expr::binary_not_equal             ? string::init("NotEqual") :
                       e->kind == expr::binary_less_than             ? string::init("LessThan") :
                       e->kind == expr::binary_less_than_or_equal    ? string::init("LessThanOrEqual") :
                       e->kind == expr::binary_greater_than          ? string::init("GreaterThan") :
                                                                       string::init("GreaterThanOrEqual")), 
                                                                       lhs->type, rhs->type);
                return false;
            }

            // TODO(sushi) Add trait implemented here
        } break;


        case expr::binary_or: 
        case expr::binary_and: {
            auto lhs = e->first_child<Expr>();
            auto rhs = e->last_child<Expr>();
            if(!expr(code, lhs, is_lvalue)) return false;
            if(!expr(code, rhs, is_lvalue)) return false;

            // control statements can only appear on the very right
            if(lhs->type->is<Whatever>()) {
                diagnostic::sema::
                    control_expressions_can_only_be_used_at_the_end_of_logical_operators(lhs->start);
                return false;
            }

            if(lhs->type == rhs->type) { 
                if(!lhs->type->is<Scalar>()) {
                    // NOTE(sushi) temp error until traits are implemented
                    Message m = message::init(String("cannot perform arithmetic between non-scalar types yet!"));
                    m.kind = message::error;
                    m = message::attach_sender(e->start, m);
                    messenger::dispatch(m);
                    return false;
                }
                e->type = lhs->type;
            } else if(lhs->type->is<Scalar>() && rhs->type->is<Scalar>()) {
                // take the larger of the two, and prefer float > signed > unsigned
                auto l = lhs->type->as<Scalar>(), 
                     r = rhs->type->as<Scalar>();
                
                auto cast = Expr::create(expr::cast);
                b32 take_left = l->kind > r->kind;
                cast->type = (take_left? l : r);
                node::insert_above((take_left? rhs : lhs), cast);
                e->type = cast->type;
            } else if(rhs->type->is_not<Whatever>()) {
                diagnostic::sema::
                    cant_find_binop_trait(e->start, "Or", lhs->type, rhs->type);
                return false;
            } else {
                e->type = lhs->type;
            }
        } break;

        case expr::varref: {
            if(code->compile_time && !e->as<VarRef>()->var->is_compile_time) {
                diagnostic::sema::
                    compile_time_code_cannot_reference_runtime_memory(e->start);
                return false;
            }

            auto v = e->as<VarRef>()->var;
            if(v->type->is<Range>()) {
                e->type = v->type->as<Range>()->type;
            } else {
                e->type = e->as<VarRef>()->var->type;
            }

        } break;

        case expr::conditional: {
            if(!expr(code, e->first_child<Expr>(), is_lvalue)) return false;

            if(e->first_child<Expr>()->type->is<Whatever>()) {
                diagnostic::sema::cant_use_whatever_as_value(e->first_child()->start);
                return false;
            }

            Type* first_type = 0;
            for(auto branch = e->first_child()->next<Expr>(); branch; branch = branch->next<Expr>()) {
                if(!expr(code, branch, is_lvalue)) return false;
                if(!first_type) first_type = branch->type;
                else {
                    if(!first_type->can_cast_to(branch->type)) {
                        // we need to check if the first type is a block returning void
                        // and if this is an else that is not a block
                        // this occurs in a case like 
                        // if(...) {...} else ...;
                        // which should behave like it does in C when the first branch isn't returning anything
                        // otherwise we would error saying that the type that the else is supposedly returning
                        // cannot cast to void 
                        if(first_type->is<Void>() && e->first_child()->next()->is<Block>() && !branch->is<Block>()) 
                            continue;
                        diagnostic::sema::
                            if_mismatched_types_cannot_coerce(branch->start, branch->type, first_type);
                        return false;
                    }
                }
            }
            e->type = first_type;
        } break;

        case expr::loop: {
            if(!expr(code, e->first_child<Expr>(), is_lvalue)) return false;

            e->type = e->first_child<Expr>()->type;
        } break;

        case expr::for_: {
            if(e->first_child()->is<Label>()) {
                if(!label(code, e->first_child<Label>())) return false;
            } else {
                if(!expr(code, e->first_child<Expr>(), is_lvalue)) return false;
            }
            if(!expr(code, e->last_child<Expr>(), is_lvalue)) return false;
            e->type = e->last_child<Expr>()->type; // TODO(sushi) handle for stuff more eventually
        } break;
        
        case expr::break_: {
            auto iter = e->parent();
            e->type = &type::whatever;
            while(1) {
                if(iter->is(expr::loop)) {
                    return true;
                } else {
                    iter = iter->parent();
                    if(!iter) {
                        diagnostic::sema::break_outside_of_loop(e->start);
                        return false;
                    }
                }
            }
        } break;

        case expr::binary_range: {
            auto lhs = e->first_child<Expr>();
            auto rhs = e->last_child<Expr>();
            if(!expr(code, lhs, is_lvalue)) return false;
            if(!expr(code, rhs, is_lvalue)) return false;
            lhs = e->first_child<Expr>();
            rhs = e->last_child<Expr>();

            if(!lhs->type->can_cast_to(rhs->type)) {
                diagnostic::sema::
                    range_mismatched_types(lhs->start);
                return false;
            }

            if(lhs->type->is_not<Scalar>() || rhs->type->is_not<Scalar>()) {
                diagnostic::sema::
                    range_non_scalar_not_supported(lhs->start);
                return false;
            }

            e->type = Range::create(lhs->type);
        } break;

        case expr::literal_scalar: {
            if(!e->type) {
                
            }
        } break;

        case expr::literal_string: {
            
        } break;

        case expr::literal_array: {
            Type* first_type = 0;
            u32 count = 0;
            for(Expr* elem = e->first_child<Expr>(); elem; elem = elem->next<Expr>()) {
                if(!expr(code, elem, is_lvalue)) return false;
                if(!first_type) first_type = elem->type;

                if(!elem->type->can_cast_to(first_type)) {
                    diagnostic::sema::
                        array_literal_type_mismatch(elem->start, count, elem->type, first_type);
                    return false;
                }

                if(elem->type->is<Scalar>() && first_type->is<Scalar>()) {
                    if(elem->is<ScalarLiteral>()) {
                        elem->as<ScalarLiteral>()->cast_to(first_type->as<Scalar>()->kind);
                    }
                }

                count++;
            }

            e->type = StaticArray::create(first_type, count);
        } break;

        case expr::subscript: {
            auto lhs = e->first_child<Expr>();
            auto rhs = e->last_child<Expr>();
            if(!expr(code, lhs, is_lvalue)) return false;
            if(!expr(code, rhs, is_lvalue)) return false;

            if(rhs->type->is_not<Scalar>() || 
               rhs->type->as<Scalar>()->is_float()) {
                diagnostic::sema::
                    subscript_must_evaluate_to_integer(rhs->start, rhs->type);
                return false;
            }

            if(lhs->type->is<Structured>()) {
                auto s = lhs->type->as<Structured>();
                switch(s->kind) {
                    case structured::user: {
                        diagnostic::sema::
                            type_is_not_subscriptable(lhs->start, lhs->type);
                        return false;
                    } break;

                    case structured::view_array: {
                        e->type = s->as<ViewArray>()->type;
                    } break;

                    case structured::dynamic_array: {
                        e->type = s->as<DynamicArray>()->type;
                    } break;

                    case structured::static_array: {
                        // TODO(sushi) this needs to check beyond just literals, but since constant propagation
                        //             isn't done until optimization, im not sure how we should handle that
                        //             probably just do constant prop in parsing
                        if(rhs->is<ScalarLiteral>()) {
                            auto sl = rhs->as<ScalarLiteral>();
                            if(abs(sl->value._s64) - (sl->is_negative()? 1 : 0) >= s->as<StaticArray>()->count) {
                                diagnostic::sema::
                                    subscript_out_of_bounds(rhs->start, sl->value._s64, s->as<StaticArray>()->count);
                            } 
                        }

                        e->type = s->as<StaticArray>()->type;
                    } break;
                }
            } else if(lhs->type->is<Pointer>()) {
                // TODO(sushi) this can just return the nth value before the pointer
                if(rhs->is<ScalarLiteral>() && rhs->as<ScalarLiteral>()->is_negative()) {
                    diagnostic::sema::
                        subscript_negative_not_allowed_on_pointer(rhs->start);
                    return false;
                }
                e->type = lhs->type->as<Pointer>()->type;
            } else {
                diagnostic::sema::
                    type_is_not_subscriptable(lhs->start, lhs->type);
                return false;
            }
            
            // If the subscript is not a u64 we cast it to one for now because it's going to be
            // added to a pointer. This will probably break later 
            if(rhs->type->as<Scalar>()->kind != scalar::unsigned64) {
                auto cast = Expr::create(expr::cast);
                cast->type = &scalar::_u64;
                cast->start = rhs->start;
                cast->end = rhs->end;
                node::insert_above(rhs, cast);
            }
        } break;

        case expr::cast: {
            if(e->type->is<Scalar>()) {
                auto c = e->first_child<Expr>();
                if(c->is<ScalarLiteral>()) {
                    c->as<ScalarLiteral>()->cast_to(e->type);
                    e->replace(c);
                }

            }
        } break;

        case expr::vm_break: {} break;

        default: {
            TODO(DString::create("unhandled expression kind: ", expr::strings[e->kind]));
        } break;    
    }

    return true;
}


b32
label(Code* code, Label* node) { announce_stage(node);
    switch(node->last_child()->kind) {
        case ast::entity: {
            switch(node->last_child<Entity>()->kind) {
                case entity::func: {
                    if(!function(code, node->last_child<Function>())) return false;
                } break;
                case entity::expr: {
                    if(!expr(code, node->last_child<Expr>(), false)) return false;
                } break;
            }
        } break;
    }

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
        case ast::entity: {
            auto e = code->parser->root->as<Entity>();
            switch(e->kind) {
                case entity::module: {
                    if(!module(code, code->parser->root)) return false;
                    // util::println(code->parser->root->print_tree(true));
                } break;

                case entity::expr: {
                    if(!expr(code, e->as<Expr>(), false)) return false;
                } break;

                default: DebugBreakpoint;
            }
        } break;

        case ast::label: {
            if(!label(code, code->parser->root->as<Label>())) return false;
        } break;

        default: DebugBreakpoint;
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