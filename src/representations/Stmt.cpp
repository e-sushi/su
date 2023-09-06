namespace amu {

Stmt* Stmt::
create() {
    Stmt* out = pool::add(compiler::instance.storage.statements);
    return out;
}

void Stmt::
destroy() {
    pool::remove(compiler::instance.storage.statements, this);
}

DString* Stmt::
display() { 
    return DString::create("Statement(TODO)");
}

DString* Stmt::
dump() {
    return DString::create("Stmt<", stmt::strings[kind], ">");
}

void
to_string(DString* start, Stmt* s) {
    start->append("Stmt<", stmt::strings[s->kind], ">");
}

} // namespace amu