#ifdef FIX_MY_IDE_PLEASE
#include "core/memory.h"

#define KIGU_STRING_ALLOCATOR deshi_temp_allocator
#include "kigu/profiling.h"
#include "kigu/array.h"
#include "kigu/array_utils.h"
#include "kigu/common.h"
#include "kigu/cstring.h"
#include "kigu/map.h"
#include "kigu/string.h"
#include "kigu/node.h"

#define DESHI_DISABLE_IMGUI
#include "core/logger.h"
#include "core/platform.h"
#include "core/file.h"
#include "core/threading.h"

#include <stdio.h>
#include <stdlib.h>

#include "kigu/common.h"
#include "kigu/unicode.h"
#include "kigu/hash.h"
#include "types.h"

#endif

//TODO(sushi) compiler struct that manages stages
// eg. has functions like spawn_lexer, spawn_parser, etc.



void compile_threaded_func(void* in){DPZoneScoped;
    CompilerThread* ct = (CompilerThread*)in;
    
    LexedFile*        lexfile = 0;
    PreprocessedFile* prefile = 0;
    ParsedFile*       parfile = 0;

    //TODO(sushi) support for passing each stages file and starting compilation from stages passed lexing if 
    //            necessary
    if(HasFlag(ct->stages, Stage_Lexer)){
        lexfile = compiler.start_lexer(ct->filepath);
    }
    if(lexfile && HasFlag(ct->stages, Stage_Preprocessor)){
        prefile = compiler.start_preprocessor(lexfile);    
    }
    if(prefile && HasFlag(ct->stages, Stage_Parser)){
        parfile = compiler.start_parser(prefile);
    }

    ct->finished = 1;
    DPTracyMessageL("compiler: notifying condvar");
    ct->wait.notify_all();
}

void Compiler::compile(CompilerRequest* request){DPZoneScoped;
    DPTracyMessageL("compiler: compile requested");
    suLog(1, "Beginning compiler request on ", request->filepaths.count, " files.");

    if(globals.verbosity > 3){
        for(str8 s : request->filepaths){
            suLog(4, "Request to compile path ", s);
        }
    }

    DPTracyMessageL("compiler: creating CompilerThreads");
    CompilerThread* ct = (CompilerThread*)memalloc(sizeof(CompilerThread) * request->filepaths.count);

    DPTracyMessageL("compiler: filling out CompilerThreads");
    forI(request->filepaths.count){
        ct[i].wait.init();
        ct[i].filepath = request->filepaths[i];
        ct[i].stages = request->stages;
        DeshThreadManager->add_job({&compile_threaded_func, &ct[i]});
    }

    DPTracyMessageL("compiler: waking threads");
    DeshThreadManager->wake_threads();

    forI(request->filepaths.count){
        while(!ct[i].finished){
            DPTracyMessageL("compiler: waiting on compiler thread");
            ct[i].wait.wait();
            //Log("", "hi");
        }
        ct[i].wait.deinit();
    }
    int hey_there = 1;
}

LexedFile* Compiler::start_lexer(str8 filepath){DPZoneScoped;
    mutexes.lexer.lock();

    suLog(2, "Spawning a lexer");

    //mutexes.memory.lock();
    
    Lexer* lexer = (Lexer*)memalloc(sizeof(Lexer));

    File* file = file_init(filepath, FileAccess_Read);
    
    //mutexes.memory.unlock();
	
    if(!file){
		suError("Unable to open file at path ", filepath);
		return 0;
	} 
    
	str8 filename = file->name;

	suLog(2, "Checking if file has already been lexed");

	if(lexed_files.has(filename)){
		suLog(2, SuccessFormat("File has already been lexed."));
        mutexes.lexer.unlock();
		return &lexed_files[filename];
	}

	suLog(2, "Adding file to lexed files");
	lexed_files.add(filename);
	lexer->lexfile = &lexed_files[filename];
	lexer->lexfile->file = file;

	suLog(2, "Reading file contents into a buffer");
    str8 buffer = file_read_alloc(file, file->bytes, deshi_allocator); 

    mutexes.lexer.unlock();

    LexerThread lt;
    lt.buffer = buffer;
    lt.lexer = lexer;
    lt.wait.init();

    suLog(2, "Starting lexer thread.");

    DeshThreadManager->add_job({&lexer_threaded_stub, &lt});
    DeshThreadManager->wake_threads(1);

    DPTracyMessageL("compiler: waiting on lexer thread");
    lt.wait.wait();
    DPTracyMessageL("compiler: lexer thread finished");

    suLog(2, "Lexer thread ended.");

    LexedFile* ret = lexer->lexfile;
    memzfree(lexer);
    return ret;
}

PreprocessedFile* Compiler::start_preprocessor(LexedFile* lexfile){DPZoneScoped;
    mutexes.preprocessor.lock();

    suLog(2, "Spawning a preprocessor.");

    Preprocessor* preprocessor = (Preprocessor*)memalloc(sizeof(Preprocessor)); 

    suLog(2, "Checking if file has already been preprocessed.");

    if(preprocessed_files.has(lexfile->file->name)){
        suLog(2, SuccessFormat("File has already been preprocessed."));
        mutexes.preprocessor.unlock();        
        return &preprocessed_files[lexfile->file->name];
    } 

    suLog(2, "Adding file to preprocessed files.");

     
    preprocessed_files.add(lexfile->file->name);
    preprocessor->prefile = &preprocessed_files[lexfile->file->name];
    preprocessor->prefile->lexfile = lexfile;

    mutexes.preprocessor.unlock();

    PreprocessorThread pt;
    pt.lexfile = lexfile;
    pt.preprocessor = preprocessor;
    pt.wait.init();
    
    suLog(2, "Starting preprocessor thread.");

    DeshThreadManager->add_job({&preprocessor_thread_stub, &pt});
    DeshThreadManager->wake_threads(1);

    DPTracyMessageL("waiting on preprocessor thread");
    pt.wait.wait();
    DPTracyMessageL("preprocessor thread finished");


    suLog(2, "Preprocessor thread ended.");

    PreprocessedFile* prefile = preprocessor->prefile;
    memzfree(preprocessor);
    return prefile;
}

ParsedFile* Compiler::start_parser(PreprocessedFile* prefile){DPZoneScoped;
    return 0;
}
