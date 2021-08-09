#include <iostream>

#include "su-lexer.h"
#include "su-parser.h"
#include "su-assembler.h"


/*
General TODOs

convert the assembler to take in instruction and register enums and then decide what instruction to put
based on what architecture we're compiling for


add the bonus operators in https://norasandler.com/2018/01/08/Write-a-Compiler-5.html


*/


//TODO setup main to take arguments for multiple files, compiler flags, etc.
int main() {

	FILE* in = fopen("main.su", "r");
	if (!in) {
		std::cout << "ERROR: file not found." << std::endl;
		return 0;
	}

	std::cout << "lexing" << std::endl;
	array<token> tokens = suLexer::lex(in);
	std::cout << "lexing finished" << std::endl;

	for (token& t : tokens) {
		std::cout << t.str << " " << tokenStrings[t.type] << std::endl;
	}
	std::cout << std::endl;

	Program program;

	std::cout << "parsing" << std::endl;
	suParser::parse(tokens, program);
	std::cout << "parsing finished" << std::endl;

	std::cout << "assembling" << std::endl;
	string assembly = suAssembler::assemble(program);
	std::cout << "assembling finished" << std::endl;

	//std::cout << assembly << std::endl;

	FILE* out = fopen("out.s", "w");
	fputs(assembly.str, out);
	fclose(in);
	fclose(out);




}