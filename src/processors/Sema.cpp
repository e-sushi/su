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
        case stmt::label: return label(code, s->first_child<Label>());
        case stmt::block_final:
        case stmt::expression: return expr(code, s->first_child<Expr>());
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
            if(!expr(code, n->as<Expr>())) return false;
        } else {
            if(!label(code, n->as<Label>())) return false;
        }
    }
    return true;
}

b32
funcdef(Code* code, Expr* e) {
    auto typeref = e->first_child<Expr>();
    auto type = typeref->type->as<FunctionType>();
    auto be = e->last_child<Block>();
    if(!func_arg_tuple(code, type->parameters->as<Tuple>()) ||
        !func_ret(code, type->returns) || 
        !block(code, be)) return false;
    if(!type->return_type->can_cast_to(be->type)) {
        diagnostic::sema::
            return_value_of_func_block_cannot_be_coerced_to_func_return_type(
                be->last_child()->start, type->return_type, be->type);
        return false;
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
        if(!expr(code, call_arg->as<Expr>())) return false;
        auto func_arg_t = func_arg->resolve_type();
        auto call_arg_t = call_arg->resolve_type();
        if(!func_arg_t->can_cast_to(call_arg_t)) {
            diagnostic::sema::
                mismatch_argument_type(call_arg->start, 
                    call_arg_t, 
                    func_arg_t, 
                    func_arg->name(), 
                    e->callee->label->start->raw);
            return false;
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
                if(!expr(code, m->last_child<Expr>())) return false;
                m->type = m->last_child<Expr>()->type;
                m->offset = s->size;
                // if size is 0, then we must have ran into a case where we don't know the size of something yet
                // and we need to handle parsing dependencies first 
                Assert(m->type->size()); 
                s->size += m->type->size();
            }
        } break;
    }
    return true;
}

// I honestly don't know what this function should do :/
b32
typeref(Code* code, Expr* e) { announce_stage(e);
    switch(e->type->kind) {
        case type::kind::scalar: {
        } break;
        case type::kind::function: {
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
expr(Code* code, Expr* e) { announce_stage(e);
    switch(e->kind) {
        case expr::unary_comptime: return expr(code, e->first_child<Expr>());
        case expr::unary_assignment: {
            if(!expr(code, e->first_child<Expr>())) return false;
            e->type = e->first_child()->resolve_type();
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
        case expr::func_def: return funcdef(code, e);
        case expr::typedef_: return typedef_(code, e);
        case expr::binary_assignment: {
            auto lhs = e->first_child<Expr>();
            auto rhs = e->last_child<Expr>();
            if(!expr(code, lhs)) return false;
            if(!expr(code, rhs)) return false;

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
                    if(rhs->is<Literal>()) {
                        // if we're dealing with a literal, we just have to ask it to cast its value
                        // for us, no cast node is needed
                    }

                    auto cast = Expr::create(expr::cast);
                    cast->type = lhs->type;
                    cast->start = e->start;
                    cast->end = e->end;
                    node::insert_above(rhs, cast);
                    e->type = cast->type;
                } else {
                    TODO("handle other type casts in assignment");                    
                }
            } else {
                e->type = lhs->type;
            }
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

        case expr::binary_plus:
        case expr::binary_minus:
        case expr::binary_division: 
        case expr::binary_multiply:
        case expr::binary_equal: 
        case expr::binary_not_equal: 
        case expr::binary_less_than:
        case expr::binary_less_than_or_equal:
        case expr::binary_greater_than:
        case expr::binary_greater_than_or_equal:  {
            auto lhs = e->first_child<Expr>();
            auto rhs = e->last_child<Expr>();
            if(!expr(code, lhs)) return false;
            if(!expr(code, rhs)) return false;

            if(lhs->type->is<Whatever>()) {
                diagnostic::sema::cant_use_whatever_as_value(lhs->start);
                return false;
            }

            if(rhs->type->is<Whatever>()) {
                diagnostic::sema::cant_use_whatever_as_value(rhs->start);
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
            } else {
                diagnostic::sema::
                    cant_find_binop_trait(e->start, // C++ IS SO GARBAGE
                      (e->kind == expr::binary_plus                  ? string::init("Add") :
                       e->kind == expr::binary_minus                 ? string::init("Sub") :
                       e->kind == expr::binary_multiply              ? string::init("Mul") :
                       e->kind == expr::binary_division              ? string::init("Div") :
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
            if(!expr(code, lhs)) return false;
            if(!expr(code, rhs)) return false;

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

        case expr::literal: {
        } break;

        case expr::varref: {
            e->type = e->as<VarRef>()->var->type;
        } break;

        case expr::conditional: {
            if(!expr(code, e->first_child<Expr>())) return false;

            if(e->first_child<Expr>()->type->is<Whatever>()) {
                diagnostic::sema::cant_use_whatever_as_value(e->first_child()->start);
                return false;
            }

            Type* first_type = 0;
            for(auto branch = e->first_child()->next<Expr>(); branch; branch = branch->next<Expr>()) {
                if(!expr(code, branch)) return false;
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
            if(!expr(code, e->first_child<Expr>())) return false;

            e->type = e->first_child<Expr>()->type;
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

        default: {
            TODO(DString::create("unhandled expression kind: ", expr::strings[e->kind]));
        } break;    
    }

    return true;
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
    if(code->parser->root->is<Module>()) {
        if(!module(code, code->parser->root)) return false;
        util::println(code->parser->root->print_tree(true));

    } else switch(code->parser->root->kind) {
        case ast::entity: {
            DebugBreakpoint;
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