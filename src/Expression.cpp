namespace amu::expression {

global Expression*
create() {
    Expression* out = pool::add(compiler::instance.storage.expressions);
    node::init(&out->node);
    out->node.kind = node::expression;
    return out;
}

} // namespace amu::expression