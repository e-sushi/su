namespace amu {

void
to_string(DString& start, Type* t) {
    if(!t) return dstring::append(start, "Type<null>");
    dstring::append(start, 
        node::util::print_tree<[](DString& current, TNode* n) {
            dstring::append(current, ((Type*)n)->name());
        }>((TNode*)t, false));
}











} // namespace amu
