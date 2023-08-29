namespace amu {

Tuple* Tuple::
create() {
    Tuple* out = pool::add(compiler::instance.storage.tuples);
    out->ASTNode::kind = ast::tuple;
    out->table = label::table::init((TNode*)out);
    return out;
}

void
to_string(DString& start, Tuple* t) {
    dstring::append(start, "Tuple<", tuple::strings[t->kind], ">");
}

} // namespace amu::tuple