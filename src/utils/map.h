#pragma once
#ifndef DESHI_MAP_H
#define DESHI_MAP_H

#include "hash.h"
#include "array.h"
#include "tuple.h"

template<typename Key, typename Value, typename HashStruct = hash<Key>>
struct map {
	array<pair<Key, Value>> data;
	u32 count = 0;

	map() {}

	map(std::initializer_list<pair<Key, Value>> list) {
		for (auto& p : list) {
			add(p.first, p.second);
		}
	}

	void clear() {
		data.clear();
		count = 0;
	}

	bool has(const Key& key) {
		for (auto& p : data)
			if (p.first == key) return true;
		return false;
	}

	Value* at(const Key& key) {
		for (auto& p : data)
			if (p.first == key) return &p.second;
		return 0;
	}

	Value* atIdx(u32 index) {
		return &data[index];
	}

	//returns index of added or existing key
	u32 add(const Key& key) {
		forI(data.count) { if (data[i].first == key) { return i; } }
		Value v{};
		data.add(make_pair(key, v));
		count++;
		return count - 1;
	}

	u32 add(const Key& key, const Value& value) {
		forI(data.count) { if (data[i].first == key) { return i; } }
		data.add(make_pair(key, value));
		count++;
		return count - 1;
	}

	Value& operator[] (const Key& key) {
		for (auto& p : data)
			if (key == p.first)
				return p.second;
		Value v{};
		data.add(make_pair(key, v));
		count++;
		return data.last->second;
	}

	void swap(u32 idx1, u32 idx2) {
		data.swap(idx1, idx2);
	}

	Value* begin() { return data.begin(); }
	Value* end() { return data.end(); }
	const Value* begin() const { return data.begin(); }
	const Value* end()   const { return data.end(); }
};

//TODO(delle) make this sorted so its faster
template<typename Key, typename Value, typename HashStruct = hash<Key>>
struct hashmap {
	array<u32>   hashes;
	array<Value> data;
	u32 count = 0;
	
	hashmap() {}

	hashmap(std::initializer_list<pair<Key,Value>> list) {
		for (auto p : list) {
			add(p.first, p.second);
		}
	}

	void clear() {
		hashes.clear();
		data.clear();
		count = 0;
	}
	
	bool has(const Key& key) {
		u32 hashed = HashStruct{}(key);
		forI(hashes.count){ if(hashed == hashes[i]){ return true; } }
		return false;
	}

	Value* at(const Key& key) {
		u32 hashed = HashStruct{}(key);
		forI(hashes.count){ if(hashed == hashes[i]){ return &data[i]; } }
		return 0;
	}

	Value* atIdx(u32 index){
		return &data[index];
	}
	
	//returns index of added or existing key
	u32 add(const Key& key){
		u32 hashed = HashStruct{}(key);
		forI(hashes.count){ if(hashed == hashes[i]){ return i; } }
		hashes.add(hashed);
		data.add(Value());
		count++;
		return count-1;
	}
	
	u32 add(const Key& key, const Value& value){
		u32 hashed = HashStruct{}(key);
		forI(hashes.count){ if(hashed == hashes[i]){ return i; } }
		hashes.add(hashed);
		data.add(value);
		count++;
		return count-1;
	}
	
	void swap(u32 idx1, u32 idx2) {
		hashes.swap(idx1, idx2);
		data.swap(idx1, idx2);
	}
	
	Value* begin() { return data.begin(); }
	Value* end()   { return data.end(); }
	const Value* begin() const { return data.begin(); }
	const Value* end()   const { return data.end(); }
};

template<typename Key, typename HashStruct = hash<Key>>
using set = hashmap<Key,Key,HashStruct>;

#endif