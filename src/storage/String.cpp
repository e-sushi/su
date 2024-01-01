#include "String.h"
#include "DString.h"
#include "utils/Unicode.h"

#include "stdio.h"
#include "cstdlib"
#include "string.h"

namespace amu {

using namespace unicode;


// returns the codepoint that begins 'a'
u32
codepoint(String a) {
    return DecodedCodepoint(a.str).codepoint;
}

// Shorthand for: a->str += bytes; a->count -= bytes;
FORCE_INLINE void
increment(String& s, u64 bytes){
	s.str += bytes;
	s.count -= bytes;
}

u64 
utf8_move_back(u8* start){
	u64 count = 0;
	while(is_continuation_byte(*(start-1))){
		start--; count++;
	}
	return count;
}

String String::
from(const char* c) {
	return String{c, (s64)strlen(c)};
}

DecodedCodepoint String::
advance(u32 n) {
	DecodedCodepoint decoded;
    while(*this && n--){
		decoded = DecodedCodepoint(str);
        increment(*this, decoded.advance);
    }
	return decoded;
}

void String::
advance_until(u32 c){
    DecodedCodepoint decoded;
    while(*this){
        decoded = DecodedCodepoint(this->str);
        if(decoded.codepoint == c) break;
        increment(*this, decoded.advance);
    }
}

void String::
advance_while(u32 c){
    DecodedCodepoint decoded;
    while(*this){
        decoded = DecodedCodepoint(this->str);
        if(decoded.codepoint != c) break;
        increment(*this, decoded.advance);
    }
}

DecodedCodepoint String::
index(u64 n){
    return advance(n+1);
}

s64 String::
length(){
    s64 result = 0;
    while(advance().codepoint) result++;
    return result;
}

s64 String::
compare(String s, u64 n){
    auto a = *this;
    if(a.str == s.str && a.count == s.count) return 0;
    s64 diff = 0;
    while(!diff && n-- && (a || s)) diff = (s64)a.advance().codepoint - (s64)s.advance().codepoint;
    return diff;
}

b32 String::
equal(String s) {
    return this->count == s.count && !compare(s, s.count);
}

b32 String::
nequal(String s, u64 n){
    return compare(s, n) == 0;
}

b32 String::
begins_with(String s){
    if(this->str == s.str && this->count == s.count) return true;
    return this->count >= s.count && !compare(s, s.length());
}

b32 String::
ends_with(String s){
    if(this->str == s.str && this->count == s.count) return true;
    return this->count >= s.count && compare(String(this->str+this->count-s.count,s.count), s) == 0;
}

b32 String::
contains(String s){
    auto a = *this;
    if(this->str == s.str && this->count == s.count) return true;
    u32 s_len = s.length();
    while(a){
        if(s.count > a.count) return false;
        if(a.nequal(s, s_len)) return true;
        a.advance();
    }
    return false;
}

u32 String::
find_first(u32 codepoint){
    auto a = *this;
    u32 iter = 0;
    while(a){
        DecodedCodepoint d = a.advance();
        if(d.codepoint==codepoint) return iter;
        iter++;
    }
    return -1;
}

u32 String::
find_last(u32 codepoint) {
    auto a = *this;
    u32 iter = 0;
    while(iter < a.count){
        iter += utf8_move_back(this->str+this->count-iter);
        iter++;
        if(DecodedCodepoint(this->str+this->count-iter).codepoint == codepoint) return this->count-iter;
    }
    return -1;
}

String String::
eat(u64 n){
    auto a = *this;
    a.advance(n);
    if(!a) return *this;
    return String(this->str, this->count-a.count);
}


String String::
eat_until(u32 c) {
    auto a = *this;
    DecodedCodepoint decoded;
    while(a){
        decoded = DecodedCodepoint(a.str);
        if(decoded.codepoint == c) break;
        increment(a, decoded.advance);
    }
    if(!a) return *this;
    return String{this->str, this->count-a.count};
}

String String::
eat_until_last(u32 c) {
    auto a = *this;
    s64 count = 0;
    DecodedCodepoint decoded;
    while(a){
        decoded = DecodedCodepoint(a.str);
        if(decoded.codepoint == c) count = a.count;
        increment(a, decoded.advance);
    }
    return String(this->str, this->count-a.count);
}

String String::
eat_until_str(String s) {
    auto a = *this;
    while(a){
        if(a.begins_with(s)) break;
        advance(a);
    }
    if(!a) return *this;
    return String{this->str, this->count-a.count};
}

String String::
eat_whitespace(){
    auto a = *this;
    String out = {this->str,0};
    while(a){
        DecodedCodepoint dc = advance(a); 
        if(!is_space(dc.codepoint)) break;
        out.count += dc.advance;
    }
    return out;
}

String String::
eat_word(b32 include_underscore){
    auto a = *this;
    String out = {this->str,0};
    while(a){
        DecodedCodepoint dc = a.advance();
        if(!is_alnum(dc.codepoint)) break;
        if(dc.codepoint == '_' && !include_underscore) break;
        out.count += dc.advance;
    }
    return out;
}

String String::
eat_int(){
    auto a = *this;
    String out = {this->str,0};
    while(a){
        DecodedCodepoint dc = a.advance();
        if(!is_digit(dc.codepoint)) break;
        out.count += dc.advance;
    }
    return out;
}

String String::
skip(u64 n) {
    auto a = *this;
    a.advance(n);
    return a;
}

String String::
skip_until(u32 c) {
    auto a = *this;
    while(a){
        DecodedCodepoint decoded = DecodedCodepoint(a.str);
        if(decoded.codepoint == c) break;
        increment(a, decoded.advance);
    }
    return a;
}

String String::
skip_until_last(u32 c) {
    auto a = *this;
    String b{};
    while(a){
        DecodedCodepoint decoded = DecodedCodepoint(a.str);
        if(decoded.codepoint == c) b = a;
        increment(a, decoded.advance);
    }
    return b;
}

String String::
skip_whitespace() {
    auto a = *this;
    String out = a;
    while(a){
        if(!is_space(*out.str)) break;
        out.advance();
    }
    return out;
}

// eats and returns a line
String String::
eat_line() {
    auto s = *this;
    String out = {s.str, 0};
    while(s && *s.str != '\n') {
        s.advance();
        out.count += 1;
    }
    return out;
}

Array<String> String::
find_lines() {
    auto s = *this;
    Array<String> out = Array<String>::create();
    String cur = {s.str, 0};
    while(s) {
        if(*s.str == '\n') {
            out.push(cur);
            cur = {s.str + 1, 0};
        } 
        s.advance();
		cur.count++;
    }
	if(cur) out.push(cur);
    return out;
}

Array<s32> String::
find_line_offsets(String s) {
    Array<s32> out = Array<s32>::create();
    u8* start = s.str;
    while(s) {
        if(*s.str == '\n') {
            out.push(s32(s.str-start));
        }
        s.advance();
    }
    return out;
}

u8* String::
seek_n_lines_backward(u64 n, u8* boundry) {
	u8* step_left = this->str;
	
	u64 count = 0;
	while(1) {
		if(*step_left == '\n') {
			count++;
			if(count == n) {
				return step_left + 1;
			}
		} else if(step_left == boundry) {
			return step_left;
		}
		step_left--;
	}
}

f64 String::
to_f64() {
	return strtod((char*)str, 0);
}

s64 String::
to_s64() {
	s64 x;
	(void)sscanf((char*)str, "%lli", &x);
	return x;
}

u32 String::
codepoint(u32 idx) {
	Assert(idx < count);
	while(idx--) advance();
	return DecodedCodepoint(str).codepoint;
}

b32 String::
is_space(u32 codepoint) {
		switch(codepoint){
		case '\t': case '\n': case '\v': case '\f':  case '\r':
		case ' ': case 133: case 160: case 5760: case 8192:
		case 8193: case 8194: case 8195: case 8196: case 8197:
		case 8198: case 8199: case 8200: case 8201: case 8202:
		case 8232: case 8239: case 8287: case 12288: return true;
		default: return false;
	}
}

b32 String::
is_digit(u32 codepoint) {
	if(codepoint >= '0' && codepoint <= '9') return true;
	return false;
}

b32 String::
is_xdigit(u32 codepoint) {
	if(is_digit(codepoint) || 
		codepoint >= 'a' && codepoint <= 'f' ||
		codepoint >= 'A' && codepoint <= 'F')
	   return true;
	return false;
}

b32 String::
is_alpha(u32 codepoint) {
	if(codepoint >= 'A' && codepoint <= 'Z' ||
	   codepoint >= 'a' && codepoint <= 'z') return true;
	return false;
}

b32 String::
is_alnum(u32 codepoint) {
	return is_alpha(codepoint) || is_digit(codepoint);
}

DString String::
replace(u32 find, u32 repl) {
	DString out;
	String src = *this;
	String seg = {str, 0};

	while(src) {
		auto dc = src.advance();
		if(dc.codepoint == find) {
			out.append(seg, repl);					
			seg = {src.str, 0};
		} else {
			seg.count += dc.advance;
		}
	}
	out.append(seg);
	return out;
}

DString String::
replace(String find, String repl) {
	DString out;
	String src = *this;
	String seg = {src.str, 0};

	while(1) {
		while(src && src.str[0] != find.str[0]) {
			seg.count += src.advance().advance; 
		} 

		if(!src) {
			out.append(seg);
			return out;
		}
		
		if(src.begins_with(find)) {
			out.append(repl);	
			increment(src, find.count);
			seg = {src.str, 0};
		}
	}
	return out;
}

namespace util {

template<> u64 hash(const String& s) { return ((String&)s).hash(); }
template<> u64 hash(String* s) { return (*s).hash(); }

void
print(String s) {
	fwrite(s.str, s.count, 1, stdout);
}

void
println(String s) {
	print(s);
	print("\n");
}

} // namespace util

} // namespace amu
