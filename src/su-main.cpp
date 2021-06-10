#include <iostream>

#include "su-lexer.h"
#include "su-parser.h"
#include "su-assembler.h"


token* begin_parse() {
	return nullptr;
}

//TODO setup main to take arguments for multiple files, compiler flags, etc.
int main() {

	array<int> wow;

	FILE* file = fopen("main.su", "r");
	if (!file) {
		std::cout << "ERROR: file not found." << std::endl;
		return 0;
	}

	array<token> tokens = suLexer::lex(file);


	for (token& t : tokens) {
		std::cout << t.str << " " << tokenStrings[t.type] << std::endl;
	}
	std::cout << std::endl;


	AST ast = suParser::parse(tokens);

	string assembly = suAssembler::assemble(ast);

	std::cout << assembly << std::endl;

	int test = 0;

}