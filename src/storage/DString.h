#ifndef AMU_DSTRING_H
#define AMU_DSTRING_H

#include "String.h"
#include <type_traits>

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

void to_string(DString& x, const char* y);
void to_string(DString& x, const String y);
void to_string(DString& x, const DString y);
void to_string(DString& x, const u8 y);
void to_string(DString& x, const u16 y);
void to_string(DString& x, const u32 y);
void to_string(DString& x, const u64 y);
void to_string(DString& x, const s8 y);
void to_string(DString& x, const s16 y);
void to_string(DString& x, const s32 y);
void to_string(DString& x, const s64 y);
void to_string(DString& x, const f32 y);
void to_string(DString& x, const f64 y);
void to_string(DString& x, const void* y);

template<typename... T> void DString::
append(T... args) {
    (to_string(*this, args), ...);
}

} // namespace amu

#endif // AMU_DSTRING_H
