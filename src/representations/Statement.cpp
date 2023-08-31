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

String Stmt::
name() { 
    return "Statement(TODO)";
}

DString Stmt::
debug_str() {
    return dstring::init("Stmt<", stmt::strings[kind], ">");
}

void
to_string(DString& start, Stmt* s) {
    dstring::append(start, "Stmt<", stmt::strings[s->kind], ">");
}

} // namespace amu