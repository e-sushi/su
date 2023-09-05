namespace amu {

Function* Function::
create(FunctionType* type) {
    Function* out = pool::add(compiler::instance.storage.functions);
    // node::init(&out->node);
    // out->node.kind = node::function;
    out->type = type;
    return out;
}


DString* Function::
name() {
    return label->name();
}

DString* Function::
dump() {
    return DString::create("Function<TODO>");
}

void
to_string(DString*& start, Function* f) {
    start->append("Function<TODO>");
}

} // namespace amu 