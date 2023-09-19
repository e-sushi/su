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

namespace amu {

struct DString {
    union {
		struct {u8* str; s64 count;};
		String fin;
	};

	s64 space;
	s64 refs;

	static DString*
	create(String s = "");

	template<typename... T> static DString*
	create(T...args);

	// NOT(sushi) this DOES NOT check if there are still refs (only gives a warning if there are)
	//            it just straight destroys the DString!
	void
	destroy();

	// returns a new reference to this DString
	DString*
	ref();

	// indicates that a reference to this DString is no longer needed
	void
	deref();

	// appends the String s to this DString
	// if String s comes from a DString, remember that this is a weak ref!
	void
	append(String s);

	// appends a variadic amount of things
	template<typename... T> void
	append(T... args);

	// inserts 's' at the given 'byte_offset'
	// NO checks are done to see if 'byte_offset' is within a codepoint
	void
	insert(u64 byte_offset, String s);

	void
	prepend(String s);

	// removes a codepoint at 'byte_offset'
	// if the given offset is within a codepoint or the offset is greater than 
	// than this DString's length, we return 0
	// otherwise return how many bytes were removed 
	u64 
	remove(u64 byte_offset);

	// removes 'count' bytes start from 'byte_offset'
	// if the given offset is within a codepoint, or the offset is greater than this DString's length we return 0
	// if 'byte_offset' plus 'count' is greater than the length of this DString, we remove until the end
	// returns the number of bytes removed
	u64 
	remove(u64 byte_offset, u64 count);

	void 
	grow(u64 bytes);

	// indents each line by n spaces
	// if n is negative, outdents instead
	void
	indent(s64 n);
	
	DString(){str=0;count=0;space=0;}
};

struct ScopedDStringRef {
	DString* x;
	ScopedDStringRef(DString* in) : x(in) { x->ref(); }
	~ScopedDStringRef() { x->deref(); }

