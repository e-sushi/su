#pragma once
#ifndef SU_UTILIY_H
#define SU_UTILIY_H

#include "string.h"
#include "array.h"
#include "tuple.h"




//NOTE: I'm not sure if i want this in a namespace yet

template<class T>
static int is_in(T& c, T array[]) {
	int size = sizeof(array);
	int size2 = sizeof(T);
	for (int i = 0; i < size; i++) {
		if (c == array[i]) return 1;
	}
	return 0;
}

template<class T>
static int is_in(T& c, array<T>& array) {
	for (T t : array) { if (t == c) return 1; }
	return 0;
}

template<class T, class S>
static int is_in(T& key, array<pair<T, S>>& array) {
	for (pair<T, S> p : array) if (p.first == key) return 1;
	return 0;
}

//get a value from a key in an array of pairs of T and S
template<class T, class S>
static S& vfk(T& key, array<pair<T, S>>& array) {
	for (pair<T, S>& p : array) {
		if (p.first == key) {
			return p.second;
		}
	}
	throw "no key";
}



static int stoi(string s) {
	int x;
	sscanf(s.str, "%d", &x);
	return x;
}





#endif