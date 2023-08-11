namespace amu::statement {

global Statement*
create() {
    Statement* out = pool::add(compiler::instance.storage.statements);
    node::init(&out->node);
    out->node.kind = node::statement;
    return out;
}

} // namespace amu::statement