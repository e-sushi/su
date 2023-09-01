/* amu
	TODO(sushi) rewrite description of amu
*/


/* NOTES
	Following are some notes about designing the compiler

	No type will ever have a constructor that initializes memory nor a deconstructor that deinitializes memory. This functionality is to be 
	implemented through explicit 'init' and 'deinit' functions. Really, no dynamic memory manipulation may ever be done through either of these.
	Constructors should primarily be used for conversions or simple initialization of a struct with arguments, for example, String heavily uses
	constructors for compile time creation from string literals and implicit conversion from DString to String.
	
	The C++ std library should only be used in cases where platform specific functionality is required. Otherwise, amu prioritizes implementing 
	things on its own.

	Member functions should be almost always be avoided in favor of putting this functionality behind a namespace
	populated with equivalent functions taking a reference to the thing to be called on as the first argument.

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
	[!!, **, 23/08/29] DStrings need to be taken better care of. Currently there are leaks all throughout the program
	                   primarily because we can just rely on memory to be cleaned up by the program ending, but when 
					   we get around to compile time stuff and especially allowing using amu in a fully interpretted 
					   style, we will want to make sure that the compiler itself does not cause leaks. 

					   Ideally I would like a system where we can keep track of when a DString is no longer needed.
					   I tried experimenting with an ownership/reference counting system, but it was just too complicated
					   and took too much time away from what I needed to be working on at the time. 


*/


#include "Common.h"
#include "util.h"

#include <iostream>
#include <filesystem>
#include <unistd.h>

#define AMU_IMPLEMENTATION
#include "basic/Memory.h"
#include "basic/Node.h"
#include "representations/AST.h"
#include "storage/View.h"
#include "storage/Pool.h"
#include "storage/Array.h"
#include "storage/SharedArray.h"
#include "storage/String.h"
#include "storage/DString.h"
#include "storage/Map.h"
#include "representations/Token.h"
#include "representations/Source.h"
#include "representations/Entity.h"
#include "representations/Label.h"
#include "representations/Module.h"
#include "representations/Variable.h"
#include "representations/Type.h"
#include "representations/Function.h"
#include "representations/Structure.h"
#include "representations/Expr.h"
#include "representations/Stmt.h"
#include "representations/Tuple.h"
#include "representations/Code.h"
#include "systems/Diagnostics.h"
#include "systems/Messenger.h"
#include "systems/Compiler.h"
#include "processors/Lexer.h"
#include "processors/Parser.h"
#include "processors/Sema.h"
#include "processors/Gen.h"
#include "systems/Machine.h"

#include "basic/Memory.cpp"
#include "basic/Node.cpp"
#include "representations/AST.cpp"
#include "storage/Pool.cpp"
#include "storage/Array.cpp"
#include "storage/SharedArray.cpp"
#include "storage/DString.cpp"
#include "storage/Map.cpp"
#include "representations/Source.cpp"
#include "representations/Entity.cpp"
#include "representations/Label.cpp"
#include "representations/Module.cpp"
#include "representations/Variable.cpp"
#include "representations/Type.cpp"
#include "representations/Function.cpp"
#include "representations/Structure.cpp"
#include "representations/Expr.cpp"
#include "representations/Stmt.cpp"
#include "representations/Tuple.cpp"
#include "representations/Code.cpp"
#include "systems/Diagnostics.cpp"
#include "systems/Messenger.cpp"
#include "systems/Compiler.cpp"
#include "processors/Lexer.cpp"
#include "processors/Parser.cpp"
#include "processors/Sema.cpp"
#include "processors/Gen.cpp"
#include "systems/Machine.cpp"


int main(int argc, char* argv[]){
	{using namespace amu;
		compiler::init();

		auto args = array::init<String>(argc);
		forI(argc) {
			array::push(args, string::init(argv[i]));
		}

		compiler::begin(args);
	}
	return 0;
}
