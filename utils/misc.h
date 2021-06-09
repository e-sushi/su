#pragma once
#ifndef SU_MISC_H
#define SU_MISC_H

#include <assert.h>

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

#endif