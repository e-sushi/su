namespace amu {
namespace node {

global inline void
init(LNode* node) {
    node->lock = deshi::mutex_init();
    node->next = node;
    node->prev = node;
}

global inline void
init(TNode* node) {
    node->lock = deshi::mutex_init();
    *node = {};
}

global inline void
insert_after(LNode* target, LNode* node) {
    deshi::mutex_lock(&node->lock);
    deshi::mutex_lock(&target->lock);
    defer{
        deshi::mutex_unlock(&node->lock);
        deshi::mutex_unlock(&target->lock);
    };

    node->next = target->next;
    node->prev = target;
    node->next->prev = node;
    target->next = node;
}

global inline void 
insert_before(LNode* target, LNode* node) {
    deshi::mutex_lock(&node->lock);
    deshi::mutex_lock(&target->lock);
    defer{
        deshi::mutex_unlock(&node->lock);
        deshi::mutex_unlock(&target->lock);
    };

    
}

} // namespace node
} // namespace amu