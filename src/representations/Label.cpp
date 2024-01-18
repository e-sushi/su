#include "Label.h"
#include "basic/Allocator.h"
#include "storage/DString.h"
#include "representations/Expr.h"
#include "representations/Token.h"

namespace amu {

Label* Label::
create(Allocator* allocator) {
    return new (allocator->allocate(sizeof(Label))) Label;
}

Label* Label::
base() {
    Label* scan = aliased;
    while(scan) scan = scan->aliased;
    return scan;
}

DString Label::
display() {
    return DString(start->raw);
}

DString Label::
dump() {
    return DString("Label<", start->raw, ">");
}

Type* Label::
resolve_type() {
    if(last_child()) return last_child<Expr>()->type;
    return 0;
}

LabelTable* LabelTable::
create(Allocator* allocator) {
	auto out = new (allocator->allocate(sizeof(LabelTable))) LabelTable;
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
        if(found)
			return table->map.values.read(idx);
        table = table->last;
    }
    return 0;
}

void 
to_string(DString& start, Label& l) {
	start.append(l.display());
}

} // namespace amu
