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

b32 label(TNode* node);

b32
statement(Statement* s) { announce_stage(s);
    switch(s->kind) {
        case statement::label: return label(s->node.first_child);
        case statement::expression: NotImplemented;
    }
    return false;
}

b32
block(Expression* e) { announce_stage(e);
    for(TNode* n = e->node.first_child; n; n = n->next) {
        if(!statement((Statement*)n)) return false;
    }
    return true;
}

b32 
func_ret(TNode* n) { announce_stage(n);
    if(n->kind == node::tuple) {

    }else if(n->kind == node::expression) {
        auto e = (Expression*)n;
        if(e->kind != expression::typeref) {
            diagnostic::validator::func_ret_expected_typeref(e->node.start);
            return false;
        }
        return true;
    }

    return false;
}

b32 
func_arg_tuple(Tuple* t) { announce_stage(t);
    if(!t->node.child_count) return true;
    return false;
}

b32
typeref(Expression* e) { announce_stage(e);

    switch(e->type->kind) {
        case type::kind::scalar: {
            // I don't think we need to do anything special for scalar type references
            return true;
        } break;
        case type::kind::function: {
            auto type = (FunctionType*)e->type;
            if(!func_arg_tuple((Tuple*)type->parameters) ||
               !func_ret(type->returns) || 
               !block((Expression*)e->node.last_child)) return false;
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
expr(Expression* e) { announce_stage(e);
    switch(e->kind) {
        case expression::unary_comptime: return expr((Expression*)e->node.first_child);
        case expression::unary_assignment: return expr((Expression*)e->node.first_child);
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
    for(TNode* n = node->first_child; n; n = n->next) {
        if(!label(n)) return false;
    }

    return true;
}

} // namespace internal

void
execute(Code* code) {
    // TODO(sushi) this will be incorrect when Code becomes more general
    internal::module((TNode*)code->source->module);
}

} // namespace validator
} // namespace amu