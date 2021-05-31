#pragma once
#include <initializer_list>
#include "Tracy.hpp"

template<class T>
struct array {
	T* items;
	int size = 0;
	int itemsize = 0;

	//small helper thing
	int roundUp(int numToRound, int multiple) {
		if (multiple == 0) return numToRound;

		int remainder = numToRound % multiple;
		if (remainder == 0) return numToRound;

		return numToRound + multiple - remainder;
	}

	array(int size) {
		ZoneScoped;
		itemsize = sizeof(T);
		items = (T*)calloc(size, sizeof(T));
		TracyAlloc(items, sizeof(roundUp(size * itemsize, 4)));
		this->size = size;
	}

	array(std::initializer_list<T> l) {

		ZoneScoped;
		itemsize = sizeof(T);
		for (auto& v : l) size++;
		items = (T*)calloc(size, sizeof(T));
		TracyAlloc(items, sizeof(roundUp(size * itemsize, 4)));


		int index = 0;
		for (auto& v : l) {
			int he = index * itemsize;
			T* nu = new(items + index) T(v);
			index++;
		}
	}

	~array() {
		ZoneScoped;
		for (int i = 0; i < size; i++) {
			items[i].~T();
		}
		TracyFree(items);
		free(items);
	}

	T& operator[](int i) {
		return items[i];
	}
};