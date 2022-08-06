#include "core/memory.h"

#define KIGU_STRING_ALLOCATOR deshi_temp_allocator
#define KIGU_ARRAY_ALLOCATOR deshi_allocator
#include "kigu/profiling.h"
#include "kigu/array.h"
#include "kigu/array_utils.h"
#include "kigu/common.h"
#include "kigu/cstring.h"
#include "kigu/map.h"
#include "kigu/string.h"
#include "kigu/string_utils.h"   
#include "kigu/node.h"

#define DESHI_DISABLE_IMGUI
#include "core/logger.h"
#include "core/platform.h"
#include "core/file.h"
#include "core/threading.h"
#include "core/time.h"

#include <stdio.h>
#include <stdlib.h>

#include "kigu/common.h"
#include "kigu/unicode.h"
#include "kigu/hash.h"

#include "types.h"

#include "lexer.cpp"
#include "preprocessor.cpp"
#include "parser.cpp"
#include "validator.cpp"
#include "compiler.cpp"
#include "astgraph.cpp"

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
	[!, ***, 2022/07/05] determine if there are other things lexer can do during its stage
	    since lexer will always parse an entire file there are probably other things we can get it to look for
		to ease work in later stages, ideally without putting too much work on the lexer.
		any work added to lexer should not be too demanding and ideally shouldnt have to look ahead or look backwards
		by moving the stream or some other cursor.

	Preprocessor
	------------ TODOs that should mainly involve working in the preprocessor
	[!, **, 2022/07/05, feature] implement #include
	    works the same as C's include, just pastes whatever file into the buffer
		as an alternative to using import or even for directly pasting data into code
	[!, **, 2022/07/05] implement #external
	    just does the opposite of #internal, used to undo it
	[!!!, **, 2022/07/25, feature]  import/include searching
	  	search relative to importing/including file
		search PATH
	[!, **, 2022/07/05, feature] implement #message
	    used for outputting simple messages at compile time

	Parser
	------ TODOs that should mainly involve working in the parser
	[!!!, **, 2022/07/25, feature] statements
      	for 
	  	while 
	  	switch 
	[!!,  **, 2022/07/25, feature] using
	  	variations of using such as 
	  	aliasing a function's name
		aliasing a function's name and signature
		using using for inheriting inside of a struct
	[!,   **, 2022/07/25, feature] misc
		initializer lists
	  	inc/dec

	Validator
	--------- TODOs that should mainly involve working in the validator
	  type checking
	  identifier validation

	Assembler
	--------- TODOs that should mainly involve working in the assembler
	[0,   **, 2022/07/25, feature] assemble su to C code. 
	[!!, ***, 2022/07/25, feature] assemble su to x86_64 assembly. 
	[!,  ***, 2022/07/25, feature] assemble su to LLVM bytecode
	[!!, ***, 2022/07/25, feature] assemble su to nasau's instruction set
	[0, ****, 2022/07/25, feature] make a general interface for all of these?


	Compiler
	-------- TODOs that deal with working on the compiler singleton struct

	Other
	----- other TODOs that may involve working on several parts of the project
	[!!!!, *, 2022/08/02] rename occurances of su throughout the project to amu
		this includes file names, names of structs (suNode), etc.
	[!!!, *, 2022/07/27, tests] tests need rewritten to be in amu's new syntax
	[!!, ***, 2022/08/02] add a system for outputting each stage's data for external use
		NOTE its probably best to put this off until the compiler is stable
		this means setting up a way for the compiler to stop at any stage and output all of that stage's data.
		some difficulties of this are
			* only getting output of select files
			* getting the data for multiple stages
			* after validation we combine all data into one suFile, so per file information is only accessible through tokens
			* how is the output formatted? can we use this output to continue compilation later?
			* a file's data is not contiguous internally, it would need to be stitched together
	[0,  **, 2022/07/31, vis, bug] fix implementation of graphviz visualization of the AST tree
		for whatever reason I get linker errors on Agdirected when trying to compile with graphviz
		not sure why, because I have it setup just like I did back when we worked on su in January
	[!!, ***, 2022/07/25, threading, memory, bug] 
		due to the randomness of these issues, i cannot provide examples,
		but you can probably make them more likely to occur by using many large files that import each other
		there are many random instances where memory asserts, most likely because we still arent handling
		thread safety completely. 
		there are still instances where deadlocks and missed wakeups occur
	[!!!!, **, 2022/07/31, bug] error on code in global space that is not a directive or declaration
		currently we don't check if code in global space is meant to be there or not because we 
		skip over anything that is not a decl or directive. preprocessor will need to do some kind of pass
		that errors in cases like these.
		example:
			#import "math.su"

			return; //this is not caught by anything

			main():s32{

			}
		a critical example where it's failing may be confusing is
			f(){ //forgot the return type
				//blah
			}
			main(){}
		in this case the compiler will never error on f, unless the user tries to call it in which it will just tell them
		it doesnt exist. this can be really confusing and so should be fixed.
	
*/

