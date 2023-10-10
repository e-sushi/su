namespace amu {

Label* Label::
create() {
    Label* out = compiler::instance.storage.labels.add();
    out->ASTNode::kind = ast::label;
    return out;
}

Label* Label::
base() {
    Label* scan = aliased;
    while(scan) scan = scan->aliased;
    return scan;
}

DString* Label::
display() {
    return DString::create(start->raw);
}

DString* Label::
dump() {
    return DString::create("Label<", start->raw, ">");
}

Type* Label::
resolve_type() {
    if(last_child()) return last_child<Expr>()->type;
    return 0;
}

VirtualLabel* VirtualLabel::
create(DString* display) {
    VirtualLabel* out = compiler::instance.storage.virtual_labels.add();
    out->id = display;
	out->virtual_token.kind = token::identifier;
	out->virtual_token.raw = display;
	out->virtual_token.hash = display->fin.hash();
	out->start = out->end = &out->virtual_token;
    return out;
}

DString* VirtualLabel::
display() {
    return id;
}

DString* VirtualLabel::
dump() {
    return DString::create("VirtualLabel<", id, ">");
}

LabelTable* LabelTable::
create() {
	LabelTable* out = compiler::instance.storage.label_tables.add();
    out->last = 0;
    out->map = Map<String, Label*>::create();
    return out;
}

void LabelTable::
add(String id, Label* l) {
	map.add(id, l);
}

Label* LabelTable::
search(u64 hash) {
	auto table = this;
	while(table) {
        auto [idx, found] = table->map.find(hash);
        if(found) {
			return table->map.values.read(idx);
		}
        table = table->last;
    }
    return 0;
}

global void
to_string(DString* start, Label* l) {
	start->append(ScopedDeref(l->display()).x->fin);
}

} // namespace amu
