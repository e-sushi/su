/*
    String view structure, just a wrapper around deshi's str8 for now.
    This is primarily implemented to help amu's internal interface stay 
    consistent and when deshi is removed, all we need to change is this
    struct's backend.

    This contains utilities for analyzing strings as well.

    A DString may implicitly be convered to a String, so a constructor is provided
    for that.
*/

#ifndef AMU_STRING_H
#define AMU_STRING_H

#include "util.h"

namespace amu {

struct DString;
struct String {
    union {
        str8 s;
        struct { char* ptr; s64 count; }__cplusplus_sucks;
        struct {
            u8* str;
            s64 count;
        };
    };


    String(){}
    String(DString& dstr);
    String(str8 in) : s(in) {}
    String(const char* s, s64 count) : s({(u8*)s, count}) {}
    consteval String(const char* in) : __cplusplus_sucks({(char*)in, util::constexpr_strlen(in)}) {}
    operator bool() { return str && count; }
};

namespace string {

// run time creation of String from a cstring
global String
init(const char* s) {
    String out;
    out.s.str = (u8*)s;
    out.s.count = (s64)strlen(s);
    return out;
}

void
advance(String& s, u32 n = 1) {
    str8_nadvance(&s.s, n);
}

u64 
hash(String& s) {
    return str8_hash64(s.s);
}

consteval u64
static_hash(String s, u64 seed = 14695981039346656037) {
    while(s.__cplusplus_sucks.count-- != 0){
		seed ^= (u8)*s.__cplusplus_sucks.ptr;
		seed *= 1099511628211; //64bit FNV_prime
		s.__cplusplus_sucks.ptr++;
	}
	return seed;
}

String
eat(String s, u64 n = 1) {
    s.s = str8_eat_count(s.s, n);
    return s;
}

String 
eat_until(String s, u32 c) {
    s.s = str8_eat_until(s.s, c);
    return s;
}

String
eat_until_last(String s, u32 c) {
    s.s = str8_eat_until_last(s.s, c);
    return s;
}

String
eat_until_str(String s, String c) {
    s.s = str8_eat_until_str(s.s, c.s);
    return s;
}

String
skip(String s, u64 n = 1) {
    s.s = str8_skip_count(s.s, n);
    return s;
}

String 
skip_until(String s, u32 c) {
    s.s = str8_skip_until(s.s, c);
    return s;
}

String
skip_until_last(String s, u32 c) {
    s.s = str8_skip_until_last(s.s, c);
    return s;
}

f64 
to_f64(String& s) {
    return strtod((char*)s.str, 0);
}

s64
to_s64(String& s) {
    s64 x;
    (void)sscanf((char*)s.str, "%lli", &x);
    return x;
}

} // namespace string
} // namespace amu
#endif // AMU_STRING_H