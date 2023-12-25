namespace amu {

Stmt* Stmt::
create() {
    Stmt* out = compiler::instance.storage.statements.add();
    return out;
}

void Stmt::
destroy() {
    compiler::instance.storage.statements.remove(this);
}

DString* Stmt::
display() { 
    return DString::create("Statement(TODO)");
}

DString* Stmt::
dump() {
    return DString::create("Stmt<", stmt::kind_strings[kind], ">");
}

void
to_string(DString* start, Stmt* s) {
    start->append("Stmt<", stmt::kind_strings[s->kind], ">");
}

} // namespace amu
