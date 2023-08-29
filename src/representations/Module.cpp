namespace amu {

Module*
Module::create() {
    Module* out = pool::add(compiler::instance.storage.modules);
    out->ASTNode::kind = ast::entity;
    out->table = label::table::init((TNode*)out);
    return out;
}

String Module::
name() {
    return label->name();
}

DString Module::
debug_str() {
    return dstring::init("Module<", label->name(), ">");
}

void
to_string(DString& start, Module* m) {
    dstring::append(start, "Module<TODO>");
}

} // namespace amu 