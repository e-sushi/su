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

#include "Memory.h"
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

// variadic version
template<typename... T> FORCE_INLINE DString
init(T...args); 

// deinitializes a string, freeing its memory
FORCE_INLINE void
deinit(DString& s);

// grows the buffer by at least 'bytes'
FORCE_INLINE void
grow(DString& s, u64 bytes);

// appends 'b' to 'a'
FORCE_INLINE void
append(DString& a, String b);

// appends 'b' to 'a'
FORCE_INLINE void
append(DString& a, DString b);

template<typename... T> void
append(DString& a, T... args);

// prepends 'b' to 'a'
FORCE_INLINE void
prepend(DString& a, String b);

// prepends 'b' to 'a'
FORCE_INLINE void
prepend(DString& a, DString b);

// concatenates two Strings into a new String
FORCE_INLINE DString
concat(DString& a, DString b);

// concatenates a str8 to a String and returns a new String
FORCE_INLINE DString
concat(DString& a, String b);

// concatenates a String to a str8 and returns a new String
FORCE_INLINE DString
concat(String a, DString b);

} // namespace dstring

DString to_string(const String& s) {
    return dstring::init(s);
}

String::String(DString& dstr) {
	str = dstr.s.str;
	count = dstr.s.count;
}

// https://stackoverflow.com/questions/301330/determine-if-type-is-a-pointer-in-a-template-function
template<typename T> struct is_ptr { static const bool value = false; };
template<typename T> struct is_ptr<T*> { static const bool value = true; };

// TODO(sushi) rewrite this later on
//             need to make a version that appends to an already existing DString so that
//             we dont allocate a ton of temp stuff
template<typename T> global DString 
to_string(T x) {
    DString out = dstring::init();
	if       constexpr(std::is_same_v<T, char*> || std::is_same_v<T, const char*>){
		dstring::append(out, String(x, (s64)strlen(x)));
	}else if constexpr(std::is_same_v<T, str8> || std::is_same_v<T, const str8&>){
        dstring::append(out, String{x});
	}else if constexpr(std::is_same_v<T, char>){
        dstring::append(out, String(&x, 1));
	}else if constexpr(std::is_same_v<T, s32>){
		out.s.count = snprintf(nullptr, 0, "%d", x);
		out.s.space = out.s.count+1;
		out.s.str   = (u8*)memory::allocate(out.s.count+1);
		Assert(out.s.str, "Failed to allocate memory");
		snprintf((char*)out.s.str, out.s.count+1, "%d", x);
	}else if constexpr(std::is_same_v<T, long>){
		out.s.count = snprintf(nullptr, 0, "%ld", x);
		out.s.space = out.s.count+1;
		out.s.str   = (u8*)memory::allocate(out.s.count+1);
		Assert(out.s.str, "Failed to allocate memory");
		snprintf((char*)out.s.str, out.s.count+1, "%ld", x);
	}else if constexpr(std::is_same_v<T, s64>){
		out.s.count = snprintf(nullptr, 0, "%lld", x);
		out.s.space = out.s.count+1;
		out.s.str   = (u8*)memory::allocate(out.s.count+1);
		Assert(out.s.str, "Failed to allocate memory");
		snprintf((char*)out.s.str, out.s.count+1, "%lld", x);
	}else if constexpr(std::is_same_v<T, u32>){
		out.s.count = snprintf(nullptr, 0, "%u", x);
		out.s.space = out.s.count+1;
		out.s.str   = (u8*)memory::allocate(out.s.count+1);
		Assert(out.s.str, "Failed to allocate memory");
		snprintf((char*)out.s.str, out.s.count+1, "%u", x);
	}else if constexpr(std::is_same_v<T, u64>){
		out.s.count = snprintf(nullptr, 0, "%llu", x);
		out.s.space = out.s.count+1;
		out.s.str   = (u8*)memory::allocate(out.s.count+1);
		Assert(out.s.str, "Failed to allocate memory");
		snprintf((char*)out.s.str, out.s.count+1, "%llu", x);
	}else if constexpr(std::is_same_v<T, f32> || std::is_same_v<T, f64>){
		out.s.count = snprintf(nullptr, 0, "%g", x);
		out.s.space = out.s.count+1;
		out.s.str   = (u8*)memory::allocate(out.s.count+1);
		Assert(out.s.str, "Failed to allocate memory");
		snprintf((char*)out.s.str, out.s.count+1, "%g", x);
	}else if constexpr(std::is_same_v<T, upt>){
		out.s.count = snprintf(nullptr, 0, "%zu", x);
		out.s.space = out.s.count+1;
		out.s.str   = (u8*)memory::allocate(out.s.count+1);
		Assert(out.s.str, "Failed to allocate memory");
		snprintf((char*)out.s.str, out.s.count+1, "%zu", x);
	}else if constexpr(is_ptr<T>::value){
		out.s.count = snprintf(nullptr, 0, "%p", (void*)x);
		out.s.space = out.s.count+1;
		out.s.str   = (u8*)memory::allocate(out.s.count+1);
		Assert(out.s.str, "Failed to allocate memory");
		snprintf((char*)out.s.str, out.s.count+1, "%p", (void*)x);
	}
	return out;
}

