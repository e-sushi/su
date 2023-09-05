namespace amu {

Label* Label::
create() {
    Label* out = pool::add(compiler::instance.storage.labels);
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
name() {
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
create(DString* name) {
    VirtualLabel* out = pool::add(compiler::instance.storage.virtual_labels);
    out->id = name;
    return out;
}

DString* VirtualLabel::
name() {
    return id;
}

DString* VirtualLabel::
debug_str() {
    return DString::create("VirtualLabel<", id, ">");
}

namespace label {
namespace table {

LabelTable
init(ASTNode* creator) {
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

Label*
search(LabelTable* table, u64 hashed_id) {
    while(table) {
        auto [idx, found] = map::find(table->map, hashed_id);
        if(found) return array::read(table->map.values, idx);
        table = table->last;
    }
    return 0;
}

} // namespace table

Label*
resolve(TNode* n) {
    // node::util::print_tree(n);
    // switch(n->kind) {
    //     case node::label:     return (Label*)n;
    //     case node::place:     return ((Place*)n)->label;
    //     case node::structure: return ((Structure*)n)->label;
    //     case node::function:  return ((Function*)n)->label;
    //     case node::module:    return ((Module*)n)->label;
    //     case node::statement: {
    //         auto s = (Statement*)n;
    //         switch(s->kind) {
    //             case statement::label: return ((Label*)n->first_child);
    //         }
    //     } break;
    //     case node::type: return ((Type*)n)->label;
    // }

    return 0;
}

void
display(DString*& current, Label* l, Formatting format, b32 allow_color) {
    // auto append_label = [&](Label* l) {
    //     DString* temp = DString::create(l->node.start->raw);
    //     if(allow_color) {
    //         util::wrap_color(temp, format.col);
    //     }
    //     dstring::prepend(temp, format.prefix);
    //     current->append(temp, format.suffix);
    //     dstring::deinit(temp);
    // };

    // append_label(l);

    // if(l->aliased && !format.no_aka) {
    //     current->append(" (aka ");
    //     if(format.full_aka) {
    //         Label* step = l->aliased;
    //         while(1) {
    //             append_label(step);
    //             step = step->aliased;
    //             if(!step) break;
    //             current->append(" aka ");
    //         }
    //     } else {
    //         Label* step = l->aliased;
    //         while(step->aliased) step = step->aliased;
    //         append_label(step);
    //     }
    //     current->append(')');
    // }
}

} // namespace label

global void
to_string(DString*& start, Label* l) {
    // start->append("Label<");

    // if(!l || !l->node.start) return start->append("unknown>");
    
    // start->append("'", l->node.start->raw, "'");

    // if(l->aliased)
    //     start->append(" (aka ", label::base(l)->node.start->raw, ")");
 
    // // TODO(sushi) add option to enable this somehow 
    // // if(l->node.end) {
    // //     start->append(l->node.start->code->name, ":", 
    // //             l->node.start->l0, ",", l->node.start->c0, ":",
    // //             l->node.end->l0, ",", l->node.end->c0,
    // //         ">");
    // // } else {
    // //     start->append(l->node.start->code->name, ":", 
    // //             l->node.start->l0, ",", l->node.start->c0, ":",
    // //             "?,?",
    // //         ">");
    // // }

    // start->append(">");
    
}

} // namespace amu
 
 