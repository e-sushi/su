#pragma once
#ifndef SU_MISC_H
#define SU_MISC_H

//assert
#ifndef NDEBUG
#define ASSERT(condition, message) \
do { \
if (! (condition)) { \
std::string file = __FILENAME__; \
std::cout << "Assertion '" #condition "' failed in " + file + " line " + std::to_string(__LINE__) + ": \n" #message << std::endl;  \
std::terminate(); \
} \
} while (false)
#else
#   define ASSERT(condition, message) condition;
#endif


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