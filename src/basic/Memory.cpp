#include "Common.h"
#include "Memory.h"

#if AMU_TRACK_MEMORY

#include "storage/DString.h"
#include "utils/Units.h"
#include <cstdio>

#endif


#include <cstdlib>
#include <cstring>

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

void Memory::
print_memory_usage() {
#if AMU_TRACK_MEMORY
	u64 actual_allocated = n_allocations * sizeof(Header) + bytes_allocated;
	printf(
R"amu(
MEMORY USAGE:
     n_allocations: %lli
   bytes_allocated: %lli%sb
  actual_allocated: %lli%sb
)amu",
	n_allocations,
	si_divide(bytes_allocated), si_prefix_symbol(bytes_allocated).str,
	si_divide(actual_allocated), si_prefix_symbol(actual_allocated).str
);
#endif
}

} // namespace amu
