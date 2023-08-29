namespace amu {

Var* Var::
create(Type* type) {
    Var* out = pool::add(compiler::instance.storage.vars);
    out->ASTNode::kind = ast::entity;
    out->type = type;
    return out;
}

String Var::
name() {
    return label->name();
}

DString Var::
debug_str() {
    return dstring::init("Var<", label->name(), ">");
}

void
to_string(DString& start, Var* p) {
    dstring::append(start, "Place<'", p->label, "' type:", p->type, ">");
}

} // namespace amu