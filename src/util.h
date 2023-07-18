/*
    Various helpful utilities or definitions for use throughout amu and other various things
	that don't really have a place anywhere else.

	This file is likely to be included in most places, so be careful about editing it.
*/

#ifndef AMU_UTIL_H
#define AMU_UTIL_H

#undef global
#include <chrono>
#define global static

namespace amu {
namespace util {

typedef std::chrono::time_point<std::chrono::high_resolution_clock> Stopwatch;

namespace stopwatch {

Stopwatch 
start() {
	return std::chrono::high_resolution_clock::now();
}

f64
peek(Stopwatch watch) {
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - watch).count(); 
}

void
reset(Stopwatch& watch) {
	watch =  std::chrono::high_resolution_clock::now();
}

} // namespace stopwatch

constexpr s64 
constexpr_strlen(const char* s) {
    s64 i = 0;
    while(s[i]) {
        i++;
    }
    return i;
}

constexpr upt
round_up_to(s64 value, s64 multiple) {
	return  (((upt)((value) + (((upt)(multiple))-1)) / (upt)(multiple)) * (upt)(multiple));
}

// specializable generic hash functions
template<typename T> u64
hash(const T& x) {
	u32 seed = 2166136261;
	size_t data_size = sizeof(T);
	const u8* data = (const u8*)&x;
	while (data_size-- != 0) {
		seed ^= *data++;
		seed *= 16777619;
	}
	return seed;
}

template<typename T> u64
hash(T* x) {
	u32 seed = 2166136261;
	size_t data_size = sizeof(T);
	const u8* data = (const u8*)x;
	while (data_size-- != 0) {
		seed ^= *data++;
		seed *= 16777619;
	}
	return seed;
}

}



} // namespace amu

#endif // AMU_UTIL_H