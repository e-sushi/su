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
    return out;
}

DString* VirtualLabel::
display() {
    return id->ref();
}

DString* VirtualLabel::
dump() {
    return DString::create("VirtualLabel<", id, ">");
}

LabelTable* LabelTable::
create() {
	LabelTable* out = compiler::instance.storage.label_tables.add();
    out->last = 0;
    out->map = map::init<String, Label*>();
    return out;
}

void LabelTable::
add(String id, Label* l) {
	map::add(map, id, l);
}

Label* LabelTable::
search(u64 hash) {
	auto table = this;
	while(table) {
        auto [idx, found] = map::find(table->map, hash);
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
