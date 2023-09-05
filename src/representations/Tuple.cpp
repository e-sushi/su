namespace amu {

Tuple* Tuple::
create() {
    Tuple* out = pool::add(compiler::instance.storage.tuples);
    out->ASTNode::kind = ast::tuple;
    out->table = label::table::init(out->as<ASTNode>());
    return out;
}

void Tuple::
destroy() {
    map::deinit(table.map);
    pool::remove(compiler::instance.storage.tuples, this); 
}

DString Tuple::
name() { 
    return dump();
}

DString Tuple::
dump() {
    DString out = dstring::init("(");
    for(ASTNode* n = first_child(); n; n = n->next()) {
        dstring::append(out, n->dump(), (n->next()? ", " : ""));
    }
    dstring::append(out, ")");
    return out;
}

} // namespace amu::tuple