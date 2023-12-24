#include "Common.h"
#include "Memory.h"

namespace amu {

Memory memory;

void* Memory:: 
allocate(upt size) {
    void* out = malloc(size);
    zero(out, size);
    return out;
}

void* Memory::
reallocate(void* ptr, upt bytes) {
	return realloc(ptr, bytes);
}

void Memory::
free(void* ptr) {
    ::free(ptr);
}

void Memory::
copy(void* destination, void* source, upt bytes) {
    memcpy(destination, source, bytes);
}

void Memory::
move(void* destination, void* source, upt bytes) {
    memmove(destination, source, bytes);
}   

void Memory::
zero(void* ptr, upt bytes) {
    memset(ptr, 0, bytes);
}

} // namespace amu
