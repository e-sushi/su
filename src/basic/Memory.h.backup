/*
    amu's generic memory interface
*/

#ifndef AMU_MEMORY_H
#define AMU_MEMORY_H


namespace amu {
namespace memory {

FORCE_INLINE void* 
allocate(upt size);

FORCE_INLINE void* 
reallocate(void* ptr, upt size);

FORCE_INLINE void 
free(void* ptr);

FORCE_INLINE void
copy(void* destination, void* source, upt bytes);

FORCE_INLINE void
move(void* destination, void* source, upt bytes);

FORCE_INLINE void
zero(void* ptr, upt bytes);

} // namespace memory
} // namespace amu

#endif  // AMU_MEMORY_H