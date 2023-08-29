namespace amu {

Stmt* Stmt::
create() {
    Stmt* out = pool::add(compiler::instance.storage.statements);
    return out;
}

String Stmt::
name() { 
    return "Statement(TODO)";
}

DString Stmt::
debug_str() {
    return dstring::init("Stmt<", statement::strings[kind], ">");
}

void
to_string(DString& start, Stmt* s) {
    dstring::append(start, "Stmt<", statement::strings[s->kind], ">");
}

} // namespace amu