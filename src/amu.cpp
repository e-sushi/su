/* amu

	NOTE(sushi) as of 2022/08/06, a lot of the stuff explained below is not implemented but should serve as an idea for what amu should be when it is mature.
                some of the purpose is to just write this stuff out to see if it makes sense to explain to someone who has never heard of amu before.

	Introduction
	------------

	This is the compiler for the programming language amu. The name of the language is subject to change, but the compiler's name is also amu.
	
	あむ (kanji: 編む, romanji: amu) literally means "to compile," or "to edit." It also means to knit, to plait, and to braid (source is Jisho). The name was chosen as an
	alternative to our original name, su. su didn't work because it conflicted with the built in linux command su, and I personally didn't like it because 
	it was originally chosen when I (sushi) first started working on su and couldnt think of anything else and it felt narcissitic to partially name it after myself. 
	eventually. I like this name for the compiler because of its meaning, but would like to look into other names for the language itself.

	amu is open to pull requests, issues, and new trusted contributors (as in people who can push whatever they want) if you work enough on it.

	Currently the source code of the compiler, the various notes within, and messages in our discord server serve as the standard specification for amu. 
	A formal standards document may never be typed up, but it certainly shouldnt be until the compiler and langauge have stabilized.

	amu's syntax and features are rooted in C and C++ and the motivation for its creation is a dislike for the overly complex and strict features of C++
	and a desire to have C with select features from C++, as well as some extra ideas we have gathered. A lot of the syntax mirrors C/C++, but there are some major differences.
	For instance, names in amu are always first in syntax. So things like struct, function, and variable declarations start with their name followed by a colon, then
	its type specifier. The colon has pretty much become the standard syntax for a declaration in amu. This is the main syntax difference, most other things are more
	or less the same.

	There are some features from C++ that we like, but a lot that we didn't. Features from C++ that we decided to keep include:
	* function overloading
	* operator overloading
	* struct field defaults
	* generics (templating)
	and as we go, more may be added. Some features we intend not to include from C++ are things like constructors, deconstructors, and
	member functions. I (sushi) am personally on the fence about member functions because I think they are useful in inheritance cases, but until
	someone wants/needs them, they will not be implemented. The only other main reason for implementing them (to me, at least), syntax, is covered
	by our choice to implement Uniform Function Call Syntax, which is explained somewhere else. TODO(sushi) point to where it is explained.
	We decided against contructors and deconstructors because of our experience with them in C++. We think there are better solutions to this problem than
	what C++ implemented. In C++, there are MANY rules to how you make a struct or class because of them and it adds overhead as well as makes some things
	very frustrating to do, such as using unions. It is likely that amu will implement a way to define a function to call on construction and deconstruction
	in the future, but it will simply be a function that is called in these cases and nothing more.

	Some features we intend to implement that are not from C/C++ are things like reflection, advanced metaprogramming, running arbitrary code at compile
	time, 

	The compiler is meant to be as customizable and useful as possible. The text output formatting can be widely customized and any message and message group
	can be disabled. Any warning can be turned off or treated as an error. You can set filters on output such as file or function names. The data from any stage
	in amu can be output to be used externally, and (ideally) be fed back into the compiler to continue from.

	The language itself is also meant to be highly customizable through powerful metaprogramming and compile time features, 
	but this will be implemented and written about later.

	Getting Started
	---------------

	All options for building a project are made in a build file. This file defines all options for the compiler and is meant
	to be the only thing passed to the compiler. This file is not restricted to acting as a build script and can be the code itself in one file,
	amu just expects the passed file to contain the options for compiling rather than on the command line.

	To generate a basic build file template you can use 
		amu -i
	This will make a build file in the current directory, to place it somewhere else, or to give the file a custom name, just pass a path
	after the init command. For example this will initialize a build file in the else folder with the name "build.amu":
		amu init /path/to/somewhere/else/
	This will initialize a build file in the current directory with the name "myprog.amu":
		amu init myprog.amu

	The default build template only contains the most common options for compiling. If you would like a build file that contains all possible options 
	for the compiler use
		amu -init-full
	This will create a build file that contains every single option you can change in the compiler, set to its default value.

	This file is intended to be the only thing passed to amu from now on. Think of it like JUMBO or Unity builds from C/C++, all used files
	will be imported by or included into this file. 
	
	The build file can either contain an entry point function titled "main" of any integer return type with no arguments, or the compiler's entry point option can be 
	set to the name of the function you would like to start from in included files. Of course this function cannot have overloads and must return an integer 
	(signed, unsigned, doesnt matter) or void. 

	If you dont want amu to output anything to stdout, regardless of the build file's settings, start with the command 'quiet', for example
		amu -q build.amu
	will not output anything to the console.
	
	TODO(sushi) determine how arguments to this function should be handled especially in the case of cli args.
			    its possible that we could allow an overloaded entry point, deducing type from command line arguments, but this may be too complicated.


	

*/

