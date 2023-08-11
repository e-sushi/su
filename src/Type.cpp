namespace amu {
namespace type {

global Type*
create() {
    Type* out = pool::add(compiler::instance.storage.types);
    node::init(&out->node);
    out->node.kind = node::type;
    return out; 
}

global Type*
base(Type& t) {
    Type* out = &t;
    while(out->node.parent)
        out = (Type*)out->node.parent;
    return out;
}

} // namespace type

void
to_string(DString& start, Type* t) {
    dstring::append(start, 
        node::util::print_tree<[](DString& current, TNode* n) {
            if(n->kind != node::type) return to_string(current, n, true);

            Type* t = (Type*)n;

            // this is a pointer
            if(!t->structure) return dstring::append(current, "ptr");

            to_string(current, t->structure);
        }>((TNode*)t, false));
}

} // namespace amu

