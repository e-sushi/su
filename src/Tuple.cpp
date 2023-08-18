namespace amu {
namespace tuple {

global Tuple*
create() {
    Tuple* out = pool::add(compiler::instance.storage.tuples);
    node::init(&out->node);
    out->node.kind = node::tuple;
    out->table = label::table::init((TNode*)out);
    return out;
}

} // namespace tuple

void
to_string(DString& start, Tuple* t) {
    dstring::append(start, "Tuple<", tuple::strings[t->kind], ">");
}

} // namespace amu::tuple