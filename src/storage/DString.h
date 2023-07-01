/*

    Dynamic string structure, really just a wrapper around dstr8.
    String views are just done through str8

    This is primarily to create a consistent interface for usage of strings in amu
    and if it somehow becomes necessary later on (and it REALLY shouldn't) to allow
    easily extending Strings to be thread safe. Though I guess if we ever come across
    that, we should probably just make a special thread safe string that is used 
    where it is needed.

    This ensures that usages of dstr8 itself are thread safe as well, since deshi's
    internal memory is not yet thread safe.

    Functions that serve stricly as wrappers around dstr8 are forced to be inline

*/

#ifndef AMU_DSTRING_H
#define AMU_DSTRING_H

#include "kigu/unicode.h"
#include "util.h"
#include "storage/String.h"

namespace amu {

struct DString {
    dstr8 s;
};

namespace dstring {

// initializes a String with an optional initial string
FORCE_INLINE DString
init(String s = ""); 

// deinitializes a string, freeing its memory
FORCE_INLINE void
deinit(DString& s);

// appends 'b' to 'a'
FORCE_INLINE void
append(DString& a, String b);

// appends 'b' to 'a'
FORCE_INLINE void
append(DString& a, DString& b);

// concatenates two Strings into a new String
FORCE_INLINE DString
concat(DString& a, DString& b);

// concatenates a str8 to a String and returns a new String
FORCE_INLINE DString
concat(DString& a, String b);

// concatenates a String to a str8 and returns a new String
FORCE_INLINE DString
concat(String a, DString& b);

} // namespace string
} // namespace amu

#endif // AMU_DSTRING_H