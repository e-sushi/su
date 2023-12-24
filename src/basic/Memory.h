/*
    amu's generic memory interface
*/

#ifndef AMU_MEMORY_H
#define AMU_MEMORY_H

#include "Common.h"

namespace amu {

// TODO(sushi) memory tracking
struct Memory {
	void*
	allocate(upt size);

	template<typename T> T*
	allocate();

	void*
	reallocate(void* ptr, upt size);
	
	void
	free(void* ptr);

	void
	copy(void* dst, void* src, upt bytes);

	template<typename T> T*
	copy(T* source, upt bytes);

	void
	move(void* dst, void* src, upt bytes);

	void
	zero(void* ptr, upt bytes);

	template<typename T> void
	zero(T* ptr);
};

extern Memory memory;

template<typename T> T* Memory::
allocate() {
	T* out = (T*)malloc(sizeof(T));
	zero(out, sizeof(T));
	return out;
}

template<typename T> T* Memory::
copy(T* source, upt bytes) {
    T* out = (T*)allocate(bytes);
    copy((void*)out, (void*)source, bytes);
    return out;
}

template<typename T> void Memory::
zero(T* ptr) {
	zero(ptr, sizeof(T));
}

} // namespace amu

#endif  // AMU_MEMORY_H
