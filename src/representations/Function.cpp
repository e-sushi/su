namespace amu {

Function* Function::
create(FunctionType* type) {
    Function* out = pool::add(compiler::instance.storage.functions);
    // node::init(&out->node);
    // out->node.kind = node::function;
    out->type = type;
    return out;
}


String Function::
name() {
    return label->name();
}

DString Function::
debug_str() {
    return dstring::init("Function<TODO>");
}

void
to_string(DString& start, Function* f) {
    dstring::append(start, "Function<TODO>");
}

} // namespace amu 