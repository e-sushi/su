#include <iostream>

#include "su-assembler.h"
#include "su-parser.h"
#include "su-lexer.h"


/*
General TODOs

convert the assembler to take in instruction and register enums and then decide what instruction to put
based on what architecture we're compiling for

add the bonus operators in https://norasandler.com/2018/01/08/Write-a-Compiler-5.html

detect if we're checking a variable against a value when doing equal/not-equal checks, you can 
just compare the location on the stack with the value directly instead of still having
to store its value first like you would if comparing 2 numbers

reduce the size of ASTs by not placing nodes unless you have to eg. a number literal
by itself doesnt need to go through all the expression nodes for us to know its 
a number literal

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