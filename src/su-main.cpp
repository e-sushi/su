#include <iostream>

#include "su-assembler.h"
#include "su-parser.h"
#include "su-lexer.h"
#include "utils/string_conversion.h"
#include "graphviz/gvc.h"



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

Agraph_t* gvgraph = 0;
GVC_t* gvc = 0;
u32 colidx = 1;

Agnode_t* make_dot_file(Node* node, Agnode_t* parent) {
	static u32 i = 0;
	i++;
	u32 save = i;
	u32 colsave = colidx;

	string send = node->debug_str;

	Agnode_t* me = agnode(gvgraph, to_string(i).str, 1);
	agset(me, "label", send.str);
	agset(me, "color", to_string(colsave).str);
	
	Agnode_t* ret = me;
	
	if (node->first_child) {
		ret = make_dot_file(node->first_child, me);
		if (node->first_child != node->last_child) {
			ret = me;
		}
	}
	if (node->next != node) { colidx = (colidx + 1) % 11 + 1; make_dot_file(node->next, parent); }

	if (parent) {
		Agedge_t* edge = agedge(gvgraph, parent, me, "", 1);
		agset(edge, "color", to_string(colsave).str);
		//if (node->next != node) { 
		//	agset(edge, "constraint", "false"); 
		//}
	}

	//TODO figure out how to make columns stay in line
	if (ret != me && node->next != node) {
		//Agedge_t* edge = agedge(gvgraph, me, ret, "", 1);
		//agset(edge, "weight", "10");
		//agset(edge, "style", "invis");
		//agset(edge, "constraint", "false");

	}

	return ret;
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

	//for (token& t : tokens) {
	//	std::cout << t.str << " " << tokenStrings[t.type] << std::endl;
	//}
	//std::cout << std::endl;

	Program program;

	std::cout << "parsing" << std::endl;
	suParser::parse(tokens, program);
	std::cout << "parsing finished" << std::endl;

	gvc = gvContext();
	gvgraph = agopen("ast tree", Agdirected, 0);
	agattr(gvgraph, AGNODE, "fontcolor",   "white");
	agattr(gvgraph, AGNODE, "color",       "1");
	agattr(gvgraph, AGNODE, "shape",       "box");
	agattr(gvgraph, AGNODE, "margins",     "0.08");
	agattr(gvgraph, AGNODE, "width",       "0");
	agattr(gvgraph, AGNODE, "height",      "0");
	agattr(gvgraph, AGNODE, "colorscheme", "rdylbu11");
	agattr(gvgraph, AGEDGE, "color",       "white");
	agattr(gvgraph, AGEDGE, "colorscheme", "rdylbu11");
	agattr(gvgraph, AGEDGE, "style",       "");
	agattr(gvgraph, AGEDGE, "arrowhead",   "none");
	agattr(gvgraph, AGEDGE, "penwidth",    "0.5");
	agattr(gvgraph, AGEDGE, "constraint",  "true");
	agattr(gvgraph, AGRAPH, "bgcolor",     "grey12");
	agattr(gvgraph, AGRAPH, "concentrate", "true");
	agattr(gvgraph, AGRAPH, "splines",     "true");

	Node* node = &program.node;
	make_dot_file(node, 0);
	gvLayout(gvc, gvgraph, "dot");
	gvRenderFilename(gvc, gvgraph, "svg", "ASTgraph.svg");

	std::cout << "assembling" << std::endl;
	string assembly = suAssembler::assemble(program);
	std::cout << "assembling finished" << std::endl;

	//std::cout << assembly << std::endl;

	FILE* out = fopen("out.s", "w");
	fputs(assembly.str, out);
	fclose(in);
	fclose(out);
}