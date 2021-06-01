#pragma once
#include <initializer_list>
#include "misc.h"

//TODO make this work like vector
template<class T>
struct array {
	T* items;
	int size = 0;
	int allocated_size = 0;
	int itemsize = 0;

	void* first = nullptr;
	void* last  = nullptr;
	void* max   = nullptr;

	//small helper thing
	int roundUp(int numToRound, int multiple) {
		if (multiple == 0) return numToRound;

		int remainder = numToRound % multiple;
		if (remainder == 0) return numToRound;

		return numToRound + multiple - remainder;
	}

	array(int size) {
		itemsize = sizeof(T);
		items = (T*)calloc(size, itemsize);
		this->size = size;
	}

	array(std::initializer_list<T> l) {

		itemsize = sizeof(T);
		for (auto& v : l) size++;
		items = (T*)calloc(size, itemsize);
		

		int index = 0;
		for (auto& v : l) {
			int he = index * itemsize;
			T* nu = new(items + index) T(v);
			index++;
		}
	}

	//TODO this can probably be much better
	//its necessary so when we return objs the entire array copies properly
	//so we have to make sure everything in the array gets recreated
	array(const array<T>& array) {
		itemsize = array.itemsize;
		size = array.size;
		items = (T*)calloc(size, itemsize);

		for (int i = 0; i < array.size; i++) {
			new(items + i) T(array.items[i]);
		}
		//memcpy(items, array.items, itemsize * size);
	}

	~array() {
		for (int i = 0; i < size; i++) {
			items[i].~T();
		}
		free(items);
	}

	void operator = (array<T>& array) {

	}

	
	void add(T t) {
		if (max - last == 0) {
			realloc(items, size * itemsize + itemsize);
			first = items;
		}





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
};