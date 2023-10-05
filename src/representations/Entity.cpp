namespace amu {

b32 Entity::
ensure_processed_to(code::level level) {
	// if the Code this entity belongs to has been processed
	// to or beyond the given level, then we don't need to 
	// worry about this
	if(code->level >= level) return true;

	// otherwise, we have to determine if the Code we're pointing
	// to directly represents this Entity.
	// if not, we need to segment this Entity into its own
	// Code object represents this Entity.
	
	

}

void
to_string(DString* start, Type* t) {
    if(!t) return start->append("Type<null>");
    start->append(node::util::print_tree<[](DString* current, TNode* n) {
            current->append(((Type*)n)->display());
        }>((TNode*)t, false));
}

} // namespace amu
