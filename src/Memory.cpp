namespace amu {

Allocator _amu_allocator{
    memory::allocate,
    memory::free,
    memory::reallocate,
};
Allocator* amu_allocator = &_amu_allocator;

namespace memory {

mutex global_lock;

void* 
allocate(upt size) {
    mutex_lock(&compiler::instance.deshi_mem_lock);
    void* out = memalloc(size);
    mutex_unlock(&compiler::instance.deshi_mem_lock);
    return out;
}

void* 
reallocate(void* ptr, upt size) {
    mutex_lock(&compiler::instance.deshi_mem_lock);
    void* out = memrealloc(ptr, size);
    mutex_unlock(&compiler::instance.deshi_mem_lock);
    return out;
}

void 
free(void* ptr) {
    mutex_lock(&compiler::instance.deshi_mem_lock);
    memzfree(ptr);
    mutex_unlock(&compiler::instance.deshi_mem_lock);
}

} // namespace memory
} // namespace amu