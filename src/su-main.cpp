#include <iostream>

#include "su-lexer.h"
#include "su-parser.h"
#include "su-assembler.h"


/*
General TODOs

convert the assembler to take in instruction and register enums and then decide what instruction to put
based on what architecture we're compiling for



*/


//TODO setup main to take arguments for multiple files, compiler flags, etc.
int main() {

	FILE* in = fopen("main.su", "r");
	if (!in) {
		std::cout << "ERROR: file not found." << std::endl;
		return 0;
	}

	array<token> tokens = suLexer::lex(in);


	for (token& t : tokens) {
		std::cout << t.str << " " << tokenStrings[t.type] << std::endl;
	}
	std::cout << std::endl;

	Program program;

	suParser::parse(tokens, program);

	std::cout << "Parse finished" << std::endl;

	string assembly = suAssembler::assemble(program);

	//std::cout << assembly << std::endl;

	FILE* out = fopen("out.s", "w");
	fputs(assembly.str, out);
	fclose(in);
	fclose(out);

}