	DString* operator->() const {
		return x;
	}
};

struct ScopedDeref {
	DString* x;
	ScopedDeref(DString* in) : x(in) {}
	~ScopedDeref() { x->deref(); }
};

DString*
to_string(const String& s) {
    return DString::create(s);
}

String::String(const DString* dstr) {
	str = dstr->str;
	count = dstr->count;
}

// https://stackoverflow.com/questions/301330/determine-if-type-is-a-pointer-in-a-template-function
template<typename T> struct is_ptr { static const bool value = false; };
template<typename T> struct is_ptr<T*> { static const bool value = true; };

// TODO(sushi) rewrite this later on
//             need to make a version that appends to an already existing DString so that
//             we dont allocate a ton of temp stuff
template<typename T> global DString*
to_string(T x) {
    DString* out = DString::create();
	if       constexpr(std::is_same_v<T, char*> || std::is_same_v<T, const char*>){
		out->append(String(x, (s64)strlen(x)));
	}else if constexpr(std::is_same_v<T, char>){
        out->append(String(&x, 1));
	}else if constexpr(std::is_same_v<T, s32>){
		out->count = snprintf(nullptr, 0, "%d", x);
		out->space = out->count+1;
		out->str   = (u8*)memory::allocate(out->count+1);
		Assert(out->str, "Failed to allocate memory");
		snprintf((char*)out->str, out->count+1, "%d", x);
	}else if constexpr(std::is_same_v<T, long>){
		out->count = snprintf(nullptr, 0, "%ld", x);
		out->space = out->count+1;
		out->str   = (u8*)memory::allocate(out->count+1);
		Assert(out->str, "Failed to allocate memory");
		snprintf((char*)out->str, out->count+1, "%ld", x);
	}else if constexpr(std::is_same_v<T, s8>){
		out->count = snprintf(nullptr, 0, "%hhi", x);
		out->space = out->count+1;
		out->str   = (u8*)memory::allocate(out->count+1);
		Assert(out->str, "Failed to allocate memory");
		snprintf((char*)out->str, out->count+1, "%hhi", x);
	}else if constexpr(std::is_same_v<T, s16>){
		out->count = snprintf(nullptr, 0, "%i", x);
		out->space = out->count+1;
		out->str   = (u8*)memory::allocate(out->count+1);
		Assert(out->str, "Failed to allocate memory");
		snprintf((char*)out->str, out->count+1, "%i", x);
	}else if constexpr(std::is_same_v<T, s32>){
		out->count = snprintf(nullptr, 0, "%ld", x);
		out->space = out->count+1;
		out->str   = (u8*)memory::allocate(out->count+1);
		Assert(out->str, "Failed to allocate memory");
		snprintf((char*)out->str, out->count+1, "%ld", x);
	}else if constexpr(std::is_same_v<T, s64>){
		out->count = snprintf(nullptr, 0, "%lld", x);
		out->space = out->count+1;
		out->str   = (u8*)memory::allocate(out->count+1);
		Assert(out->str, "Failed to allocate memory");
		snprintf((char*)out->str, out->count+1, "%lld", x);
	}else if constexpr(std::is_same_v<T, u8>){
		out->count = snprintf(nullptr, 0, "%hhu", x);
		out->space = out->count+1;
		out->str   = (u8*)memory::allocate(out->count+1);
		Assert(out->str, "Failed to allocate memory");
		snprintf((char*)out->str, out->count+1, "%hhu", x);
	}else if constexpr(std::is_same_v<T, u16>){
		out->count = snprintf(nullptr, 0, "%u", x);
		out->space = out->count+1;
		out->str   = (u8*)memory::allocate(out->count+1);
		Assert(out->str, "Failed to allocate memory");
		snprintf((char*)out->str, out->count+1, "%u", x);
	}else if constexpr(std::is_same_v<T, u32>){
		out->count = snprintf(nullptr, 0, "%lu", x);
		out->space = out->count+1;
		out->str   = (u8*)memory::allocate(out->count+1);
		Assert(out->str, "Failed to allocate memory");
		snprintf((char*)out->str, out->count+1, "%lu", x);
	}else if constexpr(std::is_same_v<T, u64>){
		out->count = snprintf(nullptr, 0, "%llu", x);
		out->space = out->count+1;
		out->str   = (u8*)memory::allocate(out->count+1);
		Assert(out->str, "Failed to allocate memory");
		snprintf((char*)out->str, out->count+1, "%llu", x);
	}else if constexpr(std::is_same_v<T, f32> || std::is_same_v<T, f64>){
		out->count = snprintf(nullptr, 0, "%g", x);
		out->space = out->count+1;
		out->str   = (u8*)memory::allocate(out->count+1);
		Assert(out->str, "Failed to allocate memory");
		snprintf((char*)out->str, out->count+1, "%g", x);
	}else if constexpr(std::is_same_v<T, upt>){
		out->count = snprintf(nullptr, 0, "%zu", x);
		out->space = out->count+1;
		out->str   = (u8*)memory::allocate(out->count+1);
		Assert(out->str, "Failed to allocate memory");
		snprintf((char*)out->str, out->count+1, "%zu", x);
	}else if constexpr(is_ptr<T>::value){
		out->count = snprintf(nullptr, 0, "%p", (void*)x);
		out->space = out->count+1;
		out->str   = (u8*)memory::allocate(out->count+1);
		Assert(out->str, "Failed to allocate memory");
		snprintf((char*)out->str, out->count+1, "%p", (void*)x);
	}
	return out;
}

// NOTE(sushi) this doesn't check if there is already enough space to hold the new characters
//             it just shrinks it down to fit the resulting string
// NOTE(sushi) the reason this isn't an overload of dstring's append is because I dont want
//             to have to start a dstring namespace just to make an overload of it 
template<typename T> global void
to_string(DString* start, T x) {
	if       constexpr(std::is_same_v<T, char*> || std::is_same_v<T, const char*>){
		start->append(String(x, (s64)strlen(x)));
	}else if constexpr(std::is_same_v<T, String> || std::is_same_v<DString*, T>) {
		start->append(String(x));
	}else if constexpr(std::is_same_v<T, char>){
        start->append(String(&x, 1));
    }else if constexpr(std::is_same_v<T, u8>){
        u64 count = snprintf(nullptr, 0, "%hhi", x);
		start->str   = (u8*)memory::reallocate(start->str, start->count + count + 1);
		Assert(start->str, "Failed to allocate memory");
		snprintf((char*)start->str+start->count, count+1, "%hhi", x);
		start->count += count;
		start->space = start->count + 1;
	}else if constexpr(std::is_same_v<T, u16>){
        u64 count = snprintf(nullptr, 0, "%hu", x);
		start->str   = (u8*)memory::reallocate(start->str, start->count + count + 1);
		Assert(start->str, "Failed to allocate memory");
		snprintf((char*)start->str+start->count, count+1, "%hu", x);
		start->count += count;
		start->space = start->count + 1;
	}else if constexpr(std::is_same_v<T, u32>){
        u64 count = snprintf(nullptr, 0, "%u", x);
		start->str   = (u8*)memory::reallocate(start->str, start->count + count + 1);
		Assert(start->str, "Failed to allocate memory");
		snprintf((char*)start->str+start->count, count+1, "%u", x);
		start->count += count;
		start->space = start->count + 1;
	}else if constexpr(std::is_same_v<T, u64>){
        u64 count = snprintf(nullptr, 0, "%llu", x);
		start->str   = (u8*)memory::reallocate(start->str, start->count + count + 1);
		Assert(start->str, "Failed to allocate memory");
		snprintf((char*)start->str+start->count, count+1, "%llu", x);
		start->count += count;
		start->space = start->count + 1;
	}else if constexpr(std::is_same_v<T, s8>){
        u64 count = snprintf(nullptr, 0, "%hhi", x);
		start->str   = (u8*)memory::reallocate(start->str, start->count + count + 1);
		Assert(start->str, "Failed to allocate memory");
		snprintf((char*)start->str+start->count, count+1, "%hhi", x);
		start->count += count;
		start->space = start->count + 1;
	}else if constexpr(std::is_same_v<T, s16>){
        u64 count = snprintf(nullptr, 0, "%hi", x);
		start->str   = (u8*)memory::reallocate(start->str, start->count + count + 1);
		Assert(start->str, "Failed to allocate memory");
		snprintf((char*)start->str+start->count, count+1, "%hi", x);
		start->count += count;
		start->space = start->count + 1;
	}else if constexpr(std::is_same_v<T, s32>){
        u64 count = snprintf(nullptr, 0, "%i", x);
		start->str   = (u8*)memory::reallocate(start->str, start->count + count + 1);
		Assert(start->str, "Failed to allocate memory");
		snprintf((char*)start->str+start->count, count+1, "%i", x);
		start->count += count;
		start->space = start->count + 1;
	}else if constexpr(std::is_same_v<T, s64>){
        u64 count = snprintf(nullptr, 0, "%lli", x);
		start->str   = (u8*)memory::reallocate(start->str, start->count + count + 1);
		Assert(start->str, "Failed to allocate memory");
		snprintf((char*)start->str+start->count, count+1, "%lli", x);
		start->count += count;
		start->space = start->count + 1;
	}else if constexpr(std::is_same_v<T, long>){
		u64 count = snprintf(nullptr, 0, "%ld", x);
		start->str   = (u8*)memory::reallocate(start->str, start->count + count + 1);
		Assert(start->str, "Failed to allocate memory");
		snprintf((char*)start->str+start->count, count+1, "%ld", x);
        start->count += count;
		start->space = start->count+1;
	}else if constexpr(std::is_same_v<T, s64>){
        u64 count = snprintf(nullptr, 0, "%lld", x);
		start->str   = (u8*)memory::reallocate(start->str, start->count + count + 1);
		Assert(start->str, "Failed to allocate memory");
		snprintf((char*)start->str+start->count, count+1, "%lld", x);
        start->count += count;
		start->space = start->count+1;
	}else if constexpr(std::is_same_v<T, f32> || std::is_same_v<T, f64>){
		u64 count = snprintf(nullptr, 0, "%g", x);
		start->str   = (u8*)memory::reallocate(start->str, start->count + count + 1);
		Assert(start->str, "Failed to allocate memory");
		snprintf((char*)start->str+start->count, count+1, "%g", x);
        start->count += count;
		start->space = start->count+1;
	}else if constexpr(std::is_same_v<T, upt>){
		u64 count = snprintf(nullptr, 0, "%zu", x);
		start->str   = (u8*)memory::reallocate(start->str, start->count + count + 1);
		Assert(start->str, "Failed to allocate memory");
		snprintf((char*)start->str+start->count, count+1, "%zu", x);
        start->count += count;
		start->space = start->count+1;
	}else if constexpr(is_ptr<T>::value){
		u64 count = snprintf(nullptr, 0, "%p", (void*)x);
		start->str   = (u8*)memory::reallocate(start->str, start->count + count + 1);
		Assert(start->str, "Failed to allocate memory");
		snprintf((char*)start->str+start->count, count+1, "%p", (void*)x);
        start->count += count;
		start->space = start->count+1;
	}
}

namespace util {
DString*
format_time(f64 ms){
	DString* out = DString::create();
    if(floor(Minutes(ms))){
		//hope it never gets this far :)
		f64 fmin = floor(Minutes(ms));
		f64 fsec = floor(Seconds(ms)) - fmin * 60;
		f64 fms  = ms - fmin*60*1000 - fsec*1000;
		out->append(fmin, "m ", fsec, "s ", fms, " ms");
	}else if(floor(Seconds(ms))){
		f64 fsec = floor(Seconds(ms));
		f64 fms  = ms - fsec*SecondsToMS(1);
		out->append(fsec, "s ", fms, "ms");
	} else {
		out->append(ms, " ms");
	}
    return out;
}


DString*
format_metric(s64 x) {
	auto out = DString::create();
	if(x > s64(1e12)) {
		out->append(x/s64(1e12), ".", x%s64(1e12), " tera");
	} else if(x > s64(1e9)) {
		out->append(x/s64(1e9), ".", x%s64(1e9), " giga");
	} else if(x > s64(1e6)) {
		out->append(x/s64(1e6), ".", x%s64(1e6), " mega");
	} else if(x > s64(1e3)) {
		out->append(x/s64(1e3), ".", x%s64(1e3), " kilo");
	} else {
		out->append(x);
	}
	return out;
}

// wraps a DString in ANSI terminal color
// TODO(sushi) need to setup using the 8 original colors so that terminal themes work properly
void
wrap_color(DString* current, u32 color) {
    auto temp = DString::create("\e[", color, "m");
    current->prepend(temp);
    current->append("\e[0m");
    temp->deref();
}

void
todo(String s, String file, u64 line) {
	// !Leak
	println(ScopedDStringRef(DString::create("TODO:", file, ":", line, ": ", s)).x);
}

// for todos that should be resolved quickly 
#define TODO(s) do { util::todo(s, __FILE__, __LINE__); Assert(0); } while(0)
#define TODOsoft(s) do { util::todo(s, __FILE__, __LINE__); } while(0)

}

// I don't know if it is just clang or what, but since DStrings are entirely accessed by pointers now
// and (probably) because there is a forward declaration of DString in String, clang will not output Debug information
// about DString beyond that forward declaration. I can't seem to reproduce this in an isolated project, so I don't know
// what the real cause is, but yeah, that's why this is here.
namespace ___clang___is___weird___ {
DString instance;
}

} // namespace amu

#endif // AMU_DSTRING_H