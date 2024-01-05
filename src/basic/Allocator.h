#ifndef AMU_ALLOCATOR_H
#define AMU_ALLOCATOR_H

#include "Common.h"

struct Allocator {
	virtual void* allocate(u32 size) = 0;
	virtual void* reallocate(void* ptr, u32 size) = 0;
	virtual void free(void* ptr) = 0;
};

#endif
