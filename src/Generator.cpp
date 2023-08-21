namespace amu {
namespace generator {

Generator*
create(Code* code) {
    Generator* out = pool::add(compiler::instance.storage.generators);
    out->tac = array::init<TAC>();
    return out;
}

namespace internal {

Generator* generator;

void
function(Function* f) {
    FunctionType* ft = f->type;
    
    // add TAC to push the function's arguments onto the stack 
    for(TNode* n = ft->parameters->first_child; n; n = n->next) {
        TAC* tac = array::push(generator->tac);
        tac->op = tac::op::stack_push;
        tac->arg0.kind = tac::arg::literal;
        tac->arg0.literal = type::size(type::resolve(n));
    }
} 

void
label(Label* l) {
    switch(l->entity->node.kind) {
        case node::function: {
            function((Function*)&l->entity->node);
        } break;
    }
}

void
module(TNode* node) {
    for(TNode* n = node->first_child; n; n = n->next) {
        label((Label*)n);
    }
}

} // namespace internal

void
execute(Code* code) {
    internal::generator = code->generator;

    internal::module(code->node);
}

} // namespace generator
} // namespace amu