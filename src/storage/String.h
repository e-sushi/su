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
    consteval String(char* in) : __cplusplus_sucks({in, util::constexpr_strlen(in)}) {}
};

namespace string {


} // namespace string
} // namespace amu

#endif // AMU_STRING_H