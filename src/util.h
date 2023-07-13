/*
    Various helpful utilities or definitions for use throughout amu and other various things
	that don't really have a place anywhere else.

	This file is likely to be included in most places, so be careful about editing it.
*/

#ifndef AMU_UTIL_H
#define AMU_UTIL_H

#include "kigu/common.h"
#include "kigu/unicode.h"

namespace deshi {
#   include "core/threading.h"
}

namespace amu {
namespace util {

// debug print functions
void 
print(str8 s) {
    printf("%s", s.str);
}

void 
println(str8 s) {
    printf("%s\n", s.str);
}

constexpr s64 
constexpr_strlen(const char* s) {
    s64 i = 0;
    while(s[i]) {
        i++;
    }
    return i;
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