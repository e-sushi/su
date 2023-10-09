#include "representations/Label.h"
namespace amu {

void NodeStack::
push(ASTNode* n) {
	messenger::qdebug(code, String("pushing node "), ScopedDeref(n->display()).x->fin);
	stack.push(current);
	current = n;
}

ASTNode* NodeStack::
pop() {
	ASTNode* save = current;
	current = stack.pop();
	messenger::qdebug(code, String("popped node "), ScopedDeref(save->display()).x->fin);
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
		auto l = label::table::search(table, hash);
		if(l) return l;
		table = table->last;
	}
	return 0;
}

Label* TableStack::
search_local(u64 hash) {
	return label::table::search(current, hash);
}

} // namespace amu
