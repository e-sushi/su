namespace amu {

Var* Var::
create(Type* type) {
    Var* out = compiler::instance.storage.vars.add();
    out->ASTNode::kind = ast::entity;
    out->type = type;
    return out;
}

DString* Var::
display() {
    return (label? label->display() : DString::create("anon/temp var"));
}

DString* Var::
dump() {
    return DString::create("Var<", ScopedDeref(display()).x, ">");
}

void
to_string(DString* start, Var* p) {
    start->append("Place<'", p->label, "' type:", p->type, ">");
}

} // namespace amu
