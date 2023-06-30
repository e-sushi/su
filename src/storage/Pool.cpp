
namespace amu {
namespace pool {

namespace internal {
    
template<typename T> global void
new_chunk(Pool<T>& pool) {
    const upt blocksize = sizeof(LNode) + sizeof(T);

    // allocate and insert the new chunk
    node::insert_before(&pool->chunk_root, 
        (LNode*)memory::allocate(sizeof(LNode)+pool->items_per_chunk*blocksize));

    // append the first free block to our free blocks list
    // node::insert_before(&pool->free_blocks, pool->chunk_root.prev + sizeof(LNode));

    // connect the remaining free blocks
    u8* blockstart = (u8*)(pool->chunk_root.prev + 1);
    forI(pool->items_per_chunk) {
        node::insert_before(&pool->free_blocks, (LNode*)(blockstart + i * blocksize));
    }
} // new_chunk

} // namespace internal

template<typename T> Pool<T>
init(spt n_per_chunk) {
    Pool<T> out;
    out.lock = mutex_init();
    out.items_per_chunk = n_per_chunk;

    node::init(&out.free_blocks);
    node::init(&out.chunk_root);
    node::init(&out.items);

    internal::new_chunk(&out);
  
    return out;
} // init

template<typename T> void
deinit(Pool<T>& pool) {
    shared_mutex_deinit(&pool->lock);
    
    for(LNode* n = pool->chunk_root->next; n != &pool->chunk_root; n = n->next) {
        memory::free(n);
    }
}

template<typename T> T*
add(Pool<T>& pool) {
    mutex_lock(&pool->lock);
    defer{mutex_unlock(&pool->lock);};

    if(pool->free_blocks.next == &pool->free_blocks) {
        internal::new_chunk(pool);
    }

    LNode* place = pool->free_blocks.next;

    T* out = (T*)(place + 1);
    node::remove(place);
    node::insert_before(&pool->items, place);
    return out;
}

template<typename T> void
add(Pool<T>& pool, T& val) {
    mutex_lock(&pool->lock);
    defer{mutex_unlock(&pool->lock);};

    T* place = add(pool);
    *place = val;
}

template<typename T> void
remove(Pool<T>& pool, T* ptr) {
    mutex_lock(&pool->lock);
    defer{mutex_unlock(&pool->lock);};

    LNode* header = (LNode*)((u8*)ptr - sizeof(LNode));
    node::remove(header);
    node::insert_before(&pool->free_blocks, header);
}

template<typename T> Iterator<T>
iterator(Pool<T>& pool) {
    Iterator<T> out;
    out.pool = &pool;
    out.current = pool->items.next;
    return out;
}

template<typename T> T*
next(Iterator<T>* iter) {
    mutex_lock(&iter->pool->lock);
    if(iter->current == &iter->pool->items) return 0;
    T* out = (T*)(iter->current + 1);
    iter->current = iter->current->next;
    mutex_unlock(&iter->pool->lock);
    return out;
}

template<typename T> T*
prev(Iterator<T>* iter) {
    mutex_lock(&iter->pool->lock);
    if(iter->current == &iter->pool->items) return 0;
    T* out = (T*)(iter->current + 1);
    iter->current = iter->current->prev;
    mutex_unlock(&iter->pool->lock);
    return out;
}

} // namespace pool
} // namespace amu