/* NOTES

	
*/

/*	TODOs

	syntax
		[priority (!), difficulty (*), date made, tags (optional)] (TODO)
		priority is relative to other todos, if its really not important you can use 0 instead
		difficulty can be seen as 
		*    : easy
		**   : medium
		***  : hard
		***+ : even harder/tedious
		its really meant to be taken as an abstract idea of how hard a task will be at first glance
		extra information about a TODO can be placed underneath the todo, tabbed

	try and keep it so that bugs come below features in each section, so that there is a clear separation
	if this doesnt look good or work well, we can just have a features and bugs section in each section 

	Lexer
	----- TODOs that should mainly involve working in the lexer
	

	Preprocessor
	------------ TODOs that should mainly involve working in the preprocessor
	

	Parser
	------ TODOs that should mainly involve working in the parser
	[!!,***, 2023/07/03, system] rewrite to be multithreaded on global label declarations 

	SemanticAnalyzer
	--------- TODOs that should mainly involve working in the semantic_analyzer
	  type checking
	  identifier validation

	Assembler
	--------- TODOs that should mainly involve working in the assembler
	[0,   **, 2022/07/25, feature] assemble amu to C code. 
	[!!, ***, 2022/07/25, feature] assemble amu to x86_64 assembly. 
	[!,  ***, 2022/07/25, feature] assemble amu to LLVM bytecode
	[!!, ***, 2022/07/25, feature] assemble amu to nasau's instruction set
	[0, ****, 2022/07/25, feature] make a general interface for all of these?


	Compiler
	-------- TODOs that deal with working on the compiler singleton struct

	Build System
	------------
	[!!!!, ***, 2022/08/06] implement build file interpretter 
	[!!!!, ***, 2022/08/06] decide on the build file's format
		we need to decide how the build file will be layed out and its syntax. ideally its syntax is the same as the language's
		and we use simple assignments for setting options, eg. not using silly complex formats like JSON or XML.
	[!!!!, **, 2022/08/06] make the build file template and implement its output when passed the init command.
		we need to make a simple build file template, showing the most common and necessary compile options as well as a full
		build file that contains every option you can set for the compiler, preset to its default value in the compiler.
		ideally these are autogenerated off of options we have in a list or lists somewhere, such as our globals struct in types.h

	Formatter/Logger
	---------------- TODOs relating to the process of formatting and outputting messages, including its interface
	
	Other
	----- other TODOs that may involve working on several parts of the project or just dont fit in any of the other categories
	
*/


#include "time.h"

#include "kigu/common.h"
#include "kigu/unicode.h"
#include "kigu/string_utils.h"

#include "core/memory.h" 
#include "core/file.h"
#include "core/logger.h"
#include "core/platform.h"
#include "core/threading.h"
#include "core/time.h"

#include "basic/Node.h"
#include "storage/Pool.h"
#include "storage/Array.h"
#include "storage/SharedArray.h"
#include "storage/String.h"
#include "storage/DString.h"
#include "storage/Map.h"
#include "Memory.h"
#include "Token.h"
#include "Source.h"
#include "Entity.h"
#include "Messenger.h"
#include "Compiler.h"
#include "Result.h"
#include "Lexer.h"
#include "Parser.h"

