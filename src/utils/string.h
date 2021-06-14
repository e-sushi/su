#pragma once
#include <stdlib.h>
#include <iostream>

struct string {
	char* str;
	int size = 0;

	string() {}
	string(const char c);
	string(const char* s);
	string(const string& s);

	~string();

	char& operator[](int i);
	void operator = (char c);
	void operator = (string s);
	void operator = (const char* s);
	bool operator == (string& s);
	bool operator == (const char* c);
	void operator += (const char* c);
	void operator += (string s);
	void operator += (char& c);
	string operator + (string& s);
	string operator + (const char* c);
	friend string operator + (const char* c, string& s);
	void clear();

	long long hash();

	//void* operator new(size_t size, void* place);
	//void operator delete(void* ptr) noexcept;

};


inline std::ostream& operator<<(std::ostream& os, string& m) {
	return os << m.str;
}

