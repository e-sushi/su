namespace amu {
namespace label {

global Label*
create() {
    Label* out = pool::add(compiler::instance.storage.labels);
    out->node.kind = node::label;
    return out;
}

global Label*
base(Label* l) {
    while(l->aliased) l = l->aliased;
    return l;
}

namespace table {

LabelTable
init(TNode* creator) {
    LabelTable out;
    out.last = 0;
    out.map = map::init<String, Label*>();
    out.owner = creator;
    return out;
}

FORCE_INLINE void
add(LabelTable* table, String id, Label* l) {
    map::add(table->map, id, l);
}

} // namespace table

Label*
resolve(TNode* n) {
    node::util::print_tree(n);
    switch(n->kind) {
        case node::label:     return (Label*)n;
        case node::place:     return ((Place*)n)->label;
        case node::structure: return ((Structure*)n)->label;
        case node::function:  return ((Function*)n)->label;
        case node::module:    return ((Module*)n)->label;
        case node::statement: {
            auto s = (Statement*)n;
            switch(s->kind) {
                case statement::label: return ((Label*)n->first_child);
            }
        } break;
        case node::type: return ((Type*)n)->label;
    }

    return 0;
}

} // namespace label

global void
to_string(DString& start, Label* l) {
    dstring::append(start, "Label<");

    if(!l || !l->node.start) return dstring::append(start, "unknown>");
    
    dstring::append(start, "'", l->node.start->raw, "' ");

    if(l->aliased)
        dstring::append(start, "(aka ", label::base(l)->node.start->raw, ") ");

    if(l->node.end) {
        dstring::append(start, code::name(l->node.start->code), ":", 
                l->node.start->l0, ",", l->node.start->c0, ":",
                l->node.end->l0, ",", l->node.end->c0,
            ">");
    } else {
        dstring::append(start, code::name(l->node.start->code), ":", 
                l->node.start->l0, ",", l->node.start->c0, ":",
                "?,?",
            ">");
    }

    
}

} // namespace amu
 
 