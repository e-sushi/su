namespace amu {
namespace statement {

global Statement*
create() {
    Statement* out = pool::add(compiler::instance.storage.statements);
    node::init(&out->node);
    out->node.kind = node::statement;
    return out;
}

} // namespace statement

void
to_string(DString& start, Statement* s) {
    dstring::append(start, "Statement<", statement::strings[s->kind], ">");
}

} // namespace amu