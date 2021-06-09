#pragma once
#include <initializer_list>
#include "misc.h"

template<class T>
struct array {
	T* items;
	//int size = 0; //num items in array
	int space = 0; //total space array has allocated
	int itemsize = 0;

	typedef T* iterator;
	typedef const T* const_iterator;

	T* first = nullptr;
	T* last  = nullptr;
	T* max   = nullptr;

	//small helper thing
	int roundUp(int numToRound, int multiple) {
		if (multiple == 0) return numToRound;

		int remainder = numToRound % multiple;
		if (remainder == 0) return numToRound;

		return numToRound + multiple - remainder;
	}

	array(int size) {
		space = size;
		itemsize = sizeof(T);
		items = (T*)calloc(size, itemsize);
		first = items;
		last = items - 1; //could break things but it makes add work 
		max = items + size - 1;
	}

	array(std::initializer_list<T> l) {
		int size = 0;

		itemsize = sizeof(T);
		for (auto& v : l) size++;
		items = (T*)calloc(size, itemsize);
		
		first = items;

		int index = 0;
		for (auto& v : l) {
			int he = index * itemsize;
			T* nu = new(items + index) T(v);
			index++;
		}

		last = &items[size - 1];
		max = last;
	}

	//TODO this can probably be much better
	//its necessary so when we return objs the entire array copies properly
	//so we have to make sure everything in the array gets recreated
	array(const array<T>& array) {
		itemsize = array.itemsize;
		space = array.space;
		items = (T*)calloc(array.space, itemsize);

		first = items;
		last = items + array.size();
		max = items + space - 1;

		int i = 0;
		for (T item : array) {
			new(items + i) T(item);
			i++;
		}

	}

	~array() {
		for (int i = 0; i < size(); i++) {
			items[i].~T();
		}
		free(items);
	}

	int size() const {
		return (int)(last - first);
	}

	void operator = (array<T>& array) {

	}
	
	void add(T t) {
		//if array is full, realloc the memory and extend it to accomodate the new item
		if (max - last == 0) {
			space = roundUp(size() * itemsize + itemsize, itemsize);
			realloc(items, space);
			max = items + space;
			//for (T* i = last + 1; i < max; i++) {
			//	memset(i, 0, itemsize);
			//}
			first = items;
			last = items + size() + 1;
			new(last) T(t);
		}
		else {
			last++;
			new(last) T(t);
		}
	}

	void remove(int i) {
		assert(size() > 0); "can't remove element from empty vector";
		assert(i < size()); "index is out of bounds";
		memset(items + i, 0, itemsize);
		for (int o = i; o < size(); o++) {
			items[o] = items[o + 1];
		}
		memset(last, 0, itemsize);
		last--;
	}

	//allows for dynamic growth
	void append(T t) {

	}

	//this is really only necessary for the copy constructor as far as i know
	T& at(int i) {
		return items[i];
	}

	T& operator[](int i) {
		return items[i];
	}

	//begin/end functions for for each loops
	iterator begin() { return &items[0]; }
	iterator end()   { return &items[size()]; }
	const_iterator begin() const { return &items[0]; }
	const_iterator end()   const { return &items[size()]; }


};