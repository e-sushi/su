namespace amu {

Module*
Module::create() {
    Module* out = compiler::instance.storage.modules.add();
    out->table = LabelTable::create();
    return out;
}

DString* Module::
display() {
    return (label? label->display() : DString::create("anon module"));
}

DString* Module::
dump() {
    return DString::create("Module<", (label? ScopedDeref(label->display()).x : DString::create("anon")), ">");
}

void
to_string(DString* start, Module* m) {
    start->append("Module<TODO>");
}

} // namespace amu 
