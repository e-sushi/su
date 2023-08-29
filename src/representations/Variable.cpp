namespace amu {

Place* Place::
create(Type* type) {
    Place* out = pool::add(compiler::instance.storage.places);
    node::init(&out->node);
    out->node.kind = node::place;
    out->type = type;
    return out;
}

void
to_string(DString& start, Place* p) {
    dstring::append(start, "Place<'", p->label, "' type:", p->type, ">");
}

} // namespace amu