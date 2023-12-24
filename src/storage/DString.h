#ifndef AMU_DSTRING_H
#define AMU_DSTRING_H

#include "String.h"

namespace amu {

struct DString {
	u8* ptr;
	
	DString(String x = ""); 
	DString(const DString& x);
	~DString();

	// creates a copy of this dstring
	DString
	copy();

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
	// if the given offset is within a codepoint or the offset is greater 
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

	// if needed, grows to fit 'bytes'
	void
	fit(u64 bytes);

	// indents each line by n spaces
	// if n is negative, outdents instead
	void
	indent(s64 n);

	s64&
	space();

	s64& 
	count();

	s64
	available_space();

	s64
	refs();

	String
	get_string();
};

static DString
to_string(const String& s) {
    return DString(s);
}

// https://stackoverflow.com/questions/301330/determine-if-type-is-a-pointer-in-a-template-function
template<typename T> struct is_ptr { static const bool value = false; };
template<typename T> struct is_ptr<T*> { static const bool value = true; };

// NOTE(sushi) this doesn't check if there is already enough space to hold the new characters
//             it just shrinks it down to fit the resulting string
// NOTE(sushi) the reason this isn't an overload of dstring's append is because I dont want
//             to have to start a dstring namespace just to make an overload of it 
template<typename T> global void
to_string(DString& start, T x) {
#define conversion(s)                                                   \
        u64 count = snprintf(nullptr, 0, s, x);                         \
		s64 available_space = start.available_space();                  \
		if(available_space < count) {                                   \
			start.grow(count - available_space);	                    \
		}                                                               \
		snprintf((char*)start.ptr + start.count(), count+1, "%hhi", x); \
		start.count() += count;

	if       constexpr(std::is_same_v<T, char*> || std::is_same_v<T, const char*>){
		start.append(String(x, (s64)strlen(x)));
	}else if constexpr(std::is_same_v<T, String> || std::is_same_v<DString*, T>) {
		start.append(String(x));
	}else if constexpr(std::is_same_v<T, char>){
        start.append(String(&x, 1));
    }else if constexpr(std::is_same_v<T, u8>){
		conversion("%hhu")
	}else if constexpr(std::is_same_v<T, u16>){
		conversion("%hu");
	}else if constexpr(std::is_same_v<T, u32>){
		conversion("%u");
	}else if constexpr(std::is_same_v<T, u64>){
		conversion("%llu");
	}else if constexpr(std::is_same_v<T, s8>){
		conversion("%hhi");
	}else if constexpr(std::is_same_v<T, s16>){
		conversion("%hi");
	}else if constexpr(std::is_same_v<T, s32>){
		conversion("%i");
	}else if constexpr(std::is_same_v<T, s64>){
		conversion("%lli");
	}else if constexpr(std::is_same_v<T, long>){
		conversion("%ld");
	}else if constexpr(std::is_same_v<T, f32> || std::is_same_v<T, f64>){
		conversion("%g");
	}else if constexpr(std::is_same_v<T, upt>){
		conversion("%zu");
	}else if constexpr(is_ptr<T>::value){
		conversion("%p");
	}
}

template<typename... T> void DString::
append(T... args) {
    (to_string(*this, args), ...);
}

} // namespace amu

#endif // AMU_DSTRING_H
