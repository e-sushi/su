#include "Bump.h"
#include "basic/Memory.h"

namespace amu {

void Bump::
init() {
	allocated_slabs_list = (u8**)memory.allocate(slab_size + sizeof(u8**));
	start = cursor = (u8*)(allocated_slabs_list + 1);
}

void Bump::
deinit() {
	u8* next = *allocated_slabs_list;
	while(allocated_slabs_list) {
		u8** next = (u8**)*allocated_slabs_list;
		memory.free(allocated_slabs_list);
		allocated_slabs_list = next;
	}
	cursor = start = 0;
}

void* Bump::
allocate(u32 size) {
	Assert(size <= slab_size);

	if(cursor - start + size > slab_size) {
		u8* newslab = (u8*)memory.allocate(slab_size + sizeof(u8**));
		*((u8**)start - 1) = newslab;
		start = cursor = newslab + 1;
	}

	u8* out = cursor;
	cursor += size;
	return out;
}

}
