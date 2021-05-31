#pragma once
#include <initializer_list>

template<class T>
struct array {
	T* items;
	int size = 0;
	int itemsize = 0;

	//array(std::initializer_list<T> l);
	//~array();
	//
	//T& operator[](int i);

	//small helper thing
	int roundUp(int numToRound, int multiple) {
		if (multiple == 0) return numToRound;

		int remainder = numToRound % multiple;
		if (remainder == 0) return numToRound;

		return numToRound + multiple - remainder;
	}

	array(int size) {
		itemsize = sizeof(T);
		items = (T*)calloc(size, sizeof(T));
		this->size = size;
	}

	array(std::initializer_list<T> l) {

		itemsize = sizeof(T);
		for (auto& v : l) size++;
		items = (T*)calloc(size, sizeof(T));


		int index = 0;
		for (auto& v : l) {
			int he = index * itemsize;
			T* nu = new(items + index) T(v);
			index++;
		}
	}

	~array() {
		for (int i = 0; i < size; i++) {
			items[i].~T();
		}
		free(items);
	}

	T& operator[](int i) {
		return items[i];
	}
};