namespace amu {

void NodeStack::
push(ASTNode* n) {
	stack.push(current);
	current = n;
}

ASTNode* NodeStack::
pop() {
	ASTNode* save = current;
	current = stack.pop();
	return save;
}

void TableStack::
push(LabelTable* l) {
	stack.push(current);
	current = l;
}

void TableStack::
pop() {
	current = stack.pop();
}

void TableStack::
add(String id, Label* l) {
	map::add(current->map, id, l);
}

Label* TableStack::
search(u64 hash) {
	LabelTable* table = current;
	while(table) {
		auto [idx, found] = map::find(table->map, hash);
		if(found) return table->map.values.read(idx);
		table = table->last;
	}
	return 0;
}

Label* TableStack::
search_local(u64 hash) {
	auto [idx, found] = map::find(current->map, hash);
	if(found) return current->map.values.read(idx);
	return 0;
}

} // namespace amu
