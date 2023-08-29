namespace amu {

Module*
Module::create() {
    Module* out = pool::add(compiler::instance.storage.modules);
    node::init(&out->node);
    out->node.kind = node::module;
    out->table = label::table::init((TNode*)out);
    return out;
}

void
to_string(DString& start, Module* m) {
    dstring::append(start, "Module<TODO>");
}

} // namespace amu 