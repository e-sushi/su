namespace amu {
namespace node {

global inline void
init(LNode* node) {
    //node->lock = deshi::mutex_init();
    node->next = node;
    node->prev = node;
}

global inline void 
deinint(LNode* node){
    //deshi::mutex_deinit(&node->lock);
}

global inline void
init(TNode* node) {
    //node->lock = deshi::mutex_init();
    *node = {};
}

global inline void
deinit(TNode* node) {
    //deshi::mutex_deinit(&node->lock);
}

global inline void
insert_after(LNode* target, LNode* node) {
    // deshi::mutex_lock(&node->lock);
    // deshi::mutex_lock(&target->lock);

    node->next = target->next;
    node->prev = target;
    node->next->prev = node;
    target->next = node;

    // deshi::mutex_unlock(&node->lock);
    // deshi::mutex_unlock(&target->lock);
}

global inline void 
insert_before(LNode* target, LNode* node) {
    // deshi::mutex_lock(&node->lock);
    // deshi::mutex_lock(&target->lock);

    node->prev = target->prev;
    node->next = target;
    node->prev->next = node;
    target->prev = node;

    // deshi::mutex_unlock(&node->lock);
    // deshi::mutex_unlock(&target->lock);
}


global inline void
insert_after(TNode* target, TNode* node) {
    // deshi::mutex_lock(&node->lock);
    // deshi::mutex_lock(&target->lock);

    if (target->next) target->next->prev = node;
	node->next = target->next;
	node->prev = target;
	target->next = node;

    // deshi::mutex_unlock(&node->lock);
    // deshi::mutex_unlock(&target->lock);
}

global inline void
insert_before(TNode* target, TNode* node) {
    // deshi::mutex_lock(&node->lock);
    // deshi::mutex_lock(&target->lock);

	if (target->prev) target->prev->next = node;
	node->prev = target->prev;
	node->next = target;
	target->prev = node;

    // deshi::mutex_unlock(&node->lock);
    // deshi::mutex_unlock(&target->lock);
}

global inline void 
remove_horizontally(TNode* node){
    //deshi::mutex_lock(&node->lock);

    if (node->next) node->next->prev = node->prev;
	if (node->prev) node->prev->next = node->next;
	node->next = node->prev = 0;

    //deshi::mutex_unlock(&node->lock);
}

// inserts the given 'node' as the last child of the node 'parent'
global inline void
insert_last(TNode* parent, TNode* child) {
    // deshi::mutex_lock(&child->lock);
    // deshi::mutex_lock(&parent->lock);
    // defer {
    //     deshi::mutex_unlock(&child->lock);
    //     deshi::mutex_unlock(&parent->lock);
    // };

    if (parent == 0) { child->parent = 0; return; }
	if(parent==child){DebugBreakpoint;}
	
	child->parent = parent;
	if (parent->first_child) {
		insert_after(parent->last_child, child);
		parent->last_child = child;
	}
	else {
		parent->first_child = child;
		parent->last_child = child;
	}
	parent->child_count++;
}

// inserts the given 'node' as the first child of the node 'parent'
global inline void 
insert_first(TNode* parent, TNode* child) {
    // deshi::mutex_lock(&child->lock);
    // deshi::mutex_lock(&parent->lock);
    // defer{
    //     deshi::mutex_unlock(&child->lock);
    //     deshi::mutex_unlock(&parent->lock);
    // };

    if (parent == 0) { child->parent = 0; return; }
	
	child->parent = parent;
	if (parent->first_child) {
		insert_before(parent->first_child, child);
		parent->first_child = child;
	}
	else {
		parent->first_child = child;
		parent->last_child = child;
	}
	parent->child_count++;
}

global inline void
change_parent(TNode* new_parent, TNode* node) {
    // deshi::mutex_lock(&node->lock);
    // deshi::mutex_lock(&new_parent->lock);

    if (node->parent) {
		if (node->parent->child_count > 1) {
			if (node == node->parent->first_child) node->parent->first_child = node->next;
			if (node == node->parent->last_child)  node->parent->last_child = node->prev;
		}
		else {
			Assert(node == node->parent->first_child && node == node->parent->last_child, "if node is the only child node, it should be both the first and last child nodes");
			node->parent->first_child = 0;
			node->parent->last_child = 0;
		}
		node->parent->child_count--;
	}
	
	//remove self horizontally
	remove_horizontally(node);
	
	//add self to new parent
	insert_last(new_parent, node);

    // deshi::mutex_unlock(&node->lock);
    // deshi::mutex_unlock(&new_parent->lock);
}

// rearrages the parent of 'node' such that 'node' is the first child, 
// moving all other children nodes to the right
global inline void
move_to_parent_first(TNode* node)  {
    // deshi::mutex_lock(&node->lock);
    // defer{deshi::mutex_unlock(&node->lock);};

	if(!node->parent) return;
	
	TNode* parent = node->parent;
	if(parent->first_child == node) return;
	if(parent->last_child == node) parent->last_child = node->prev;
	
	remove_horizontally(node);
	node->next = parent->first_child;
	parent->first_child->prev = node;
	parent->first_child = node;
}

// rearrages the parent of 'node' such that 'node' is the last child, 
// moving all other children nodes to the left
global inline void
move_to_parent_last(TNode* node)  {
    // deshi::mutex_lock(&node->lock);
    // defer{deshi::mutex_unlock(&node->lock);};

    TNode* parent = node->parent;
	if(parent->last_child == node) return;
	if(parent->first_child == node){
        parent->first_child = node->next;
    } 
	
	remove_horizontally(node);
	node->prev = parent->last_child;
	parent->last_child->next = node;
	parent->last_child = node;

}

global inline void 
remove(LNode* node) {
    //deshi::mutex_lock(&node->lock);

    node->next->prev=node->prev;
    node->prev->next=node->next;

    //deshi::mutex_unlock(&node->lock);
}

global inline void
remove(TNode* node) {
    //deshi::mutex_lock(&node->lock);

    //add children to parent (and remove self from children)
	for(TNode* it = node->first_child; it; it = it->next) {
		TNode* next = it->next;
		change_parent(node->parent, it);
	}
	
	//remove self from parent
	if (node->parent) {
		if (node->parent->child_count > 1) {
			if (node == node->parent->first_child) node->parent->first_child = node->next;
			if (node == node->parent->last_child)  node->parent->last_child = node->prev;
		}
		else {
			Assert(node == node->parent->first_child && node == node->parent->last_child, "if node is the only child node, it should be both the first and last child nodes");
			node->parent->first_child = 0;
			node->parent->last_child = 0;
		}
		node->parent->child_count--;
	}
	node->parent = 0;
	
	//remove self horizontally
	remove_horizontally(node);

    //deshi::mutex_unlock(&node->lock);
}

} // namespace node
} // namespace amu