// NOTE(sushi) this doesn't check if there is already enough space to hold the new characters
//             it just shrinks it down to fit the resulting string
// NOTE(sushi) the reason this isn't an overload of dstring's append is because I dont want
//             to have to start a dstring namespace just to make an overload of it 
template<typename T> global void
to_string(DString& start, T x) {
	if       constexpr(std::is_same_v<T, char*> || std::is_same_v<T, const char*>){
		dstring::append(start, String(x, (s64)strlen(x)));
	}else if constexpr(std::is_same_v<T, str8> || std::is_same_v<T, const str8&>){
        dstring::append(start, String{x});
	}else if constexpr(std::is_same_v<T, char>){
        dstring::append(start, String(&x, 1));
    }else if constexpr(std::is_same_v<T, u8>){
        u64 count = snprintf(nullptr, 0, "%d", x);
		start.s.str   = (u8*)memory::reallocate(start.s.str, start.s.count + count + 1);
		Assert(start.s.str, "Failed to allocate memory");
		snprintf((char*)start.s.str+start.s.count, count+1, "%d", x);
		start.s.count += count;
		start.s.space = start.s.count + 1;
	}else if constexpr(std::is_same_v<T, s32>){
        u64 count = snprintf(nullptr, 0, "%d", x);
		start.s.str   = (u8*)memory::reallocate(start.s.str, start.s.count + count + 1);
		Assert(start.s.str, "Failed to allocate memory");
		snprintf((char*)start.s.str+start.s.count, count+1, "%d", x);
		start.s.count += count;
		start.s.space = start.s.count + 1;
	}else if constexpr(std::is_same_v<T, long>){
		u64 count = snprintf(nullptr, 0, "%ld", x);
		start.s.str   = (u8*)memory::reallocate(start.s.str, start.s.count + count + 1);
		Assert(start.s.str, "Failed to allocate memory");
		snprintf((char*)start.s.str+start.s.count, count+1, "%ld", x);
        start.s.count += count;
		start.s.space = start.s.count+1;
	}else if constexpr(std::is_same_v<T, s64>){
        u64 count = snprintf(nullptr, 0, "%lld", x);
		start.s.str   = (u8*)memory::reallocate(start.s.str, start.s.count + count + 1);
		Assert(start.s.str, "Failed to allocate memory");
		snprintf((char*)start.s.str+start.s.count, count+1, "%lld", x);
        start.s.count += count;
		start.s.space = start.s.count+1;
	}else if constexpr(std::is_same_v<T, u32>){
		u64 count = snprintf(nullptr, 0, "%u", x);
		start.s.str   = (u8*)memory::reallocate(start.s.str, start.s.count + count + 1);
		Assert(start.s.str, "Failed to allocate memory");
		snprintf((char*)start.s.str+start.s.count, count+1, "%u", x);
        start.s.count += count;
		start.s.space = start.s.count+1;
	}else if constexpr(std::is_same_v<T, u64>){
		u64 count = snprintf(nullptr, 0, "%llu", x);
		start.s.str   = (u8*)memory::reallocate(start.s.str, start.s.count + count + 1);
		Assert(start.s.str, "Failed to allocate memory");
		snprintf((char*)start.s.str+start.s.count, count+1, "%llu", x);
        start.s.count += count;
		start.s.space = start.s.count+1;
	}else if constexpr(std::is_same_v<T, f32> || std::is_same_v<T, f64>){
		u64 count = snprintf(nullptr, 0, "%g", x);
		start.s.str   = (u8*)memory::reallocate(start.s.str, start.s.count + count + 1);
		Assert(start.s.str, "Failed to allocate memory");
		snprintf((char*)start.s.str+start.s.count, count+1, "%g", x);
        start.s.count += count;
		start.s.space = start.s.count+1;
	}else if constexpr(std::is_same_v<T, upt>){
		u64 count = snprintf(nullptr, 0, "%zu", x);
		start.s.str   = (u8*)memory::reallocate(start.s.str, start.s.count + count + 1);
		Assert(start.s.str, "Failed to allocate memory");
		snprintf((char*)start.s.str+start.s.count, count+1, "%zu", x);
        start.s.count += count;
		start.s.space = start.s.count+1;
	}else if constexpr(is_ptr<T>::value){
		u64 count = snprintf(nullptr, 0, "%p", (void*)x);
		start.s.str   = (u8*)memory::reallocate(start.s.str, start.s.count + count + 1);
		Assert(start.s.str, "Failed to allocate memory");
		snprintf((char*)start.s.str+start.s.count, count+1, "%p", (void*)x);
        start.s.count += count;
		start.s.space = start.s.count+1;
	}else if constexpr(std::is_same_v<DString, T>) {
		dstring::append(start, x);
	}
}

} // namespace amu

#endif // AMU_DSTRING_H