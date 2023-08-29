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


	!!!!!!!
		Becuase this struct owns memory, it should NEVER have a constructor, at least
		one where it acquires new memory!!!

*/

#ifndef AMU_DSTRING_H
#define AMU_DSTRING_H

#include "Memory.h"
#include "util.h"
#include "storage/String.h"

namespace amu {

struct DString {
    union {
		struct {u8* str; s64 count;};
		String fin;
	};
	s64 space;

	DString(){str=0;count=0;space=0;}
};

namespace dstring {

// initializes a String with an optional initial string
DString
init(String s = "");

// variadic version
template<typename... T> FORCE_INLINE DString
init(T...args); 

// deinitializes a string, freeing its memory
void
deinit(DString& s);

void
append(DString& a, String b);

template<typename... T> void
append(DString& a, T... args);

// offset is a byte offset, NOT a codepoint offset
// no checks are done to ensure that the offset is not inside of a codepoint
global void
insert(DString& a, u64 offset, String b);

// prepends 'b' to 'a'
void
prepend(DString& a, String b);

// concatenates a String to a DString and returns a new DString
DString
concat(DString& a, String b);

// removes a codepoint at BYTE 'offset' 
// if the given offset is within a codepoint, or the offset is greater than 'a's length, returns 0
// returns how many bytes were removed
global u64
remove(DString& a, u64 offset);

// removes 'count' bytes starting from byte 'offset'
// if the given offset is within a codepoint, or the offset is greater than a's length, returns 0
// if 'offset' plus 'count' is greater than a's length, we remove until we can't anymore
// returns the number of bytes removed
global u32
remove(DString& a, u64 offset, u64 count);

} // namespace dstring

DString to_string(const String& s) {
    return dstring::init(s);
}

String::String(const DString& dstr) {
	str = dstr.str;
	count = dstr.count;
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
	}else if constexpr(std::is_same_v<T, char>){
        dstring::append(out, String(&x, 1));
	}else if constexpr(std::is_same_v<T, s32>){
		out.count = snprintf(nullptr, 0, "%d", x);
		out.space = out.count+1;
		out.str   = (u8*)memory::allocate(out.count+1);
		Assert(out.str, "Failed to allocate memory");
		snprintf((char*)out.str, out.count+1, "%d", x);
	}else if constexpr(std::is_same_v<T, long>){
		out.count = snprintf(nullptr, 0, "%ld", x);
		out.space = out.count+1;
		out.str   = (u8*)memory::allocate(out.count+1);
		Assert(out.str, "Failed to allocate memory");
		snprintf((char*)out.str, out.count+1, "%ld", x);
	}else if constexpr(std::is_same_v<T, s64>){
		out.count = snprintf(nullptr, 0, "%lld", x);
		out.space = out.count+1;
		out.str   = (u8*)memory::allocate(out.count+1);
		Assert(out.str, "Failed to allocate memory");
		snprintf((char*)out.str, out.count+1, "%lld", x);
	}else if constexpr(std::is_same_v<T, u32>){
		out.count = snprintf(nullptr, 0, "%u", x);
		out.space = out.count+1;
		out.str   = (u8*)memory::allocate(out.count+1);
		Assert(out.str, "Failed to allocate memory");
		snprintf((char*)out.str, out.count+1, "%u", x);
	}else if constexpr(std::is_same_v<T, u64>){
		out.count = snprintf(nullptr, 0, "%llu", x);
		out.space = out.count+1;
		out.str   = (u8*)memory::allocate(out.count+1);
		Assert(out.str, "Failed to allocate memory");
		snprintf((char*)out.str, out.count+1, "%llu", x);
	}else if constexpr(std::is_same_v<T, f32> || std::is_same_v<T, f64>){
		out.count = snprintf(nullptr, 0, "%g", x);
		out.space = out.count+1;
		out.str   = (u8*)memory::allocate(out.count+1);
		Assert(out.str, "Failed to allocate memory");
		snprintf((char*)out.str, out.count+1, "%g", x);
	}else if constexpr(std::is_same_v<T, upt>){
		out.count = snprintf(nullptr, 0, "%zu", x);
		out.space = out.count+1;
		out.str   = (u8*)memory::allocate(out.count+1);
		Assert(out.str, "Failed to allocate memory");
		snprintf((char*)out.str, out.count+1, "%zu", x);
	}else if constexpr(is_ptr<T>::value){
		out.count = snprintf(nullptr, 0, "%p", (void*)x);
		out.space = out.count+1;
		out.str   = (u8*)memory::allocate(out.count+1);
		Assert(out.str, "Failed to allocate memory");
		snprintf((char*)out.str, out.count+1, "%p", (void*)x);
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
	}else if constexpr(std::is_same_v<T, String> || std::is_same_v<DString, T>) {
		dstring::append(start, String(x));
	}else if constexpr(std::is_same_v<T, char>){
        dstring::append(start, String(&x, 1));
    }else if constexpr(std::is_same_v<T, u8>){
        u64 count = snprintf(nullptr, 0, "%d", x);
		start.str   = (u8*)memory::reallocate(start.str, start.count + count + 1);
		Assert(start.str, "Failed to allocate memory");
		snprintf((char*)start.str+start.count, count+1, "%d", x);
		start.count += count;
		start.space = start.count + 1;
	}else if constexpr(std::is_same_v<T, s32>){
        u64 count = snprintf(nullptr, 0, "%d", x);
		start.str   = (u8*)memory::reallocate(start.str, start.count + count + 1);
		Assert(start.str, "Failed to allocate memory");
		snprintf((char*)start.str+start.count, count+1, "%d", x);
		start.count += count;
		start.space = start.count + 1;
	}else if constexpr(std::is_same_v<T, long>){
		u64 count = snprintf(nullptr, 0, "%ld", x);
		start.str   = (u8*)memory::reallocate(start.str, start.count + count + 1);
		Assert(start.str, "Failed to allocate memory");
		snprintf((char*)start.str+start.count, count+1, "%ld", x);
        start.count += count;
		start.space = start.count+1;
	}else if constexpr(std::is_same_v<T, s64>){
        u64 count = snprintf(nullptr, 0, "%lld", x);
		start.str   = (u8*)memory::reallocate(start.str, start.count + count + 1);
		Assert(start.str, "Failed to allocate memory");
		snprintf((char*)start.str+start.count, count+1, "%lld", x);
        start.count += count;
		start.space = start.count+1;
	}else if constexpr(std::is_same_v<T, u32>){
		u64 count = snprintf(nullptr, 0, "%u", x);
		start.str   = (u8*)memory::reallocate(start.str, start.count + count + 1);
		Assert(start.str, "Failed to allocate memory");
		snprintf((char*)start.str+start.count, count+1, "%u", x);
        start.count += count;
		start.space = start.count+1;
	}else if constexpr(std::is_same_v<T, u64>){
		u64 count = snprintf(nullptr, 0, "%llu", x);
		start.str   = (u8*)memory::reallocate(start.str, start.count + count + 1);
		Assert(start.str, "Failed to allocate memory");
		snprintf((char*)start.str+start.count, count+1, "%llu", x);
        start.count += count;
		start.space = start.count+1;
	}else if constexpr(std::is_same_v<T, f32> || std::is_same_v<T, f64>){
		u64 count = snprintf(nullptr, 0, "%g", x);
		start.str   = (u8*)memory::reallocate(start.str, start.count + count + 1);
		Assert(start.str, "Failed to allocate memory");
		snprintf((char*)start.str+start.count, count+1, "%g", x);
        start.count += count;
		start.space = start.count+1;
	}else if constexpr(std::is_same_v<T, upt>){
		u64 count = snprintf(nullptr, 0, "%zu", x);
		start.str   = (u8*)memory::reallocate(start.str, start.count + count + 1);
		Assert(start.str, "Failed to allocate memory");
		snprintf((char*)start.str+start.count, count+1, "%zu", x);
        start.count += count;
		start.space = start.count+1;
	}else if constexpr(is_ptr<T>::value){
		u64 count = snprintf(nullptr, 0, "%p", (void*)x);
		start.str   = (u8*)memory::reallocate(start.str, start.count + count + 1);
		Assert(start.str, "Failed to allocate memory");
		snprintf((char*)start.str+start.count, count+1, "%p", (void*)x);
        start.count += count;
		start.space = start.count+1;
	}
}

