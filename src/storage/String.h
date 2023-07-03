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
    };


    String(){}
    String(DString& dstr); 
    String(str8 in) : s(in) {}
    String(const char* s, s64 count) : s({(u8*)s, count}) {}
    consteval String(char* in) : __cplusplus_sucks({in, util::constexpr_strlen(in)}) {}
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

} // namespace string
} // namespace amu
#endif // AMU_STRING_H