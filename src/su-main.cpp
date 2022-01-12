#include <iostream>

#include "su-assembler.h"
#include "su-parser.h"
#include "su-lexer.h"
#include "utils/string_conversion.h"


//NOTE when doing stuff with trees add the node FIRST and reference it inside of it's array
//	   this way you don't have a node with 20 million children being added to a single node
//     and having to copy all of it 

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

u32 i = 0;
FILE* graph = 0;

void compilegraph(Node* node, u32 parent) {
	i++;
	u32 save = i;

	string send = node->debug_str;
	send.replace('&', "&amp;");
	send.replace('<', "&lt;");
	send.replace('>', "&gt;");

	fputs(toStr(save, "[label=<<font color=\"#ffffff\">", send, "</font>>", "shape=box] ", '\n').str, graph);
	fflush(graph);
	Node* stage = node;

	if (stage->first_child)   compilegraph(stage->first_child, save);
	if (stage->next != stage) compilegraph(stage->next, parent);

	fputs(toStr(parent, " -- ", save, "[color=\"white\" labelfontcolor=\"white\"]", '\n').str, graph);
	fflush(graph);
}


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

	graph = fopen("ASTgraph.dot", "w");
	 fputs("graph ast {\nbgcolor=\"black\"\n", graph);

	Node* node = &program.node;
	compilegraph(node, 0);
	fputs("}", graph);
	fclose(graph);

	std::cout << "assembling" << std::endl;
	string assembly = suAssembler::assemble(program);
	std::cout << "assembling finished" << std::endl;

	//std::cout << assembly << std::endl;

	FILE* out = fopen("out.s", "w");
	fputs(assembly.str, out);
	fclose(in);
	fclose(out);
}