/* NOTES

	amu is split into various stages. This is a list of them and their tasks

	Lexer
	-----
	Turn an input file into a series of tokens
	Mark places of interest
	  Mark where possible declarations are (colons)
	  Mark where directives are (pounds)
	Mark tokens with their scope depth
	Mark tokens with their line and column numbers, both start and end
	Set the raw string that the token represets
	Set the file string that the token resides in
	-----

	Preprocesser
	-------------
    Iterate where lexer marked import directives and run lexer and preprocessor on files that are imported
      Error on invalid filenames and syntax. Note that this does not parse specific imports, as in it 
      does not care about importing only 'sin' from "math.su", all it does is start compiling files.
    Resolve ':' tokens as possible declarations
      This checks that the colons marked by lexer are actually declarations. Following amu's rule of
      <identifier> [<funcargs>] ":" <typeid> we parse around the colon to see if it is a valid declaration
      and if it is mark the <identifier> token as a declaration. This is useful so that in parsing we dont 
      have to look for a : when we come across every identifier.
    Parse marked internal directives and organize tokens
      Before this is started, preprocessor's exported decls array takes in all global decls. Then we parse
      internal directives and as needed pull indexes from the exported decls array into the internal
      decls array.
    Parse run directives
      This is possibly going to be removed from this stage as I'm not sure if #run will be allowed to be
      used in a global space to indicate some code to run immediately, so if not this part would be 
      irrelevant.
	-------------

	Parser
	------
	Converts tokens into an Abstract Syntax Tree
	The parser only checks syntax and builds the AST from it. It doesn't do anything like type checking
	or if identifiers exist.
	Mark token start and token end ranges for each node
	------

	Validator
	---------
	Join every files AST together
	Identifier validating
	Type checking
	---------

	All files are independent of each other up until validator. Meaning we can have a thread per file
	running freely through the stages until it reaches validator, since validator is the stage in which 
	we join all the files information together. 

	amu organizes information by file using suFile. When a file is loaded a suFile is made for it
	and represents any instance of it. suFile stores information for all stages and is how stages
	communicate information between each other. 

	Something I would like to try and follow is that all data from previous stages is static. 
	This rule is nice for multithreading since it guarantees atomic access to stages that a 
	file has completed. This rule stops being followed once we reach validation because it needs 
	to take nodes from each parser and join them together, as well as modify the AST where it needs to.
	This will probably also apply to optimization stage, if its done using the AST.

	Some notes on syntax
	  Declarations always follow the pattern <identifier> ":" <type-specifier> with the small exception
	  of functions that have parameter declarations after its identifier. There should be nothing in between
	  the colon and the type-specifier. This makes the colon act as an anchor to allow easy access to the type
	  specifier.


*/

//this is temp allocated, so just clear temp mem or free the str yourself
str8 format_time(f64 ms){
	if(floor(Minutes(ms))){
		//hope it never gets this far :)
		f64 fmin = floor(Minutes(ms));
		f64 fsec = floor(Seconds(ms)) - fmin * 60;
		f64 fms  = ms - fmin*60*1000 - fsec*1000;
		return toStr8(fmin, "m ", fsec, "s ", fms, " ms");
	}else if(floor(Seconds(ms))){
		f64 fsec = floor(Seconds(ms));
		f64 fms  = ms - fsec*SecondsToMS(1);
		return toStr8(fsec, "s ", fms, "ms");
	}else{
		return toStr8(ms, " ms");
	}
}

void speed_test(const u64 samples, str8 filepath){
	f64 sum = 0;
	
	compiler.logger.log(0, "performing speed_test() on ", CyanFormatDyn(filepath), " with ", samples, " samples.");
	globals.supress_messages = 1;
	Stopwatch ttime = start_stopwatch();
	forI(samples){
		compiler.ctime = start_stopwatch();

		CompilerRequest cr; 
		cr.filepaths.add(filepath);
		cr.stage = FileStage_Parser;

		compiler.compile(&cr);
		sum += peek_stopwatch(compiler.ctime);

		compiler.reset();
	}
	globals.supress_messages = 0;
	compiler.logger.log(0, "speed_test() on ", CyanFormatDyn(filepath), " with ", samples, " samples had an average time of ", format_time(sum / samples), " and speed_test() took a total of ", format_time(peek_stopwatch(ttime)));
}

void print_tree(suNode* node, u32 indent = 0){
	logger_pop_indent(-1);
	logger_push_indent(indent);
	Log("", node->debug);
	for_node(node->first_child){
		print_tree(it, indent + 1);
	}
	if(str8_equal_lazy(node->debug, STR8("{"))){
		logger_push_indent(indent);
		Log("", "}");
	}
	logger_pop_indent(-1);
}

int main(){DPZoneScoped;


   	memory_init(Megabytes(1024), Megabytes(1024));//this much memory should not be needed, will trim later
   	platform_init();
   	logger_init();

	DeshThreadManager->init(255);
	DeshThreadManager->spawn_thread(10);
	
	arena.init();


	//speed_test(50, STR8("tests/imports/valid/imports.su"));
	compiler.init();
	compiler.ctime = start_stopwatch();

	CompilerRequest cr; 
	//cr.filepaths.add(STR8("tests/imports/valid/imports.su"));
	cr.filepaths.add(STR8("tests/_/main.su"));

	cr.stage = FileStage_Validator;

	DPFrameMark;
	compiler.compile(&cr);
	DPFrameMark;

	compiler.logger.log(0, "time: ", format_time(peek_stopwatch(compiler.ctime)));

	//print_tree(&compiler.files.atIdxPtrVal(0)->parser.base);
	//generate_ast_graph_svg("ast.svg", &compiler.files.atIdx(0)->parser.exported_decl.atIdx(0)->node);
  
	return 1;
}