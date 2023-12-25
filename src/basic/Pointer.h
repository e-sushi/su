#ifndef AMU_POINTER_H
#define AMU_POINTER_H

#include "Common.h"
#include "Memory.h"

namespace amu {

template<typename T, s32 DEPTH = 1>
struct ptr {
	void* x = 0;

	ptr() {}
	ptr(const T& p) : x((void*)&p) {};
	ptr(const T* p) : x(p) {};
	ptr(const ptr<T, DEPTH>& rhs) { x = rhs.x; }

	inline ptr<T, DEPTH+1> 
	ref() {
		return ptr((T*)&x);
	}

	inline std::conditional_t<DEPTH==1, T, ptr<T,DEPTH-1>>
	deref() {
		if constexpr (DEPTH==1){
			return *(T*)x;
		} else {
			return ptr<T, DEPTH-1>((void*)*(T**)(x));
		}
	}
};

template<typename T, s32 N = 1>
struct counted_ptr {
	void* x;

	struct header {
		u32 count;
	};

	counted_ptr() {
		this->x = memory.allocate(sizeof(header) + sizeof(T));
		auto h = (header*)this->x;
		h->count = 1;
	}

	counted_ptr(const T& val) {
		counted_ptr();
		*((T*)x) = val;
	}

	template<typename X>
	counted_ptr(const X* rhs) {
		static_assert(!std::is_same_v<X, T>, "A counted_ptr cannot take an already existing raw pointer. Use .allocate instead.");
		Assert(0);
	}

	counted_ptr(const counted_ptr<T, N>& rhs) {
		auto h = ((header*)rhs.x) - 1;
		h->count += 1;
		this->x = rhs.x;
	}

	~counted_ptr() {
		auto h = ((header*)this->x) - 1;
		h->count -= 1;
		if(!h->count) {
			memory.free(this->x);
		}
	}

	counted_ptr<T, N+1>
	ref() {
		counted_ptr<T, N+1> out = &x;
		return out;
	}

	std::conditional_t<N == 1, T, counted_ptr<T, N - 1>>
	deref() {
		if constexpr (N==1) {
			return *(T*)x;
		} else {
			return counted_ptr<T, N-1>();
		}
	}


	T* operator ->() {
		return (T*)this->x;
	}

};

// possibly mess with later
//template<typename T, s32 DEPTH = 1>
//struct moved_ptr : public ptr<T, DEPTH> {
//	
//};
//
//template<typename T, s32 DEPTH = 1>
//struct movable_ptr : public ptr<T, DEPTH> {
//	b32 moved = false;
//
//	moved_ptr<T, DEPTH>
//	move() {
//		if(moved) {
//			Assert(0);
//		}
//		moved = true;
//		moved_ptr<T, DEPTH> out;
//		out.x = this->x;
//		return out;
//	}
//	
//	template<typename X>
//	operator X() {
//		static_assert(!std::is_same_v<X, ptr<T,DEPTH>>, "A movable_ptr cannot implicitly cast to a ptr, you must use .move()");
//		static_assert(!std::is_same_v<X, moved_ptr<T,DEPTH>>, "A movable_ptr must call .move() to be used where a moved_ptr is required.");
//	}
//};

}

#endif
