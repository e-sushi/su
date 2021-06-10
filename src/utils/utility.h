#pragma once
#ifndef SU_UTILIY_H
#define SU_UTILIY_H

#include "string.h"
#include "array.h"




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

static int stoi(string s) {
	int x;
	sscanf(s.str, "%d", &x);
	return x;
}



#endif