#include "Memory.cpp"
#include "basic/Node.cpp"
#include "storage/Pool.cpp"
#include "storage/Array.cpp"
#include "storage/SharedArray.cpp"
#include "storage/DString.cpp"
#include "storage/Map.cpp"
#include "Messenger.cpp"
#include "Diagnostics.cpp"
#include "Compiler.cpp"
#include "Lexer.cpp"
#include "Parser.cpp"

void speed_test(const u64 samples, str8 filepath){
	// f64 sum = 0;
	
	// compiler.logger.log(0, "performing speed_test() on ", CyanFormatComma(filepath), " with ", samples, " samples.");
	// globals.supress_messages = 1;
	// Stopwatch ttime = start_stopwatch();
	// forI(samples){
	// 	compiler.ctime = start_stopwatch();

	// 	CompilerRequest cr; 
	// 	cr.filepaths.add(filepath);
	// 	cr.stage = FileStage_Parser;

	// 	compiler.compile(&cr);
	// 	sum += peek_stopwatch(compiler.ctime);

	// 	compiler.reset();
	// }
	// globals.supress_messages = 0;
	// compiler.logger.log(0, "speed_test() on ", CyanFormatComma(filepath), " with ", samples, " samples had an average time of ", format_time(sum / samples), " and speed_test() took a total of ", format_time(peek_stopwatch(ttime)));
}

// void print_tree(amuNode* node, u32 indent = 0){
// 	// logger_pop_indent(-1);
// 	// logger_push_indent(indent);
// 	// Log("", node->debug);
// 	// for_node(node->first_child){
// 	// 	print_tree(it, indent + 1);
// 	// }
// 	// if(str8_equal_lazy(node->debug, STR8("{"))){
// 	// 	logger_push_indent(indent);
// 	// 	Log("", "}");
// 	// }
// 	// logger_pop_indent(-1);
// }

mutex jobm;
void job(void* in) {
	mutex_lock(&jobm);
	printf("%s\n", ((str8*)in)->str);
	mutex_unlock(&jobm);
}

int main(int argc, char* argv[]){DPZoneScoped;
   	memory_init(Megabytes(1024), Megabytes(1024));//this much memory should not be needed, will trim later
   	platform_init();
   	logger_init();
	threader_init(4, 4, 10);

	{using namespace amu;

		compiler::init();

		auto args = array::init<String>(argc);
		forI(argc) {
			array::push(args, string::init(argv[i]));
		}

		compiler::begin(args);

	}

	// threader_init(255);
	// threader_spawn_thread(10);
	
	// arena.init();

	// compiler.init();
	// compiler.ctime = start_stopwatch();

	// CompilerRequest cr; 
	// cr.filepaths.add(STR8("tests/_/main.amu"));

	// cr.stage = FileStage_Validator;

	// DPFrameMark;
	// CompilerReport report = compiler.compile(&cr);
	// DPFrameMark;
	
	// compiler.logger.log(0, "time: ", format_time(peek_stopwatch(compiler.ctime)));

	// if(report.failed){
	// 	compiler.logger.error("compilation failed.");
	// 	compiler.logger.note("file status:");
	// 	forI(compiler.files.count){
	// 		amuFile* file = compiler.files.data[i];
	// 		compiler.logger.log(Verbosity_Always, CyanFormatComma(file->file->name), ":");
	// 		compiler.logger.log(Verbosity_Always, "         lexer: ", (file->lexical_analyzer.failed ? ErrorFormat("failed") : SuccessFormat("succeeded")));
	// 		compiler.logger.log(Verbosity_Always, "  preprocessor: ", (file->preprocessor.failed ? ErrorFormat("failed") : SuccessFormat("succeeded")));
	// 		compiler.logger.log(Verbosity_Always, "        syntax_analyzer: ", (file->syntax_analyzer.failed ? ErrorFormat("failed") : SuccessFormat("succeeded")));
	// 		compiler.logger.log(Verbosity_Always, "     semantic_analyzer: ", (file->semantic_analyzer.failed ? ErrorFormat("failed") : SuccessFormat("succeeded")));
	// 	}
	// }

	// //print_tree(&compiler.files.atIdxPtrVal(0)->syntax_analyzer.base);
	// generate_ast_graph_svg("ast.svg", (amuNode*)compiler.files.atIdxPtrVal(0)->module);
  
	return 1;
}