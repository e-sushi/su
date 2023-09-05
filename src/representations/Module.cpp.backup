namespace amu {

Module*
Module::create() {
    Module* out = pool::add(compiler::instance.storage.modules);
    out->table = label::table::init(out->as<ASTNode>());
    return out;
}

DString Module::
name() {
    return (label? label->name() : dstring::init("anon module"));
}

DString Module::
dump() {
    return dstring::init("Module<", (label? label->name() : dstring::init("anon")), ">");
}

void
to_string(DString& start, Module* m) {
    dstring::append(start, "Module<TODO>");
}

} // namespace amu 