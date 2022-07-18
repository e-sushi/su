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

//so we can easily disable in release
#define ThreadSetName(name) DeshThreadManager->set_thread_name(name)

#include "types.h"

#include "lexer.cpp"
#include "preprocessor.cpp"
#include "parser.cpp"
#include "compiler.cpp"

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

int main(){DPZoneScoped;


   	memory_init(Megabytes(512), Megabytes(512));//this much memory should not be needed, will trim later
   	platform_init();
   	logger_init();

	DeshThreadManager->init(255);
	DeshThreadManager->spawn_thread(10);
	compiler.logger.owner_str_if_sufile_is_0 = STR8("compiler");

	compiler.mutexes.log.init();
	compiler.mutexes.preprocessor.init();
	compiler.mutexes.parser.init();
	compiler.mutexes.lexer.init();

	arena.init();

	//speed_test(50, STR8("tests/imports/valid/imports.su"));

	compiler.ctime = start_stopwatch();

	CompilerRequest cr; 
	//cr.filepaths.add(STR8("tests/imports/valid/imports.su"));
	cr.filepaths.add(STR8("tests/_/main.su"));

	cr.stage = FileStage_Parser;

	compiler.compile(&cr);

	compiler.logger.log(0, "time: ", format_time(peek_stopwatch(compiler.ctime)));
  
	return 1;
}