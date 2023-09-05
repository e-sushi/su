namespace amu {

Var* Var::
create(Type* type) {
    Var* out = pool::add(compiler::instance.storage.vars);
    out->ASTNode::kind = ast::entity;
    out->type = type;
    return out;
}

DString* Var::
name() {
    return (label? label->name() : DString::create("anon/temp var"));
}

DString* Var::
dump() {
    return DString::create("Var<", name(), ">");
}

void
to_string(DString*& start, Var* p) {
    start->append("Place<'", p->label, "' type:", p->type, ">");
}

} // namespace amu