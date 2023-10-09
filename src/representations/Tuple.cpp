namespace amu {

Tuple* Tuple::
create() {
    Tuple* out = compiler::instance.storage.tuples.add();
    out->table = LabelTable::create();
    return out;
}

void Tuple::
destroy() {
    map::deinit(table->map);
	compiler::instance.storage.label_tables.remove(this->table);
    compiler::instance.storage.tuples.remove(this); 
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
