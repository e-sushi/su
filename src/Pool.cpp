
namespace amu {
namespace pool {

namespace internal {
    
template<typename T> global void
new_chunk(Pool<T>* pool) {
    const upt blocksize = sizeof(LNode) + sizeof(T);

    node::insert_before(&pool->chunk_root, 
        (LNode*)util::allocate(sizeof(LNode)+pool->items_per_chunk*blocksize));

    node::insert_before(&pool->free_blocks, pool->chunk_root.prev + sizeof(LNode));

    forI(pool->items_per_chunk) {
        node::insert_before(&pool->free_blocks, sizeof(LNode) + i * blocksize);
    }
} // new_chunk

} // namespace internal

template<typename T> Pool<T>
init(spt n_per_chunk) {
    Pool<T> out;
    out.lock = deshi::mutex_init();
    out.items_per_chunk = n_per_chunk;

    node::init(out.free_blocks);
    node::init(out.chunk_root);

    internal::new_chunk(&out);
  
    return out;
} // init

template<typename T> void
deinit(Pool<T>* pool) {
    deshi::mutex_deinit(&pool->lock);
    
    for(LNode* n = pool->chunk_root->next; n != &pool->chunk_root; n = n->next) {
        util::free(n);
    }
}

template<typename T> T*
add(Pool<T>* pool) {
    deshi::mutex_lock(&pool->lock);
    defer{deshi::mutex_unlock(&pool->lock);};

    if(pool->free_blocks.next == &pool->free_blocks) {
        internal::new_chunk(pool);
    }

    T* out = pool->free_blocks.next + sizeof(LNode);
    node::remove(out->free_block.next);

    return out;

}

template<typename T> void
remove(Pool<T>* pool, T* ptr) {
    deshi::mutex_lock(&pool->lock);
    defer{deshi::mutex_unlock(&pool->lock);};

    LNode* header = ptr - sizeof(LNode);
    node::insert_before(&pool->free_blocks, header);
}

} // namespace pool
} // namespace amu