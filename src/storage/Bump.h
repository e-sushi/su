/*

   Bump allocator.
   
   Basically an arena. You cannot deallocate memory from this allocator,
   and its memory never moves.
   Currently it allocates slabs of 4096 bytes each.

*/

#ifndef AMU_BUMP_H
#define AMU_BUMP_H

#include "Common.h"
#include "Array.h"
#include "basic/Allocator.h"

namespace amu {

struct Bump : public Allocator {
	const static u32 
		slab_size = 4096;
	
	u8* start;
	u8* cursor;

	u8** allocated_slabs_list;

	void
	init();

	void
	deinit();

	void* allocate(u32 size);

	// bump allocators do no tracking of memory except their blocks
	// so these operations are not supported
	void* reallocate(void* ptr, u32 size) {Assert(0); return 0;}
	void  free(void* ptr) {Assert(0);}
};

}

#endif
