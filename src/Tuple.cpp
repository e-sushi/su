namespace amu::tuple {

global Tuple*
create() {
    Tuple* out = pool::add(compiler::instance.storage.tuples);
    node::init(&out->node);
    out->node.kind = node::tuple;
    return out;
}

} // namespace amu::tuple