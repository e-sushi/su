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
 
 