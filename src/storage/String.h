/*
    String view structure supporting utf8.

    This is completely taken from kigu's str8 because I didn't want to try and
    use kigu since it comes with a lot more than what I want. The majority of
    this was originally written by delle.

    This contains utilities for analyzing strings as well.

    A lot of this interface is probably not necessary, because the only time we
    extensively use String is in lexing, so once the compiler is somewhat
    stable, it may be worth seeing if this is still true and removing stuff
    that's not necessary.

    A DString may implicitly be convered to a String, so a constructor is
    provided for that.
*/

#ifndef AMU_STRING_H
#define AMU_STRING_H

#include "util.h"
#include "Array.h"

namespace amu {

struct DString;
struct MessagePart;


struct DecodedCodepoint{
	u32 codepoint;
	u32 advance;
};

struct String {
    union {
        u8* str;
        char* __char_str; // need to cheat cpp a bit to get compile time Strings from literals
    };
    s64 count;


    String(){str=0;count=0; } // i wish i could avoid this
	// NOTE(sushi) this results in a weak reference to the given DString
	//             eg. the DString does not care if this String is still using it !
    String(const DString* dstr);
    String(const char* s, s64 count) : str((u8*)s), count(count) {}
    String(u8* s, s64 count) : str(s), count(count) {}
    consteval String(const char* in) : __char_str((char*)in), count(util::constexpr_strlen(in)) {}
    operator bool() { return str && count; }

	// use for runtime creation of a String from a cstring
	static String
	from(const char* c);

	DecodedCodepoint
	advance(u32 n = 1);

	void
	advance_until(u32 c);

	void
	advance_while(u32 c);

	inline DecodedCodepoint
	index(u64 n);

	// returns the number of codepoints in this String
	inline s64
	length();

	s64
	compare(String s, u64 n = 1);

	b32 
	equal(String s);

	inline b32
	nequal(String s, u64 n);

	inline b32
	begins_with(String s);
	
	inline b32
	ends_with(String s);

	b32
	contains(String s);

	u32
	find_first(u32 codepoint);

	u32
	find_last(u32 codepoint);

	inline String
	eat(u64 n = 1);

	inline String 
	eat_until(u32 c);

	inline String
	eat_until_last(u32 c);

	inline String
	eat_until_str(String s);

	inline String
	eat_whitespace();

	inline String
	eat_word(b32 include_underscore = 0);

	inline String
	eat_int();

	inline String
	skip(u64 n = 1);

	inline String 
	skip_until(u32 c);

	inline String
	skip_until_last(u32 c);

	inline String
	skip_whitespace();

	// eats and returns a line
	String
	eat_line();

	Array<String>
	find_lines();

	Array<s32>
	find_line_offsets(String s);
	
	// seeks backwards in a string until we either pass
	// 'n' newlines or the boundry is reached
	u8*
	seek_n_lines_backward(u64 n, u8* boundry);

	u64 
	hash(u64 seed = 14695981039346656037) {
		auto s = *this;
		while(s.count-- != 0){
			seed ^= *s.str++;
			seed *= 1099511628211; //64bit FNV_prime
		}
		return seed;
	}

	f64 
	to_f64() {
		return strtod((char*)str, 0);
	}

	s64
	to_s64() {
		s64 x;
		(void)sscanf((char*)str, "%lli", &x);
		return x;
	}

	// Returns the codepoint at the given idx
	u32 codepoint(u32 idx = 0);

	static b32 is_space(u32 codepoint);
	static b32 is_digit(u32 codepoint);
	static b32 is_alpha(u32 codepoint);
	static b32 is_alnum(u32 codepoint);

	consteval u64 static_hash(String s, u64 seed = 14695981039);

	DString*
	replace(u32 codepoint_to_replace, u32 codepoint_to_replace_with);

	// Replaces all occurances of 'find_string' with 'replace_string' in this String,
	// and returns a DString* containing the result.
	DString*
	replace(String find_string, String replace_string);

};

namespace util {
template<> FORCE_INLINE u64 hash(const String& s) {return ((String&)s).hash();}
template<> FORCE_INLINE u64 hash(String* s) {return (*s).hash();}

// direct printing functions for debug use
void 
print(String s) {
	fwrite(s.str, s.count, 1, stdout);
}

void
println(String s) {
	print(s);
	print("\n");
}

} // namespace util::hash

} // namespace amu
#endif // AMU_STRING_H
