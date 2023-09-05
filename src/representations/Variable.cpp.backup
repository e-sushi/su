namespace amu {

Var* Var::
create(Type* type) {
    Var* out = pool::add(compiler::instance.storage.vars);
    out->ASTNode::kind = ast::entity;
    out->type = type;
    return out;
}

DString Var::
name() {
    return (label? label->name() : dstring::init("anon/temp var"));
}

DString Var::
dump() {
    return dstring::init("Var<", name(), ">");
}

void
to_string(DString& start, Var* p) {
    dstring::append(start, "Place<'", p->label, "' type:", p->type, ">");
}

} // namespace amu