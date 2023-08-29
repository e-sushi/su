namespace amu {

Tuple* Tuple::
create() {
    Tuple* out = pool::add(compiler::instance.storage.tuples);
    out->ASTNode::kind = ast::tuple;
    out->table = label::table::init(out->as<ASTNode>());
    return out;
}

String Tuple::
name() { 
    return debug_str();
}

DString Tuple::
debug_str() {
    DString out = dstring::init("(");
    for(ASTNode* n = first_child(); n; n = n->next()) {
        dstring::append(out, n->debug_str(), (n->next()? ", " : ""));
    }
    dstring::append(out, ")");
    return out;
}

void
to_string(DString& start, Tuple* t) {
    dstring::append(start, "Tuple<", tuple::strings[t->kind], ">");
}

} // namespace amu::tuple