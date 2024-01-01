#include "AST.h"
#include "Token.h"
#include "Common.h"
#include "Code.h"
#include "storage/DString.h"

namespace amu {

void ASTNode::
insert_after(ASTNode* x) {
	if(next()) next()->prev() = x;
	x->next() = next();
	x->prev() = this;
	next() = x;
}

void ASTNode::
insert_before(ASTNode* x) {
	if(prev()) prev()->next() = x;
	x->next() = this;
	x->prev()->next() = x;
	prev() = x;
}

void ASTNode::
remove_horizontally() {
	if(next()) next()->prev() = prev(); 
	if(prev()) prev()->next() = next();
	next() = prev() = 0;
}

void ASTNode::
insert_last(ASTNode* x) {
	x->parent() = this;
	if(first_child()) {
		last_child()->insert_after(x);
		last_child() = x;
	} else {
		first_child() = x;
		last_child() = x;
	}
	child_count++;
}

void ASTNode::
insert_first(ASTNode* x) {
	x->parent() = this;
	if(first_child()) {
		first_child()->insert_before(x);
		first_child() = x;
	} else {
		first_child() =
		last_child() = x;
	}
	child_count++;
}

void ASTNode::
change_parent(ASTNode* new_parent) {
	if(parent()) {
		if(parent()->child_count > 1) {
			if(this == parent()->first_child()) parent()->first_child() = next();
			if(this == parent()->last_child()) parent()->last_child() = prev();
		} else {
			parent()->first_child() = 
			parent()->last_child() = 0;
		}
		parent()->child_count--;
	}

	remove_horizontally();

	new_parent->insert_last(this);
}

void ASTNode::
insert_above(ASTNode* below) {
	if(!parent()) {
		below->change_parent(this);
		return;
	}

	auto copy = below->link;
	copy.parent->child_count++;	
	below->change_parent(0);

	if(copy.parent) {
		parent() = copy.parent;

		if(copy.next && copy.prev) {
			copy.prev->insert_after(this);
		} else if(copy.next && !copy.prev) {
			copy.parent->first_child() = this;
		} else if(!copy.next && copy.prev) {
			copy.parent->last_child() = this;
		} else {
			copy.parent->first_child() = copy.parent->last_child() = this;
		}
	}

	change_parent(below);
}

void ASTNode::
move_to_parent_first() {
	if(!parent()) return;

	if(parent()->first_child() == this) return;
	if(parent()->last_child() == this) parent()->last_child() = prev();

	remove_horizontally();
	next() = parent()->first_child();
	parent()->first_child()->prev() = this;
	parent()->first_child() = this;
}

void ASTNode::
move_to_parent_last() {
	if(!parent()) return;

	if(parent()->last_child() == this) return;
	if(parent()->first_child() == this) parent()->first_child() = next();

	remove_horizontally();
	prev() = parent()->last_child();
	parent()->last_child()->next() = this;
	parent()->last_child() = this;
}

void ASTNode::
remove() {
	for(auto n = first_child(); n; n = n->next()) {
		n->change_parent(parent());
	}

	if(parent()) {
		if(parent()->child_count > 1) {
			if(this == parent()->first_child()) parent()->first_child() = next();
			if(this == parent()->last_child()) parent()->last_child() = prev();
		} else {
			parent()->first_child() =
			parent()->last_child() = 0;
		}
		parent()->child_count--;
	}
	parent() = 0;

	remove_horizontally();
}

void ASTNode::
replace(ASTNode* with) {
	with->link = link;

	for(auto n = first_child(); n; n = n->next()) {
		n->parent() = with;
	}

	if(parent()) {
		if(parent()->first_child() == this) parent()->first_child() = with;
		if(parent()->last_child() == this) parent()->last_child() = with;
	}

	if(prev()) prev()->next() = with;
	if(next()) next()->prev() = with;
}



template<void (*callback)(DString&,ASTNode*)> void
print_tree_recursive(DString& current, ASTNode* n, b32 newlines) {
    // TODO(sushi) this can't be static when things become multithreaded
    static thread_local u32 layers = 0;
	if(newlines) forI(layers) current.append("  ");

    if(n->child_count) current.append("(");

	callback(current, n);
	
	layers++;
	for(ASTNode* c = n->first_child(); c; c = c->next()) {
		if(newlines) current.append("\n");
		else current.append(" ");
		print_tree_recursive<callback>(current, c, newlines);
	}
	layers--;

	if(n->child_count) {
		current.append(")");
	} 
} 

template<void (*callback)(DString&,ASTNode*)> DString ASTNode::
print_tree(b32 newlines) {
    DString out;
    print_tree_recursive<callback>(out, this, newlines);
    return out;
} 

DString ASTNode::
print_tree(b32 newlines) {
    return print_tree<[](DString& s, ASTNode* n) { s.append(n->dump()); }>(newlines);
}

String ASTNode::
first_line(b32 line_numbers, b32 remove_leading_whitespace) {
	if(!start) return "ASTNode::first_line called, but start is null";
	
	u8* scan_left = start->raw.str;

	while(scan_left != start->code->raw.str && *scan_left != '\n') 
		scan_left--;

	// dont want to include a newline at the start
	if(scan_left != start->code->raw.str) scan_left++;

	u8* scan_right = start->raw.str;

	while(scan_right != start->code->raw.str + start->code->raw.count && *scan_right != '\n')
		scan_right++;

	String out = String{scan_left, scan_right-scan_left};

	if(remove_leading_whitespace) 
		out = out.skip_whitespace();

	if(line_numbers)
		out = DString(start->l0, ": ", out);

	return out;
}

String ASTNode::
lines() {
	if(!start || !end) return "ASTNode::lines called, but either start or end are null";

	u8* scan_left = start->raw.str;

	while(scan_left != start->code->raw.str && *scan_left != '\n') 
		scan_left--;

	u8* scan_right = end->raw.str;

	while(scan_right != start->code->raw.str + start->code->raw.count && *scan_right != '\n')
		scan_right++;

	return String{scan_left, scan_right-scan_left};
}

DString ASTNode::
underline() {
	if(!start || !end) return DString("ASTNode::underline called, but either start or end are null");

	String line = first_line();

	// TODO(sushi) handle nodes spanning multiple lines in a better way 
	u64 start_depth = start->raw.str - line.str;
	u64 end_depth = util::Min(end->raw.str + end->raw.count - line.str, line.count);

	DString out = DString(line, "\n");

	// TODO(sushi) this does not handle non u8 codepoints
	forI(start_depth) {
		out.append(" ");
	}

	forI(end_depth - start_depth) {
		out.append("~");
	}

	return out;
}

} // namespace amu
