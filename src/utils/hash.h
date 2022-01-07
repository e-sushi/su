#pragma once
#ifndef DESHI_HASH_H
#define DESHI_HASH_H

#include "array.h"
#include "string.h"
#include "defines.h"

template<class T>
struct hash {
	
	hash() {};
	
	inline u32 operator()(const T& v)const {
		u32 seed = 2166136261;
		size_t data_size = sizeof(T);
		const u8* data = (const u8*)&v;
		while (data_size-- != 0) {
			seed ^= *data++;
			seed *= 16777619;
		}
		return seed;
		
	}
	
	inline u32 operator()(T* v)const {
		u32 seed = 2166136261;
		size_t data_size = sizeof(T);
		const u8* data = (const u8*)v;
		while (data_size-- != 0) {
			seed ^= *data++;
			seed *= 16777619;
		}
		return seed;
		
	}
};

template<> 
struct hash<string> {
	inline u32 operator()(const string& s) {
		u32 seed = 2166136261;
		u32 size = s.count+1;
		while (size-- != 0) {
			seed ^= s.str[size];
			seed *= 16777619;
		}
		return seed;
	}
};

template<> 
struct hash<const char*> {
	inline u32 operator()(const char* s) {
		u32 seed = 2166136261;
		u32 size = strlen(s)+1;
		while (size-- != 0) {
			seed ^= s[size];
			seed *= 16777619;
		}
		return seed;
	}
};

template<class T> 
struct hash<array<T>> {
	inline u32 operator()(array<T>* s) {
		u32 seed = 2166136261;
		u32 size = s->size;
		while (size-- != 0) {
			seed ^= s->operator[](size - 1);
			seed *= 16777619;
		}
		return seed;
	}
};
#endif