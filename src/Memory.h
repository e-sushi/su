/*
    amu's generic memory interface
*/

#ifndef AMU_MEMORY_H
#define AMU_MEMORY_H


#include "kigu/common.h"
#include "core/memory.h"

namespace amu {

extern Allocator _amu_allocator;
extern Allocator* amu_allocator;

namespace memory {

extern mutex global_lock;

void* 
allocate(upt size);

void* 
reallocate(void* ptr, upt size);

void 
free(void* ptr);


} // namespace memory
} // namespace amu

#endif  // AMU_MEMORY_H