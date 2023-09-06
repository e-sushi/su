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

DString* Tuple::
display() { 
    return dump();
}

DString* Tuple::
dump() {
    DString* out = DString::create("(");
    for(ASTNode* n = first_child(); n; n = n->next()) {
        out->append(ScopedDeref(n->dump()).x, (n->next()? ", " : ""));
    }
    out->append(")");
    return out;
}

} // namespace amu::tuple