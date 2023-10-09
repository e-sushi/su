namespace amu {

Tuple* Tuple::
create() {
    Tuple* out = pool::add(compiler::instance.storage.tuples);
    out->table = label::table::init(out->as<ASTNode>());
    return out;
}

void Tuple::
destroy() {
    map::deinit(table->map);
	pool::remove(compiler::instance.storage.label_tables, this->table);
    pool::remove(compiler::instance.storage.tuples, this); 
}

DString* Tuple::
display() { 
	DString* out = DString::create("(");
    for(ASTNode* n = first_child(); n; n = n->next()) {
        out->append(ScopedDeref(n->dump()).x, (n->next()? ", " : ""));
    }
    out->append(")");
    return out;
}

DString* Tuple::
dump() {
	auto out = DString::create("Tuple<type: ");
	if(type) {
		out->append(ScopedDeref(type->display()).x);
	} else {
		out->append("unknown");
	}
	
	out->append(">");
	return out;
}

} // namespace amu::tuple
