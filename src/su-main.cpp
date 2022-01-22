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

Hash things like struct names, function signatures, and such to prevent tons of string comparing

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

//headers
#include "su-lexer.h"
#include "su-parser.h"
#include "su-assembler.h"

//source
#include "su-io.cpp"
#include "su-lexer.cpp"
#include "su-parser.cpp"
#include "su-assembler.cpp"
#include "su-ast-graph.cpp"

int main(int argc, char* argv[]) { //NOTE argv includes the entire command line (including .exe)
	//////////////////////////////////////////////////////////////////////////////////////////////////
	//// Command Line Arguments
	if (argc < 2) {
		PRINTLN("ERROR: no arguments passed");
		return ReturnCode_No_File_Passed;
	}

	//make this not array and string later maybe 
	array<string> filepaths;
	string output_dir = "";
	for(int i=1; i<argc; ++i) {
		char* arg = argv[i];
		//// @-i files to compile ////
		if (!strcmp("-i", arg)) {
			i++; arg = argv[i];
			if (str_ends_with(arg, ".su")) {
				filepaths.add(argv[i]);
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
		//// @-wl warning level ////
		else if (!strcmp("-wl", arg)) {
			i++; arg = argv[i];
			globals.warning_level = stoi(argv[i]);
			//TODO handle invalid arg here
		}
		//// @-os output OS ////
		else if (!strcmp("-os", arg)) {
			i++; arg = argv[i];
			if      (!strcmp("windows", arg)) { globals.osout = OSOut_Windows; }
			else if (!strcmp("linux",   arg)) { globals.osout = OSOut_Linux; }
			else if (!strcmp("osx",     arg)) { globals.osout = OSOut_OSX; }
			else {
				PRINTLN("ERROR: invalid argument");
				return ReturnCode_Invalid_Argument;
			}
		}
		//// @-o output directory ////
		else if (!strcmp("-o", arg)) {
			i++; arg = argv[i];
			output_dir = arg;
			if(output_dir[output_dir.count-1] != '\\' && output_dir[output_dir.count-1] != '/'){
				output_dir += "/";
			}
		}
		else if (!strcmp("-v", arg)) {
			globals.verbose_print = true;
		}
		else {
			PRINTLN("ERROR: invalid argument: '" << arg << "'");
			return ReturnCode_Invalid_Argument;
		}
	}
	
	//check that a file was passed
	if(filepaths.count == 0){
		PRINTLN("ERROR: no files passed");
		return ReturnCode_No_File_Passed;
	}
	
	for(const string& raw_filepath : filepaths){ //NOTE MULTIPLE FILES DOESNT ACTUALLY WORK YET
		//TODO dont do this so we support relative paths
		FilePath filepath(cstring{raw_filepath.str, raw_filepath.count});
		
		string source = load_file(raw_filepath.str);
		if(!source) return ReturnCode_File_Not_Found;
		
		//////////////////////////////////////////////////////////////////////////////////////////////////
		//// Lexing
		array<token> tokens;
		if(globals.verbose_print) PRINTLN("lexing started");
		if(!suLexer::lex(source, tokens)){
			PRINTLN("ERROR: lexer failed");
			return ReturnCode_Lexer_Failed;
		}
		if(globals.verbose_print) PRINTLN("lexing finished");
		
		//////////////////////////////////////////////////////////////////////////////////////////////////
		//// Parsing
		Program program;
		if(globals.verbose_print) PRINTLN("parsing started");
		if(suParser::parse(tokens, program)){
			PRINTLN("ERROR: parser failed");
			return ReturnCode_Parser_Failed;
		}
		if(globals.verbose_print) PRINTLN("parsing finished");
		
		string output_graph_path = output_dir + filepath.filename + ".svg";
		generate_ast_graph_svg(output_graph_path.str, program);
		
		//////////////////////////////////////////////////////////////////////////////////////////////////
		//// Assembling
		if(globals.verbose_print) PRINTLN("assembling started");
		string assembly;
		if(!suAssembler::assemble(program, assembly)){
			PRINTLN("ERROR: assembler failed");
			return ReturnCode_Assembler_Failed;
		}
		if(globals.verbose_print) PRINTLN("assembling finished");
		
		string output_asm_path = output_dir + filepath.filename + ".s";
		b32 success = write_file(output_asm_path.str, assembly);
		if(!success) return ReturnCode_File_Locked;
		
		//print successfully compiled file
		printf("%s\n", filepath.filename.str); //NOTE printf will go until \0 which happens to include the extension
	}
	return ReturnCode_Success;
}