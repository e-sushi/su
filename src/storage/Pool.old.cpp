
namespace amu {

namespace pool::internal {
    
template<typename T> global void
new_chunk(Pool<T>* pool) {
    const upt blocksize = sizeof(LNode) + sizeof(T);

    // allocate and insert the new chunk
    node::insert_before(pool->chunk_root, 
        (LNode*)memory::allocate(sizeof(LNode)+pool->items_per_chunk*blocksize));

    // connect the remaining free blocks
    u8* blockstart = (u8*)(pool->chunk_root->prev + 1);
    forI(pool->items_per_chunk) {
        node::insert_before(pool->free_blocks, (LNode*)(blockstart + i * blocksize));
    }
} // new_chunk

} // namespace pool::internal


template<typename T> Pool<T> Pool<T>::
create(spt n_per_chunk) {
	Pool<T> out;
	out.items_per_chunk = n_per_chunk;

	const upt blocksize = sizeof(LNode) + sizeof(T);

	auto initchunk = (LNode*)memory::allocate(4*sizeof(LNode) + out.items_per_chunk*blocksize);
	out.free_blocks = initchunk;
	out.chunk_root = initchunk + 1;
	out.items = initchunk + 2;

	node::init(out.free_blocks);
	node::init(out.chunk_root);
	node::init(out.items);

	node::insert_before(out.chunk_root, initchunk + 3);

	auto blockstart = (u8*)(out.chunk_root->prev + 1);
	forI(out.items_per_chunk) {
		node::insert_before(out.free_blocks, (LNode*)(blockstart + i * blocksize));
	}

	out.m = new Mutex();

	return out;
}

template<typename T> void Pool<T>::
destroy() {
    // NOTE(sushi) skip the first chunk cause it's not the same as anything allocated
    //             after it, see init
    // TODO(sushi) this is stupid so fix it eventually
    for(LNode* n = chunk_root->next->next; n != chunk_root; n = n->next) {
        memory::free(n);
    }

    memory::free(free_blocks);

	delete m;
}

template<typename T> T* Pool<T>::
add() {
	this->m->lock();
	defer { this->m->unlock(); };
    if(this->free_blocks->next == this->free_blocks) {
		pool::internal::new_chunk(this);
    }

    LNode* place = this->free_blocks->next;

    // vtables require you to allocate with new
    T* out = new (place+1) T();
    node::remove(place);
    node::insert_before(this->items, place);
    return out;
}

template<typename T> T* Pool<T>::
add(const T& val) {
    T* place = add();
    *place = val;
    return place;
}

template<typename T> void Pool<T>::
remove(T* ptr) {
	this->m->lock();
	defer { this->m->unlock(); }; 
    LNode* header = (LNode*)((u8*)ptr - sizeof(LNode));
    node::remove(header);
    node::insert_before(this->free_blocks, header);
}



template<typename T> Pool<T>
init(spt n_per_chunk) {
    Pool<T> out;
    out.items_per_chunk = n_per_chunk;

    const upt blocksize = sizeof(LNode) + sizeof(T);

    // NOTE(sushi) allocating 4 LNodes because we store 3 as roots for free blocks, chunks, and items
    LNode* initchunk = (LNode*)memory::allocate(4*sizeof(LNode)+out.items_per_chunk*blocksize);
    out.free_blocks = initchunk;
    out.chunk_root = initchunk + 1;
    out.items = initchunk + 2;

    node::init(out.free_blocks);
    node::init(out.chunk_root);
    node::init(out.items);

    // insert new chunk
    node::insert_before(out.chunk_root, initchunk + 3);

    // append the first free block to our free blocks list
    // node::insert_before(&out.free_blocks, out.chunk_root.prev + sizeof(LNode));

    // connect the remaining free blocks
    u8* blockstart = (u8*)(out.chunk_root->prev + 1);
    forI(out.items_per_chunk) {
        node::insert_before(out.free_blocks, (LNode*)(blockstart + i * blocksize));
    }

	out.m = new Mutex();
  
    return out;
} // init

namespace pool {

template<typename T> Iterator<T>
iterator(Pool<T>& pool) {
    Iterator<T> out;
    out.pool = &pool;
    out.current = pool.items->next;
    return out;
}

template<typename T> T*
next(Iterator<T>& iter) {
    if(iter.current == iter.pool->items) return 0;
    T* out = (T*)(iter.current + 1);
    iter.current = iter.current->next;
    return out;
}

template<typename T> T*
prev(Iterator<T>& iter) {
    if(iter.current == iter.pool->items) return 0;
    T* out = (T*)(iter.current + 1);
    iter.current = iter.current->prev;
    return out;
}

} // namespace pool
} // namespace amu
