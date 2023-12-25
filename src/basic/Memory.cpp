#include "Common.h"
#include "Memory.h"

#include "stdlib.h"
#include "string.h"

namespace amu {

Memory memory;

void* Memory:: 
allocate(upt size) {
#if AMU_TRACK_MEMORY
	void* out = malloc(size + sizeof(Header));
	zero(out, size + sizeof(Header));
	((Header*)out)->size = size;
	bytes_allocated += size;
	n_allocations++;
	return (u8*)out + sizeof(Header);
#else
    void* out = malloc(size);
    zero(out, size);
    return out;
#endif
}

void* Memory::
reallocate(void* ptr, upt bytes) {
#if AMU_TRACK_MEMORY
	Header* header = ((Header*)ptr) - 1;
	bytes_allocated -= header->size;
	bytes_allocated += bytes;
	header->size = bytes;
	return (u8*)realloc(header, bytes + sizeof(Header)) + sizeof(Header);
#else
	return realloc(ptr, bytes);
#endif
}
 
void Memory::
free(void* ptr) {
#if AMU_TRACK_MEMORY
	Header* header = ((Header*)ptr) - 1;
	bytes_allocated -= header->size;
	n_allocations--;
	::free(header);
#else
    ::free(ptr);
#endif
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
