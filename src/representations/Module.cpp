namespace amu {

Module*
Module::create() {
    Module* out = pool::add(compiler::instance.storage.modules);
    out->table = label::table::init(out->as<ASTNode>());
    return out;
}

String Module::
name() {
    return (label? label->name() : "anon module");
}

DString Module::
debug_str() {
    return dstring::init("Module<", (label? label->name() : "anon"), ">");
}

void
to_string(DString& start, Module* m) {
    dstring::append(start, "Module<TODO>");
}

} // namespace amu 