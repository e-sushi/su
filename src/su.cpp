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

struct uhuh{
	b32 finished = 0;
	condvar cv;
};

void wake_up_sub_thread(void* in){
	platform_sleep(100);
	((uhuh*)in)->finished = 1;
	((uhuh*)in)->cv.notify_all();
}

void wake_up_main_thread(void* in){
	const u32 n = 3;
	uhuh test[n];

	forI(n){
		DeshThreadManager->add_job({&wake_up_sub_thread, &test[i]});
	}

	DeshThreadManager->wake_threads();
	
	forI(n){
		if(!test[i].finished){
			test[i].cv.wait();
		}
	}

	((condvar*)in)->notify_all();
}

int main(){DPZoneScoped;

   	memory_init(Megabytes(512), Megabytes(512));//this much memory should not be needed, will trim later
   	platform_init();
   	logger_init();

	arena.init();

	DeshThreadManager->init(5);
	DeshThreadManager->spawn_thread(5);
	platform_sleep(50); //sleep to allow threads to spawn (not necessary, so probably remove later)

	//platform_sleep(100);


	//compiler.compile(STR8("tests/lexer/lexer-full.su"));

	Stopwatch ctime = start_stopwatch();

	CompilerRequest cr; 
	cr.filepaths.add(STR8("tests/imports/valid/imports.su"));
	cr.stages = Stage_Lexer | Stage_Preprocessor | Stage_Parser;

	compiler.compile(&cr);

/*
	LexedFile*         lf = lexer.lex(STR8("tests/imports/valid/imports.su"));
	//LexedFile*         lf = lexer.lex(STR8("tests/_/main.su"));
	//LexedFile*         lf = lexer.lex(STR8("stresstest.su"));

	PreprocessedFile* ppf = preprocessor.preprocess(lf);

	ParserThread pt;
	pt.parser = &parser;
	pt.pfile = ppf;


	DeshThreadManager->add_job({&parse_threaded_stub, &pt});
	DeshThreadManager->wake_threads(1);

	pt.wake.wait();

	// forI(preprocessed_files.count){
	// 	//DeshThreadManager->add_job({&parse_threaded_stub, (void*)ParserThreadInfo{parser, &preprocessed_files[preprocessed_files.count - 1 - i]}});		//parser.parse(&preprocessed_files[preprocessed_files.count - 1 - i]);
	// 	//DeshThreadManager->wake_threads();
	// 	parser.parse(&preprocessed_files[preprocessed_files.count - 1 - i]);
	// 	parser = Parser();
	// }
*/	
	//TODO(sushi) change the unit based on how long it took
	suLog(0, "Compiling took ", peek_stopwatch(ctime), " ms");
  
	return 1;
}