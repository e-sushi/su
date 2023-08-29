namespace amu {

Structure*
Structure::create() {
    Structure* out = pool::add(compiler::instance.storage.structures);
    out->table = label::table::init(0);
    return out;
}






void
to_string(DString& start, Structure* s) {
    dstring::append(start, "Structure");
}

} // namespace amu