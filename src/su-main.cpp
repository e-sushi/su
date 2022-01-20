/*


Return codes are listen in su-types.h

.subuild files are detailed below, however if you're using one, the following flags do not need to be specified
 on the command line and can be done in the build file

command line arguments:
		-i        input files; either a collection of .su files or a .subuild file
		
		-wl [int] default: 1
		          set warning level:
				  :  0: disable all warnings
				  :  1: arthmatic errors; logic errors
				  :  2: saftey errors
				  :  3: idk yet
				  :  4: all warnings

		-defstr [option] default: utf8
			      set the default type of str 
				  :  ascii
				  :  utf8
				  :  utf16
				  :  utf32

		-os [str] default: auto-detected
				  set the platform/OS to compile for; 
				  :  windows //maybe need options for different releases?
				  :  linux
				  :  osx

		-ep [str] default: main 
				set the name of the entry point function of the program; 

		-sw       suppress warnings
		-se       suppress errors
		-sm       suppress messages
		-sa       suppress all printing
		-v        verbose printing of internal actions (actual compiler steps, not warnings, errors, etc. for debugging the compiler)
		-gv       generate graphviz graph of the AST tree (output as .svg)

.subuild specific
		-conf [str] default: none
				  use a specific configuration specified in a subuild file 

subuild files:
	TODO write about this when we have it :)



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

//utils
#include "utils/array.h"
#include "utils/array_utils.h"
#include "utils/carray.h"
#include "utils/cstring.h"
#include "utils/defines.h"
#include "utils/hash.h"
#include "utils/map.h"
#include "utils/pair.h"
#include "utils/string.h"
#include "utils/string_utils.h"
#include "utils/unicode.h"
#include "utils/utils.h"

//libs
#include <iostream>
#include "graphviz/gvc.h"

//headers
#include "su-lexer.h"
#include "su-parser.h"
#include "su-assembler.h"

//source
#include "su-lexer.cpp"
#include "su-parser.cpp"
#include "su-assembler.cpp"

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
int main(int argc, char* argv[]) {
	if (!argc) {
		PRINTLN("ERROR: no arguments passed");
		return ReturnCode_No_File_Passed;
	}

	//make this not array and string later maybe 
	array<string> filenames;
	forI(argc) {
		char* arg = argv[i];
		if (!strcmp("-i", arg)) { //////////////////////////////////// @-i
			i++; arg = argv[i];
			if (str_ends_with(arg, ".su")) {
				filenames.add(argv[i]);
				//TODO block .subuild files after finding a .su
			}
			else if (str_ends_with(arg, ".subuild")) {
				//TODO build file parsing
				//TODO block .su files after finding a build file
			}
			else {
				PRINTLN("file with invalid file type passed: " << arg);
				return ReturnCode_File_Invalid_Extension;
			}
		}
		else if (!strcmp("-wl", arg)) { ///////////////////////////// @-wl
			i++; arg = argv[i];
			globals.warning_level = stoi(argv[i]);
			//TODO handle invalid arg here
		}
		else if (!strcmp("-os", arg)) { ////////////////////////////// @-os
			i++; arg = argv[i];
			if      (!strcmp("windows", arg)) { globals.osout = OSOut_Windows; }
			else if (!strcmp("linux",   arg)) { globals.osout = OSOut_Linux; }
			else if (!strcmp("osx",     arg)) { globals.osout = OSOut_OSX; }
			else {
				PRINTLN("ERROR: invalid argument");
				return ReturnCode_Invalid_Argument;
			}
		}
		else {
			//ignore invalid args for cuz vs is silly and puts the path of the exe for wahtever reason
			//PRINTLN("ERROR: invalid argument");
			//return ReturnCode_Invalid_Argument;
		}
	}
	FILE* in = fopen("main.su", "r");
	if (!in) {
		PRINTLN("ERROR: file not found.");
		return ReturnCode_File_Not_Found;
	}

	array<token> tokens;
	PRINTLN("lexing");
	if (!suLexer::lex(in, tokens)) {
		PRINTLN("lexer failed");
		return ReturnCode_Lexer_Failed;
	}
	PRINTLN("lexing finished");

	Program program;
	PRINTLN("parsing");
	if (suParser::parse(tokens, program)) {
		PRINTLN("parsing failed");
		return ReturnCode_Parser_Failed;
	}
	PRINTLN("parsing finished");

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

	make_dot_file(&program.node, 0);
	gvLayout(gvc, gvgraph, "dot");
	gvRenderFilename(gvc, gvgraph, "svg", "ASTgraph.svg");

	PRINTLN("assembling");
	string assembly;
	if (!suAssembler::assemble(program, assembly)) {
		PRINTLN("assembler failed");
		return ReturnCode_Assembler_Failed;
	}
	PRINTLN("assembling finished");

	//std::cout << assembly << std::endl;

	FILE* out = fopen("out.s", "w");
	fputs(assembly.str, out);
	fclose(in);
	fclose(out);

	return ReturnCode_Success;
}