/*
    String view structure supporting utf8.

    This is completely taken from kigu's str8 because I didn't want to try and use kigu since it comes
    with a lot more than what I want. The majority of this was originally written by delle.

    This contains utilities for analyzing strings as well.

    A lot of this interface is probably not necessary, because the only time we extensively use String is 
    in lexing, so once the compiler is somewhat stable, it may be worth seeing if this is still true
    and removing stuff that's not necessary.

    A DString may implicitly be convered to a String, so a constructor is provided
    for that.
*/

#ifndef AMU_STRING_H
#define AMU_STRING_H

#include "util.h"
#include "Array.h"

namespace amu {

struct DString;
struct String {
    union {
        u8* str;
        char* __char_str; // need to cheat cpp a bit to get compile time Strings from literals
    };
    s64 count;


    String(){str=0;count=0;} // i wish i could avoid this
    String(const DString& dstr);
    String(const char* s, s64 count) : str((u8*)s), count(count) {}
    String(u8* s, s64 count) : str(s), count(count) {}
    consteval String(const char* in) : __char_str((char*)in), count(util::constexpr_strlen(in)) {}
    operator bool() { return str && count; }
};

struct DecodedCodepoint{
	u32 codepoint;
	u32 advance;
};

namespace string {

// run time creation of String from a cstring
global String
init(const char* s) {
    String out;
    out.str = (u8*)s;
    out.count = (s64)strlen(s);
    return out;
}

namespace internal {
#define unicode_bitmask1 0x01
#define unicode_bitmask2 0x03
#define unicode_bitmask3 0x07
#define unicode_bitmask4 0x0F
#define unicode_bitmask5 0x1F
#define unicode_bitmask6 0x3F
#define unicode_bitmask7 0x7F
#define unicode_bitmask8 0xFF
#define unicode_bitmask9  0x01FF
#define unicode_bitmask10 0x03FF

global b32
utf8_continuation_byte(u8 byte){
	return ((byte & 0xC0) == 0x80);
}

// Returns the next codepoint and advance from the UTF-8 string `str`
global DecodedCodepoint
decoded_codepoint_from_utf8(u8* str, u64 max_advance){
	persist u8 utf8_class[32] = { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,2,2,2,2,3,3,4,5, };
	
	DecodedCodepoint result = {(u32)-1, 1};
	u8 byte = str[0];
	u8 byte_class = utf8_class[byte >> 3];
	switch(byte_class){
		case 1:{
			result.codepoint = byte;
		}break;
		case 2:{
			if(2 <= max_advance){
				u8 next_byte = str[1];
				if(utf8_class[next_byte >> 3] == 0){
					result.codepoint  = (     byte & unicode_bitmask5) << 6;
					result.codepoint |= (next_byte & unicode_bitmask6);
					result.advance = 2;
				}
			}
		}break;
		case 3:{
			if(3 <= max_advance){
				u8 next_byte[2] = {str[1], str[2]};
				if(   (utf8_class[next_byte[0] >> 3] == 0)
				   && (utf8_class[next_byte[1] >> 3] == 0)){
					result.codepoint  = (        byte & unicode_bitmask4) << 12;
					result.codepoint |= (next_byte[0] & unicode_bitmask6) << 6;
					result.codepoint |= (next_byte[1] & unicode_bitmask6);
					result.advance = 3;
				}
			}
		}break;
		case 4:{
			if(4 <= max_advance){
				u8 next_byte[3] = {str[1], str[2], str[3]};
				if(   (utf8_class[next_byte[0] >> 3] == 0)
				   && (utf8_class[next_byte[1] >> 3] == 0)
				   && (utf8_class[next_byte[2] >> 3] == 0)){
					result.codepoint  = (        byte & unicode_bitmask3) << 18;
					result.codepoint |= (next_byte[0] & unicode_bitmask6) << 12;
					result.codepoint |= (next_byte[1] & unicode_bitmask6) << 6;
					result.codepoint |=  next_byte[2] & unicode_bitmask6;
					result.advance = 4;
				}
			}
		}break;
	}
	return result;
}

// Shorthand for: a->str += bytes; a->count -= bytes;
FORCE_INLINE void
increment(String& s, u64 bytes){
	s.str += bytes;
	s.count -= bytes;
}

global u64 
utf8_move_back(u8* start){
	u64 count = 0;
	while(utf8_continuation_byte(*(start-1))){
		start--; count++;
	}
	return count;
}

#undef unicode_bitmask1
#undef unicode_bitmask2
#undef unicode_bitmask3
#undef unicode_bitmask4
#undef unicode_bitmask5
#undef unicode_bitmask6
#undef unicode_bitmask7
#undef unicode_bitmask8
#undef unicode_bitmask9
#undef unicode_bitmask10
} // namespace internal

// returns the codepoint that begins 'a'
global u32
codepoint(String a) {
    return internal::decoded_codepoint_from_utf8(a.str, 4).codepoint;
}

global b32 
isspace(u32 codepoint){
	switch(codepoint){
		case '\t': case '\n': case '\v': case '\f':  case '\r':
		case ' ': case 133: case 160: case 5760: case 8192:
		case 8193: case 8194: case 8195: case 8196: case 8197:
		case 8198: case 8199: case 8200: case 8201: case 8202:
		case 8232: case 8239: case 8287: case 12288: return true;
		default: return false;
	}
}

global b32 
isalnum(u32 codepoint){
	if(codepoint >= 'A' && codepoint <= 'Z' ||
	   codepoint >= 'a' && codepoint <= 'z' ||
	   codepoint >= '0' && codepoint <= '9') return true;
	return false;
}

global b32 
isdigit(u32 codepoint){
	if(codepoint >= '0' && codepoint <= '9') return true;
	return false;
}

DecodedCodepoint
advance(String& s, u32 n = 1) {
    DecodedCodepoint decoded{};
    while(s && n--){
        decoded = internal::decoded_codepoint_from_utf8(s.str, 4);
        internal::increment(s, decoded.advance);
    }
	return decoded;
}

global void
advance_until(String& s, u32 c){
    DecodedCodepoint decoded{};
    while(s){
        decoded = internal::decoded_codepoint_from_utf8(s.str, 4);
        if(decoded.codepoint == c) break;
        internal::increment(s, decoded.advance);
    }
}

global void
advance_while(String& s, u32 c){
    DecodedCodepoint decoded{};
    while(s){
        decoded = internal::decoded_codepoint_from_utf8(s.str, 4);
        if(decoded.codepoint != c) break;
        internal::increment(s, decoded.advance);
    }
}

global inline DecodedCodepoint
index(String a, u64 n){
	return advance(a, n+1);
}

global inline s64
length(String a){
	s64 result = 0;
	while(advance(a).codepoint) result++;
	return result;
}

global s64
compare(String a, String b, u64 n = 1){
	if(a.str == b.str && a.count == b.count) return 0;
	s64 diff = 0;
	while(diff == 0 && n-- && (a || b)) diff = (s64)advance(a).codepoint - (s64)advance(b).codepoint;
	return diff;
}

global b32 
equal(String a, String b) {
    return a.count == b.count && compare(a, b) == 0;
}

global inline b32
nequal(String a, String b, u64 n){
	return compare(a, b, n) == 0;
}

global inline b32
begins_with(String a, String b){
	if(a.str == b.str && a.count == b.count) return true;
	return a.count >= b.count && compare(a, b, length(b)) == 0;
}

global inline b32
ends_with(String a, String b){
	if(a.str == b.str && a.count == b.count) return true;
	return a.count >= b.count && compare(String(a.str+a.count-b.count,b.count), b) == 0;
}

global b32
contains(String a, String b){
	if(a.str == b.str && a.count == b.count) return true;
	u32 b_len = length(b);
	while(a){
		if(b.count > a.count) return false;
		if(nequal(a, b, b_len)) return true;
		advance(a);
	}
	return false;
}

global u32
find_first(String a, u32 codepoint){
	u32 iter = 0;
	while(a.count){
		DecodedCodepoint d = advance(a);
		if(d.codepoint==codepoint) return iter;
		iter++;
	}
	return npos;
}

global u32
find_last(String a, u32 codepoint) {
	u32 iter = 0;
	while(iter < a.count){
		iter += internal::utf8_move_back(a.str+a.count-iter);
		iter++;
		if(internal::decoded_codepoint_from_utf8(a.str+a.count-iter,4).codepoint == codepoint) return a.count-iter;
	}
	return npos;
}

global inline String
eat(String a, u64 n = 1){
	String b = a;
	advance(b, n);
	if(!b) return a;
	return String(a.str, a.count-b.count);
}


global inline String 
eat_until(String a, u32 c) {
   	String b = a;
    DecodedCodepoint decoded{};
	while(b){
		decoded = internal::decoded_codepoint_from_utf8(b.str, 4);
		if(decoded.codepoint == c) break;
		internal::increment(b, decoded.advance);
	}
	if(!b) return a;
	return String{a.str, a.count-b.count};
}

global inline String
eat_until_last(String a, u32 c) {
	String b = a;
	s64 count = 0;
    DecodedCodepoint decoded{};
	while(b){
		decoded = internal::decoded_codepoint_from_utf8(b.str, 4);
		if(decoded.codepoint == c) count = b.count;
		internal::increment(b, decoded.advance);
	}
	return String(a.str, a.count-count);
}

global inline String
eat_until_str(String a, String c) {
    String b = a;
	while(b){
		if(begins_with(b, c)) break;
		advance(b);
	}
	if(!b) return a;
	return String{a.str, a.count-b.count};
}

global inline String
eat_whitespace(String a){
	String out = {a.str,0};
	while(a){
		DecodedCodepoint dc = advance(a); 
		if(!isspace(dc.codepoint)) break;
		out.count += dc.advance;
	}
	return out;
}

global inline String
eat_word(String a, b32 include_underscore = 0){
	String out = {a.str,0};
	while(a){
		DecodedCodepoint dc = advance(a);
		if(!isalnum(dc.codepoint)) break;
		if(dc.codepoint == '_' && !include_underscore) break;
		out.count += dc.advance;
	}
	return out;
}

global inline String
str8_eat_int(String a){
	String out = {a.str,0};
	while(a){
		DecodedCodepoint dc = advance(a);
		if(!isdigit(dc.codepoint)) break;
		out.count += dc.advance;
	}
	return out;
}

global inline String
skip(String a, u64 n = 1) {
    advance(a, n);
	return a;
}

global inline String 
skip_until(String a, u32 c) {
    while(a){
		DecodedCodepoint decoded = internal::decoded_codepoint_from_utf8(a.str, 4);
		if(decoded.codepoint == c) break;
		internal::increment(a, decoded.advance);
	}
	return a;
}

global inline String
skip_until_last(String a, u32 c) {
    String b{};
	while(a){
		DecodedCodepoint decoded = internal::decoded_codepoint_from_utf8(a.str, 4);
		if(decoded.codepoint == c) b = a;
		internal::increment(a, decoded.advance);
	}
	return b;
}

// eats and returns a line
String
eat_line(String s) {
	String out = {s.str, 0};
	while(s && *s.str != '\n') {
		string::advance(s);
		out.count += 1;
	}
	return out;
}

Array<String>
find_lines(String s) {
	Array<String> out = array::init<String>();
	String cur = {s.str, 0};
	while(s) {
		if(*s.str == '\n') {
			array::push(out, cur);
			cur = {s.str + 1, 0};
		} else cur.count++;
		string::advance(s);
	}
	return out;
}

Array<s32>
find_line_offsets(String s) {
	Array<s32> out = array::init<s32>();
	u8* start = s.str;
	while(s) {
		if(*s.str == '\n') {
			array::push(out, s32(s.str-start));
		}
		string::advance(s);
	}
	return out;
}

global u64 
hash(String s, u64 seed = 14695981039346656037) {
    while(s.count-- != 0){
		seed ^= *s.str++;
		seed *= 1099511628211; //64bit FNV_prime
	}
	return seed;
}

consteval u64
static_hash(String s, u64 seed = 14695981039346656037) {
    while(s.count-- != 0){
		seed ^= (u8)*s.__char_str;
		seed *= 1099511628211; //64bit FNV_prime
		s.__char_str++;
	}
	return seed;
}

f64 
to_f64(const String& s) {
    return strtod((char*)s.str, 0);
}

s64
to_s64(const String& s) {
    s64 x;
    (void)sscanf((char*)s.str, "%lli", &x);
    return x;
}

} // namespace string

namespace util {
template<> FORCE_INLINE u64 hash(const String& s) {return string::hash((String&)s);}
template<> FORCE_INLINE u64 hash(String* s) {return string::hash(*s);}

void 
print(String s) {
	fprintf(stdout, "%s", s.str);
}

void
println(String s) {
	fprintf(stdout, "%s\n", s.str);
}



} // namespace util::hash

} // namespace amu
#endif // AMU_STRING_H