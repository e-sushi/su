#pragma once
#ifndef DESHI_STRING_UTILS_H
#define DESHI_STRING_UTILS_H

#include "string.h"
#include "cstring.h"
#include "defines.h"

#include <cstdarg>
#include <string>

///////////////
//// @stox ////
///////////////
global_ s32
stoi(const string& s) { //TODO make cstring overloads of these
	s32 x;
	(void)sscanf(s.str, "%d", &x);
	return x;
}

global_ s64
stolli(const string& s){
	s64 x;
	(void)sscanf(s.str, "%lli", &x);
	return x;
}

global_ f64
stod(const string& s) {
	return strtod(s.str, 0);
}

////////////////////
//// @to_string ////
////////////////////
global_ string 
to_string(cstring x){
	return string(x.str, x.count);
}

global_ string 
to_string(char* str){ 
	return string(str); 
}

global_ string 
to_string(const char* str){ 
	return string(str); 
}

global_ string 
to_string(const string& str){ 
	return str; 
}

global_ string 
to_string(const std::string& str){ 
	return str.c_str(); 
}

global_ string 
stringf(const char* fmt, ...){
	va_list argptr;
	va_start(argptr, fmt);
	string s;
	s.count = vsnprintf(nullptr, 0, fmt, argptr);
	s.str   = (char*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
	s.allocator->commit(s.str, s.count+1);
	s.space = s.count+1;
	vsnprintf(s.str, s.count+1, fmt, argptr);
	va_end(argptr);
	return s;
}

global_ string 
to_string(char x){
	string s(&x, 1);
	return s;
}

global_ string 
to_string(s32 x){
	string s;
	s.count = snprintf(nullptr, 0, "%d", x);
	s.str   = (char*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
	s.allocator->commit(s.str, s.count+1);
	s.space = s.count+1;
	snprintf(s.str, s.count+1, "%d", x);
	return s;
}

global_ string
to_string(s64 x) {
	string s;
	s.count = snprintf(nullptr, 0, "%lli", x);
	s.str = (char*)s.allocator->reserve(s.count + 1); Assert(s.str, "Failed to allocate memory");
	s.allocator->commit(s.str, s.count + 1);
	s.space = s.count + 1;
	snprintf(s.str, s.count + 1, "%lli", x);
	return s;
}

global_ string 
to_string(u32 x){
	string s;
	s.count = snprintf(nullptr, 0, "%d", x);
	s.str   = (char*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
	s.allocator->commit(s.str, s.count+1);
	s.space = s.count+1;
	snprintf(s.str, s.count+1, "%d", x);
	return s;
}

#if COMPILER_GCC && !defined(OS_WINDOWS)
global_ string
to_string(u64 x) {
	string s;
	s.count = snprintf(nullptr, 0, "%llu", x);
	s.str = (char*)s.allocator->reserve(s.count + 1); Assert(s.str, "Failed to allocate memory");
	s.allocator->commit(s.str, s.count + 1);
	s.space = s.count + 1;
	snprintf(s.str, s.count + 1, "%d", x);
	return s;
}
#endif

global_ string 
to_string(f32 x, bool trunc = true){
	string s;
	if(trunc){
		s.count = snprintf(nullptr, 0, "%g", x);
		s.str   = (char*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
		s.allocator->commit(s.str, s.count+1);
		s.space = s.count+1;
		snprintf(s.str, s.count+1, "%g", x);
	}else{
		s.count = snprintf(nullptr, 0, "%f", x);
		s.str   = (char*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
		s.allocator->commit(s.str, s.count+1);
		s.space = s.count+1;
		snprintf(s.str, s.count+1, "%f", x);
	}
	return s;
}

global_ string 
to_string(f64 x, bool trunc = true){
	string s;
	if(trunc){
		s.count = snprintf(nullptr, 0, "%g", x);
		s.str   = (char*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
		s.allocator->commit(s.str, s.count+1);
		s.space = s.count+1;
		snprintf(s.str, s.count+1, "%g", x);
	}else{
		s.count = snprintf(nullptr, 0, "%f", x);
		s.str   = (char*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
		s.allocator->commit(s.str, s.count+1);
		s.space = s.count+1;
		snprintf(s.str, s.count+1, "%f", x);
	}
	return s;
}

global_ string 
to_string(upt x){
	string s;
	s.count = snprintf(nullptr, 0, "%zu", x);
	s.str   = (char*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
	s.allocator->commit(s.str, s.count+1);
	s.space = s.count+1;
	snprintf(s.str, s.count+1, "%zu", x);
	return s;
}

global_ string
to_string(void* ptr) {
	string s;
	s.count = snprintf(nullptr, 0, "%p", ptr);
	s.str = (char*)s.allocator->reserve(s.count + 1); Assert(s.str, "Failed to allocate memory");
	s.allocator->commit(s.str, s.count + 1);
	s.space = s.count + 1;
	snprintf(s.str, s.count + 1, "%p", ptr);
	return s;
}

#define toStr(...) (ToString(__VA_ARGS__))
template<class... T> global_ string 
ToString(T... args){
	string str;
	constexpr auto arg_count{sizeof...(T)};
	string arr[arg_count] = {to_string(args)...};
	forI(arg_count) str += arr[i];
	return str;
}

template<typename... T>
inline b32 company(char c, T... args) {
	return ((c == args) || ...);
}

template<char... in> global_  u32
find_first_char(const char* str, u32 strsize, u32 offset = 0) {
	for (u32 i = offset; i < strsize; ++i)
		if (company(str[i], in...)) return i;
	return npos;
}
template<char... in> global_ u32 find_first_char(const char*    str, u32 offset = 0) { return find_first_char<in...>(str, strlen(str), offset); } 
template<char... in> global_ u32 find_first_char(const cstring& str, u32 offset = 0) { return find_first_char<in...>(str.str, str.count, offset); } 
template<char... in> global_ u32 find_first_char(const string&  str, u32 offset = 0) { return find_first_char<in...>(str.str, str.count, offset); }

template<char... in> global_ u32
find_first_char_not(const char* str, u32 strsize, u32 offset = 0) {
	for (u32 i = 0; i < strsize; ++i)
		if (!company(str[i], in...)) return i;
	return npos;
}
template<char... in> global_ u32 find_first_char_not(const char*    str, u32 offset = 0) { return find_first_char_not<in...>(str, strlen(str), offset); }
template<char... in> global_ u32 find_first_char_not(const cstring& str, u32 offset = 0) { return find_first_char_not<in...>(str.str, str.count, offset); }
template<char... in> global_ u32 find_first_char_not(const string&  str, u32 offset = 0) { return find_first_char_not<in...>(str.str, str.count, offset); }

template<char... in> global_ u32
find_last_char(const char* str, u32 strsize, char c, u32 offset = 0) {
	for (u32 i = (offset != 0 ? offset : strsize - 1); i != 0; --i)
		if (company(str[i], in...)) return i;
	return npos;
}
template<char... in> global_ u32 find_last_char(const char*    str, u32 offset = 0) { return find_last_char<in...>(str, strlen(str), offset); } 
template<char... in> global_ u32 find_last_char(const cstring& str, u32 offset = 0) { return find_last_char<in...>(str.str, str.count, offset); } 
template<char... in> global_ u32 find_last_char(const string&  str, u32 offset = 0) { return find_last_char<in...>(str.str, str.count, offset); }

template<char... in> global_ u32
find_last_char_not(const char* str, u32 strsize, char c, u32 offset = 0) {
	for (u32 i = strsize - 1; i != 0; --i)
		if (!company(str[i], in...)) return i;
	return npos;
}
template<char... in> global_ u32 find_last_char_not(const char*    str, u32 offset = 0) { return find_last_char_not<in...>(str, strlen(str), offset); }
template<char... in> global_ u32 find_last_char_not(const cstring& str, u32 offset = 0) { return find_last_char_not<in...>(str.str, str.count, offset); }
template<char... in> global_ u32 find_last_char_not(const string&  str, u32 offset = 0) { return find_last_char_not<in...>(str.str, str.count, offset); }

global_ u32 
find_first_string(const char* str, u32 strsize, const char* str2, u32 str2size, u32 offset = 0) {
	if (str2size > strsize) return npos;
	for (u32 i = offset; i < strsize - str2size; i++)
		if (!memcmp(str + i, str2, str2size)) return i;
	return npos;
}
global_ u32 find_first_string(const string&  buf1, const string&  buf2, u32 offset = 0) { return find_first_string(buf1.str, buf1.count, buf2.str, buf2.count, offset); }
global_ u32 find_first_string(const cstring& buf1, const cstring& buf2, u32 offset = 0) { return find_first_string(buf1.str, buf1.count, buf2.str, buf2.count, offset); }
global_ u32 find_first_string(const string&  buf1, const cstring& buf2, u32 offset = 0) { return find_first_string(buf1.str, buf1.count, buf2.str, buf2.count, offset); }
global_ u32 find_first_string(const cstring& buf1, const string&  buf2, u32 offset = 0) { return find_first_string(buf1.str, buf1.count, buf2.str, buf2.count, offset); }
global_ u32 find_first_string(const char*    buf1, const string&  buf2, u32 offset = 0) { return find_first_string(buf1, strlen(buf1), buf2.str, buf2.count, offset); }
global_ u32 find_first_string(const string&  buf1, const char*    buf2, u32 offset = 0) { return find_first_string(buf1.str, buf1.count, buf2, strlen(buf2), offset); }
global_ u32 find_first_string(const char*    buf1, const cstring& buf2, u32 offset = 0) { return find_first_string(buf1, strlen(buf1), buf2.str, buf2.count, offset); }
global_ u32 find_first_string(const cstring& buf1, const char*    buf2, u32 offset = 0) { return find_first_string(buf1.str, buf1.count, buf2, strlen(buf2), offset); }
global_ u32 find_first_string(const char*    buf1, const char*    buf2, u32 offset = 0) { return find_first_string(buf1, strlen(buf1), buf2, strlen(buf2), offset); }

FORCE_INLINE global_ b32
str_begins_with(const char* buf1, const char* buf2, u32 buf2len) {return !memcmp(buf1, buf2, buf2len);}
global_ b32 str_begins_with(const string&  buf1, const string&  buf2) { return str_begins_with(buf1.str, buf2.str, buf2.count); }
global_ b32 str_begins_with(const cstring& buf1, const cstring& buf2) { return str_begins_with(buf1.str, buf2.str, buf2.count); }
global_ b32 str_begins_with(const string&  buf1, const cstring& buf2) { return str_begins_with(buf1.str, buf2.str, buf2.count); }
global_ b32 str_begins_with(const cstring& buf1, const string&  buf2) { return str_begins_with(buf1.str, buf2.str, buf2.count); }
global_ b32 str_begins_with(const char*    buf1, const string&  buf2) { return str_begins_with(buf1, buf2.str, buf2.count); }
global_ b32 str_begins_with(const string&  buf1, const char*    buf2) { return str_begins_with(buf1.str, buf2, strlen(buf2)); }
global_ b32 str_begins_with(const char*    buf1, const cstring& buf2) { return str_begins_with(buf1, buf2.str, buf2.count); }
global_ b32 str_begins_with(const cstring& buf1, const char*    buf2) { return str_begins_with(buf1.str, buf2, strlen(buf2)); }
global_ b32 str_begins_with(const char*    buf1, const char*    buf2) { return str_begins_with(buf1, buf2, strlen(buf2)); }

FORCE_INLINE global_ b32
str_ends_with(const char* buf1, u32 buf1len, const char* buf2, u32 buf2len) {return (buf2len > buf1len ? 0 : !memcmp(buf1 + (buf1len - buf2len), buf2, buf2len));}
global_ b32 str_ends_with(const string&  buf1, const string&  buf2) { return str_ends_with(buf1.str, buf1.count, buf2.str, buf2.count); }
global_ b32 str_ends_with(const cstring& buf1, const cstring& buf2) { return str_ends_with(buf1.str, buf1.count, buf2.str, buf2.count); }
global_ b32 str_ends_with(const string&  buf1, const cstring& buf2) { return str_ends_with(buf1.str, buf1.count, buf2.str, buf2.count); }
global_ b32 str_ends_with(const cstring& buf1, const string&  buf2) { return str_ends_with(buf1.str, buf1.count, buf2.str, buf2.count); }
global_ b32 str_ends_with(const char*    buf1, const string&  buf2) { return str_ends_with(buf1, strlen(buf1), buf2.str, buf2.count); }
global_ b32 str_ends_with(const string&  buf1, const char*    buf2) { return str_ends_with(buf1.str, buf1.count, buf2, strlen(buf2)); }
global_ b32 str_ends_with(const char*    buf1, const cstring& buf2) { return str_ends_with(buf1, strlen(buf1), buf2.str, buf2.count); }
global_ b32 str_ends_with(const cstring& buf1, const char*    buf2) { return str_ends_with(buf1.str, buf1.count, buf2, strlen(buf2)); }
global_ b32 str_ends_with(const char*    buf1, const char*    buf2) { return str_ends_with(buf1, strlen(buf1), buf2, strlen(buf2)); }

global_ b32
str_contains(const char* buf1, u32 buf1len, const char* buf2, u32 buf2len) {
	if (buf2len > buf1len) return false;
	for (u32 i = 0; i < buf2len - buf1len; i++) {
		if (!memcmp(buf2, buf1 + i, buf2len)) return true;
	}
	return false;
}
global_ b32 str_contains(const string&  buf1, const string&  buf2) { return str_contains(buf1.str, buf1.count, buf2.str, buf2.count); }
global_ b32 str_contains(const cstring& buf1, const cstring& buf2) { return str_contains(buf1.str, buf1.count, buf2.str, buf2.count); }
global_ b32 str_contains(const string&  buf1, const cstring& buf2) { return str_contains(buf1.str, buf1.count, buf2.str, buf2.count); }
global_ b32 str_contains(const cstring& buf1, const string&  buf2) { return str_contains(buf1.str, buf1.count, buf2.str, buf2.count); }
global_ b32 str_contains(const char*    buf1, const string&  buf2) { return str_contains(buf1, strlen(buf1), buf2.str, buf2.count); }
global_ b32 str_contains(const string&  buf1, const char*    buf2) { return str_contains(buf1.str, buf1.count, buf2, strlen(buf2)); }
global_ b32 str_contains(const char*    buf1, const cstring& buf2) { return str_contains(buf1, strlen(buf1), buf2.str, buf2.count); }
global_ b32 str_contains(const cstring& buf1, const char*    buf2) { return str_contains(buf1.str, buf1.count, buf2, strlen(buf2)); }
global_ b32 str_contains(const char*    buf1, const char*    buf2) { return str_contains(buf1, strlen(buf1), buf2, strlen(buf2)); }


#endif //DESHI_STRING_UTILS_H
