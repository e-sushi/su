namespace amu {

Module*
Module::create() {
    Module* out = pool::add(compiler::instance.storage.modules);
    out->table = label::table::init(out->as<ASTNode>());
    return out;
}

DString* Module::
name() {
    return (label? label->name() : DString::create("anon module"));
}

DString* Module::
dump() {
    return DString::create("Module<", (label? ScopedDeref(label->name()).x : DString::create("anon")), ">");
}

void
to_string(DString* start, Module* m) {
    start->append("Module<TODO>");
}

} // namespace amu 