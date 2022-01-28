/*
Return codes are listed in su-types.h

command line arguments with options
-----------------------------------
		-i [str...]
list input .su files
		
		-wl [int] default: 1
		          set warning level
				  :  0: disable all warnings
				  :  1: arthmatic errors; logic errors
				  :  2: saftey errors
				  :  3: often intentional but could be considered a warning
				  :  4: all warnings

		TODO -defstr [option] default: utf8
			      set the default type of str 
				  :  ascii
				  :  utf8
				  :  utf16
				  :  utf32
				  :  ucs2

		-os [str] default: current OS
				  set the platform/OS to compile for
				  :  windows
				  :  linux
				  :  osx

		TODO -arch [str] default: current architecture
				  set the CPU architecture to compile for
				  :  x64 | amd64
				  :  x86 | i386
				  :  arm64
				  :  arm32

		-o [str] default: working directory
				  directory to output the assembly (and other output) files to

		TODO -ep [str] default: main
				  set the name of the entry point function of the program;

		-dw [int...]
				//TODO to make the named array useful, maybe also allow taking in warning names, and maybe give warning simple names
				  disable certain warnings from appearing while compiling
				  list of warnings is in su-warnings.h
				  this can take multiple unsigned ints

command line arguments without options
-----------------------------------
		--sw       suppress warnings
		--sm       suppress messages
		--sa       suppress all printing
		--v        verbose printing of internal actions (actual compiler steps, not warnings, errors, etc. for debugging the compiler)
		TODO --gv       generate graphviz graph of the AST tree (output as .svg)
        TODO --wx       treat warnings as errors

General TODOs
-------------
hash things like struct names, function signatures, and such to prevent tons of string comparing

maybe count the the amount of tokens of each node type we have as we lex, so we can estimate how much storage parser will need to allocate

maybe require -defstr to specify OS versions and distros

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
#include <chrono>

#define TIMER_START(name) std::chrono::time_point<std::chrono::high_resolution_clock> name = std::chrono::high_resolution_clock::now()
#define TIMER_RESET(name) name = std::chrono::high_resolution_clock::now()
#define TIMER_END(name) std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - name).count()

//headers
#include "su-warnings.h"
#include "su-errors.h"
#include "su-types.h"

//source
#include "su-io.cpp"
#include "su-lexer.cpp"
#include "su-preprocessor.cpp"
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
			arg = argv[++i];
			if (str_ends_with(arg, ".su")) {
				filepaths.add(argv[i]);
				//TODO block .subuild files after finding a .su
			}
			else {
				PRINTLN("file with invalid file type passed: " << arg);
				return ReturnCode_File_Invalid_Extension;
			}
		}
		//// @-wl warning level ////
		else if (!strcmp("-wl", arg)) {
			arg = argv[++i];
			globals.warning_level = stoi(argv[i]);
			//TODO handle invalid arg here
		}
		//// @-os output OS ////
		else if (!strcmp("-os", arg)) {
			arg = argv[++i];
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
			arg = argv[++i];
			output_dir = arg;
			if(output_dir[output_dir.count-1] != '\\' && output_dir[output_dir.count-1] != '/'){
				output_dir += "/";
			}
		}
		//// @-dw disable warnings ////
		else if (!strcmp("-dw", arg)) {
			arg = argv[++i];
			while (1) {
				int warning = stoi(arg);
				if(warning > 0 && warning < WarningCodes_COUNT){
					disabledWC.set(warning);
				}else{
					printf("su : invalid -dw value : There is no warning with code '%d'.\n", warning);
					return ReturnCode_Invalid_Argument;
				}
				if (i == argc - 1) break;
				arg = argv[++i];
				if (!isnumber(arg[0])) { i--; break; }
			}
		}
		//// @--v verbose printing ////
		else if (!strcmp("--v", arg)) {
			globals.verbose_print = true;
		}
		//// @--sw supress warnings ////
		else if (!strcmp("--sw", arg)) {
			globals.supress_warnings = true;
		}
		//// @--sm supress messages ////
		else if (!strcmp("--sm", arg)) {
			globals.supress_messages = true;
		}
		//// @--sa supress all printing ////
		else if (!strcmp("--sa", arg)) {
			globals.supress_warnings = true;
			globals.supress_messages = true;
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
		u32 error_code = 0;
		
		//TODO dont do this so we support relative paths
		FilePath filepath(cstring{raw_filepath.str, raw_filepath.count});
		cstring name_dot_ext = {filepath.filename.str, filepath.filename.count + 1 + filepath.extension.count};
		
		string source = load_file(raw_filepath.str);
		if(!source) return ReturnCode_File_Not_Found;
		
		//////////////////////////////////////////////////////////////////////////////////////////////////
		//// Lexing
		log("verbose", "lexing started");
		TIMER_START(timer);
		error_code = lex_file(name_dot_ext, source);
		if(error_code != EC_Success){ PRINTLN("ERROR: lexer failed"); return ReturnCode_Lexer_Failed; }
		log("verbose", "lexing took ", TIMER_END(timer)," ms");
		log("verbose", "lexing finished");
		
		/*
		forE(lexer.tokens){
			if(it->type == Token_LiteralFloat){
				printf("type: %s, val: %f\n", TokenTypes_Names[it->type], it->float_value);
			}else if(it->type == Token_LiteralInteger){
				printf("type: %s, val: %lld\n", TokenTypes_Names[it->type], it->int_value);
			}else{
				printf("type: %s, val: %.*s\n", TokenTypes_Names[it->type], int(it->raw.count), it->raw.str);
			}
		}
*/
		
		preprocessor.preprocess();

		//////////////////////////////////////////////////////////////////////////////////////////////////
		//// Parsing
		Program program;
		program.filename = name_dot_ext;
		log("verbose", "parsing started");
		TIMER_RESET(timer);
		error_code = parser.parse_program(program);
		if(error_code != EC_Success){ PRINTLN("ERROR: parser failed"); return ReturnCode_Parser_Failed; }
		log("verbose", "parsing took ", TIMER_END(timer), " ms");
		log("verbose", "parsing finished");
		
		string output_graph_path = output_dir + filepath.filename + ".svg";
		generate_ast_graph_svg(output_graph_path.str, program);
		generate_ast_graph_svg("parserdebugtree.svg", &ParserDebugTree);

		
		//////////////////////////////////////////////////////////////////////////////////////////////////
		//// Assembling
		log("verbose", "assembling started");
		string assembly;
		TIMER_RESET(timer);
		error_code = assemble_program(program, assembly);
		if(error_code != EC_Success){ PRINTLN("ERROR: assembler failed"); return ReturnCode_Assembler_Failed; }
		log("verbose", "assembling took ", TIMER_END(timer), " ms");
		log("verbose", "assembling finished");
		
		string output_asm_path = output_dir + filepath.filename + ".s";
		b32 success = write_file(output_asm_path.str, assembly);
		if(!success) return ReturnCode_File_Locked;
		
		//print successfully compiled file
		printf("%s\n", filepath.filename.str); //NOTE printf will go until \0 which happens to include the extension
	}
	return ReturnCode_Success;
}