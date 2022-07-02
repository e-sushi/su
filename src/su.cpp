#include "core/memory.h"

#define KIGU_STRING_ALLOCATOR deshi_temp_allocator
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


#include "compiler.cpp"

int main(){DPZoneScoped;

   	memory_init(Megabytes(512), Megabytes(512));//this much memory should not be needed, will trim later
   	platform_init();
   	logger_init();



	arena.init();

	DeshThreadManager->init(3);
	DeshThreadManager->spawn_thread();

	//compiler.compile(STR8("tests/lexer/lexer-full.su"));

	Stopwatch ctime = start_stopwatch();

	//LexedFile*         lf = lexer.lex(STR8("tests/imports/valid/imports.su"));
	//LexedFile*         lf = lexer.lex(STR8("tests/_/main.su"));
	LexedFile*         lf = lexer.lex(STR8("stresstest.su"));

	PreprocessedFile* ppf = preprocessor.preprocess(lf);
	forI(preprocessed_files.count){
		//DeshThreadManager->add_job({&parse_threaded_stub, (void*)ParserThreadInfo{parser, &preprocessed_files[preprocessed_files.count - 1 - i]}});		//parser.parse(&preprocessed_files[preprocessed_files.count - 1 - i]);
		//DeshThreadManager->wake_threads();
		parser.parse(&preprocessed_files[preprocessed_files.count - 1 - i]);
		parser = Parser();
	}
	
	//TODO(sushi) change the unit based on how long it took
	suLog(0, "Compiling took ", peek_stopwatch(ctime), " ms");
  
	return 1;
}