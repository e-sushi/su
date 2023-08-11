namespace amu::type {

global Type*
create() {
    Type* out = pool::add(compiler::instance.storage.types);
    node::init(&out->node);
    return out; 
}

Type*
add_indirection(Type* type, Structure* s) {
    Type* out = type::create();
    // out->indirection = type;
    // out->structure = s;
    return out;
}

} // namespace amu::type

