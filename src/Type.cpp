namespace amu::type {

global Type*
create() {
    Type* out = pool::add(compiler::instance.storage.types);
    node::init(&out->node);
    return out; 
}

global Type*
base(Type& t) {
    Type* out = &t;
    while(out->node.parent)
        out = (Type*)out->node.parent;
    return out;
}

} // namespace amu::type

