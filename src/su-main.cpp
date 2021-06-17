#include <iostream>

#include "su-lexer.h"
#include "su-parser.h"
#include "su-assembler.h"

//TODO setup main to take arguments for multiple files, compiler flags, etc.
int main() {

	FILE* in = fopen("main.su", "r");
	if (!in) {
		std::cout << "ERROR: file not found." << std::endl;
		return 0;
	}

	array<token> tokens = suLexer::lex(in);


	//for (token& t : tokens) {
	//	std::cout << t.str << " " << tokenStrings[t.type] << std::endl;
	//}
	//std::cout << std::endl;

	Program program;

	suParser::parse(tokens, program);

	string assembly = suAssembler::assemble(program);

	//std::cout << assembly << std::endl;

	FILE* out = fopen("out.s", "w");
	fputs(assembly.str, out);
	fclose(in);
	fclose(out);
}