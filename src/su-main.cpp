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

-o [str] default: working directory
directory to output the .asm (and other output) files to

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

int main(int argc, char* argv[]) { //NOTE argv includes the entire command line (including .exe)
	if (argc < 2) {
		PRINTLN("ERROR: no arguments passed");
		return ReturnCode_No_File_Passed;
	}
	
	//make this not array and string later maybe 
	array<string> filenames;
	string output_dir = "";
	for(int i=1; i<argc; ++i) {
		char* arg = argv[i];
		if (!strcmp("-i", arg)) { //////////////////////////////////////////////////// @-i
			i++; arg = argv[i];
			if (str_ends_with(arg, ".su")) {
				filenames.add(argv[i]);
				//TODO block .subuild files after finding a .su
				//NOTE maybe actually allow .subuild files to be used with different combinations of .su files?
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
		else if (!strcmp("-wl", arg)) { ////////////////////////////////////////////// @-wl
			i++; arg = argv[i];
			globals.warning_level = stoi(argv[i]);
			//TODO handle invalid arg here
		}
		else if (!strcmp("-os", arg)) { ////////////////////////////////////////////// @-os
			i++; arg = argv[i];
			if      (!strcmp("windows", arg)) { globals.osout = OSOut_Windows; }
			else if (!strcmp("linux",   arg)) { globals.osout = OSOut_Linux; }
			else if (!strcmp("osx",     arg)) { globals.osout = OSOut_OSX; }
			else {
				PRINTLN("ERROR: invalid argument");
				return ReturnCode_Invalid_Argument;
			}
		}
		else if (!strcmp("-o", arg)) { ////////////////////////////////////////////// @-o
			i++; arg = argv[i];
			output_dir = arg;
			if(output_dir[output_dir.count-1] != '\\' && output_dir[output_dir.count-1] != '/'){
				output_dir += "/";
			}
		}
		else {
			PRINTLN("ERROR: invalid argument: '" << arg << "'");
			return ReturnCode_Invalid_Argument;
		}
	}
	//TODO dont do this so we support relative paths
	cstring filename_raw{filenames[0].str, filenames[0].count}; //just the name, no extention or path
	u32 last_slash = find_last_char<'/', '\\'>(filename_raw);
	if (last_slash != npos) {
		filename_raw.str += last_slash+1;
		filename_raw.count -= last_slash + 3;
	}
	string filename(filename_raw);
	
	FILE* in = fopen(filenames[0].str, "r");
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
	string output_graph_path = output_dir + filename + ".svg";
	gvRenderFilename(gvc, gvgraph, "svg", output_graph_path.str);
	
	PRINTLN("assembling");
	string assembly;
	if (!suAssembler::assemble(program, assembly)) {
		PRINTLN("assembler failed");
		return ReturnCode_Assembler_Failed;
	}
	PRINTLN("assembling finished");
	
	string output_asm_path = output_dir + filename + ".s";
	FILE* out = fopen(output_asm_path.str, "w");
	if (!out) {
		PRINTLN("ERROR: failed to open output file.");
		return ReturnCode_File_Locked;
	}
	
	fputs(assembly.str, out);
	
	fclose(in);
	fclose(out);
	return ReturnCode_Success;
}