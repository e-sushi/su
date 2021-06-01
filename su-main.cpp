#include <iostream>

#include "su-lexer.h"

token* begin_parse() {
	return nullptr;
}

//TODO setup main to take arguments for multiple files, compiler flags, etc.
int main() {


	

	FILE* file = fopen("main.su", "r");
	if (!file) {
		std::cout << "ERROR: file not found." << std::endl;
		return 0;
	}

	array<token> tokens = suLexer::lex(file);

	for (int i = 0; i < tokens.size; i++) {
		std::cout << tokens[i].str << std::endl;
	}


}