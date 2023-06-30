/*
    amu's generic memory interface
*/

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