namespace amu {


namespace internal {

template<void (*callback)(DString&,ASTNode*)> void
print_tree_recursive(DString& current, ASTNode* n, b32 newlines) {
    // TODO(sushi) this can't be static when things become multithreaded
    persist u32 layers = 0;
	if(newlines) forI(layers) dstring::append(current, "  ");

    if(n->child_count) dstring::append(current, "(");

	callback(current, n);
	
	layers++;
	for(ASTNode* c = n->first_child(); c; c = c->next()) {
		if(newlines) dstring::append(current, "\n");
		else dstring::append(current, " ");
		print_tree_recursive<callback>(current, c, newlines);
	}
	layers--;

	if(n->child_count) {
		dstring::append(current, ")");
	} 
} 

} // namespace internal

template<void (*callback)(DString&,ASTNode*)> DString ASTNode::
print_tree(b32 newlines) {
    DString out = dstring::init();
    internal::print_tree_recursive<callback>(out, this, newlines);
    return out;
} 

DString ASTNode::
print_tree(b32 newlines) {
    return print_tree<[](DString& s, ASTNode* n) { dstring::append(s, n->debug_str()); }>(newlines);
}

} // namespace amu