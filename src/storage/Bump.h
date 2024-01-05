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

template<typename T>
struct FAIL_DUMMY { static const b32 value = false; };

namespace amu {

struct Bump : public Allocator {
	const static u32 
		slab_size = 4096;
	
	u8* start;
	u8* cursor;

	u8** allocated_slabs_list;

	void init();
	void deinit();

	void* allocate(u32 size) override;

	void* reallocate(void* ptr, u32 old_size, u32 new_size);

	[[deprecated("normal reallocate is not implemented for Bump! use the version that takes previous size instead.")]]
	void* reallocate(void* ptr, u32 size) override { Assert(0); return 0; } 
	
	[[deprecated("Bump does not implement free!")]]
	void  free(void* ptr) override { Assert(0); }
};

}

#endif
