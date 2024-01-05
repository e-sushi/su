/*
    amu's generic memory interface
*/

#ifndef AMU_MEMORY_H
#define AMU_MEMORY_H

#define AMU_TRACK_MEMORY true

#include "Common.h" 

namespace amu {

// TODO(sushi) memory tracking
struct Memory {
#if AMU_TRACK_MEMORY
	struct Header {
		u64 size;
	};
	u64 bytes_allocated;
	u64 n_allocations;
#endif

	void*
	allocate(upt size);

	void*
	reallocate(void* ptr, upt size);
	
	void
	free(void* ptr);

	void
	copy(void* dst, void* src, upt bytes);

	void
	move(void* dst, void* src, upt bytes);

	void
	zero(void* ptr, upt bytes);

	template<typename T, typename... Args> T
	construct(Args... args);

	// prints the current amount of memory amu is using
	// note that this will do nothing if AMU_TRACK_MEMORY
	// is not defined.
	// note that this uses printf and not messenger
	void print_memory_usage();
};

extern Memory memory;

} // namespace amu

#endif  // AMU_MEMORY_H