namespace util {
DString format_time(f64 ms){
	DString out = dstring::init();
    if(floor(Minutes(ms))){
		//hope it never gets this far :)
		f64 fmin = floor(Minutes(ms));
		f64 fsec = floor(Seconds(ms)) - fmin * 60;
		f64 fms  = ms - fmin*60*1000 - fsec*1000;
		dstring::append(out, fmin, "m ", fsec, "s ", fms, " ms");
	}else if(floor(Seconds(ms))){
		f64 fsec = floor(Seconds(ms));
		f64 fms  = ms - fsec*SecondsToMS(1);
		dstring::append(out, fsec, "s ", fms, "ms");
	}else{
		dstring::append(out, ms, " ms");
	}
    return out;
}

// wraps a DString in ANSI terminal color
// TODO(sushi) need to setup using the 8 original colors so that terminal themes work properly
void
wrap_color(DString& current, u32 color) {
    DString temp = dstring::init("\e[", color, "m");
    dstring::prepend(current, temp);
    dstring::append(current, "\e[0m");
    dstring::deinit(temp);
}

void
todo(String s, String file, u64 line) {
	// !Leak
	println(dstring::init("TODO:", file, ":", line, ": ", s));
}

// for todos that should be resolved quickly 
#define TODO(s) util::todo(s, __FILE__, __LINE__)
}

} // namespace amu

#endif // AMU_DSTRING_H