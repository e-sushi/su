namespace amu {

void
to_string(DString*& start, Type* t) {
    if(!t) return start->append("Type<null>");
    start->append(node::util::print_tree<[](DString*& current, TNode* n) {
            current->append(((Type*)n)->name());
        }>((TNode*)t, false));
}











} // namespace amu
