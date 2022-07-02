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



void compile_threaded_func(void* in){

    CompilerThread* ct = (CompilerThread*)in;

    LexedFile* lexfile = compiler.start_lexer(ct->filepath);
    PreprocessedFile* prefile = compiler.start_preprocessor(lexfile);

    ct->wait.notify_all();

}

void Compiler::compile(str8 filepath){
    suLog(1, "Starting compilation with ", filepath);

    CompilerThread ct;
    ct.filepath = filepath;

    DeshThreadManager->add_job({&compile_threaded_func, &ct});
    DeshThreadManager->wake_threads(1);
    
    ct.wait.wait();
}

LexedFile* Compiler::start_lexer(str8 filepath){
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

    suLog(2, "Starting lexer thread.");

    DeshThreadManager->add_job({&lexer_threaded_stub, &lt});
    DeshThreadManager->wake_threads(1);

    lt.wait.wait();

    suLog(2, "Lexer thread ended.");

    LexedFile* ret = lexer->lexfile;
    memzfree(lexer);
    return ret;
}

PreprocessedFile* Compiler::start_preprocessor(LexedFile* lexfile){
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
    
    suLog(2, "Starting preprocessor thread.");

    DeshThreadManager->add_job({&preprocessor_thread_stub, &pt});
    DeshThreadManager->wake_threads(1);

    pt.wait.wait();

    suLog(2, "Preprocessor thread ended.");


    PreprocessedFile* prefile = preprocessor->prefile;
    memzfree(preprocessor);
    return prefile;
}

ParsedFile* Compiler::start_parser(PreprocessedFile* prefile){
    return 0;
}
