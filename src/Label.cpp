namespace amu::label {

global Label*
create() {
    Label* out = pool::add(compiler::instance.storage.labels);
    out->node.kind = node::label;
    return out;
}

} // namespace amu::label
 
 