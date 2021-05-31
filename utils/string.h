#pragma once
#include <stdlib.h>
#include <iostream>

//TODO this will probably break somewhere and could use some functionality
//it also needs to store the characters on it somehow, but im not sure how yet, if its even possible
struct string {
	char* str;
	int size = 0;

	string() {}
	string(const char c);
	string(const char* s);
	string(const string& s);

	~string();

	char& operator[](int i);
	void operator = (const char* s);
	bool operator == (string& s);
	void operator += (string& s);
	void operator += (char& c);

	void clear();

	long long hash();

	//void* operator new(size_t size, void* place);
	//void operator delete(void* ptr) noexcept;

};


inline std::ostream& operator<<(std::ostream& os, string& m) {
	return os << m.str;
}
