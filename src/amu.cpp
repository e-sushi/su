/* amu
	TODO(sushi) rewrite description of amu
*/


/* NOTES
	Following are some notes about designing the compiler

	No type will ever have a constructor that initializes memory nor a deconstructor that deinitializes memory. This functionality is to be 
	implemented through explicit 'init' and 'deinit' functions. Really, no dynamic memory manipulation may ever be done through either of these.
	Constructors should primarily be used for conversions or simple initialization of a struct with arguments, for example, String heavily uses
	constructors for compile time creation from string literals and implicit conversion from DString to String.
	
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

	Project Wide
	------------------------
	[!!!, *, 23/09/03] I started experimenting with OOP stuff in the compiler some time ago and have gone so far as liberally 
					   using inheritance in the Type system. This seems to work well internally, but when it comes to the language
					   trying to reference this information we'll probably start to run into trouble. The biggest thing being 
					   the fact that I'm not using vtables, which amu will not support (natively), which will probably make 
					   working with things internally not work like I wanted. If it causes trouble, we'll have to refactor
					   a lot of stuff to not using OOP style, unfortunately.

	Types
	------------------------
	[!!!, **, 23/09/03] The type system needs to uniquely store Types. As in, a u32[] used in two different places 
	                    needs to point to the same Type object. Currently the system just does some arbitrary hashing
						of the stuff that exists on the Type, but for QOL sake we'll probably want to setup a generic
						hashing system that just takes any type and spits out a hash for it. Clang has an interesting 
						way of doing this (though I would not like to have a system as complicated). See FoldingSet.h
						in LLVM. The thing I like about it is that you define what each part of the data you are hashing
						is. For instance
							struct {
								int a;
								Array<Thing> things;
								...
							};
						You would setup hashing for this by writing something like 
							FoldingSetNodeID ID;
							ID.AddInteger(a);
							ID.AddArray(things);
							...
						In any case, the logic for creating Types is a big mess that needs cleaned up bad.
	
	Parser
	------------------------
	[!!, *, 23/09/15] It may be best to not have specific stages of the parser for things that are prescanned
		              and just let it all be handled normally. 
*/


#include "Common.h"
#include "util.h"

#include <iostream>
#include <filesystem>
#include <unistd.h>
#include <thread>
#include <future>

#ifdef AMU_USE_NOTCURSES
#include "notcurses/notcurses.h"
#endif

#define AMU_IMPLEMENTATION
#include "basic/Memory.h"
#include "basic/Node.h"
#include "storage/View.h"
#include "storage/Pool.h"
#include "storage/Array.h"
#include "storage/DString.h"
#include "storage/String.h"
#include "storage/Map.h"
#include "storage/Stacks.h"
#include "Base.h"
#include "representations/AST.h"
#include "representations/Token.h"
#include "representations/Source.h"
#include "representations/Entity.h"
#include "representations/Label.h"
#include "representations/Module.h"
#include "representations/Variable.h"
#include "representations/Type.h"
#include "representations/ScalarValue.h"
#include "representations/Function.h"
#include "representations/Structure.h"
#include "representations/Expr.h"
#include "representations/Stmt.h"
#include "representations/Tuple.h"
#include "representations/TAC.h"
#include "representations/AIR.h"
#include "representations/Frame.h"
#include "representations/Code.h"
#include "systems/Threading.h"
#include "systems/Diagnostics.h"
#include "systems/VM.h"
#include "systems/Compiler.h"
#include "processors/Lexer.h"
#include "processors/Parser.h"
#include "processors/Sema.h"
#include "processors/GenTAC.h"
#include "processors/GenAIR.h"
#include "systems/VM.h"
#include "systems/Debugger.h"

#include "basic/Memory.cpp"
#include "basic/Node.cpp"
#include "representations/AST.cpp"
#include "storage/Pool.cpp"
#include "storage/Array.cpp"
#include "storage/DString.cpp"
#include "storage/String.cpp"
#include "storage/Map.cpp"
#include "storage/Stacks.cpp"
#include "representations/Source.cpp"
#include "representations/Entity.cpp"
#include "representations/Label.cpp"
#include "representations/Module.cpp"
#include "representations/Variable.cpp"
#include "representations/Type.cpp"
#include "representations/ScalarValue.cpp"
#include "representations/Function.cpp"
#include "representations/Structure.cpp"
#include "representations/Expr.cpp"
#include "representations/Stmt.cpp"
#include "representations/Tuple.cpp"
#include "representations/Code.cpp"
#include "systems/Threading.cpp"
#include "systems/Diagnostics.cpp"
#include "systems/Messenger.cpp"
#include "systems/Compiler.cpp"
#include "processors/Lexer.cpp"
#include "processors/Parser.cpp"
#include "processors/Sema.cpp"
#include "processors/GenTAC.cpp"
#include "processors/GenAIR.cpp"
#include "systems/VM.cpp"
#include "systems/Debugger.cpp"

int main(int argc, char* argv[]){
	{using namespace amu;
		srand(time(0));
		compiler::init();
		auto args = Array<String>::create(argc);
		forI(argc) 
			args.push(string::init(argv[i]));
		compiler::begin(args);
	}
	return 0;
}
