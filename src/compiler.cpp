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
    suLog(1, "compiler", "Beginning compiler request on ", request->filepaths.count, " files.");

    if(globals.verbosity > 3){
        for(str8 s : request->filepaths){
            suLog(4, "compiler", "Request to compile path ", s);
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
    DeshThreadManager->wake_threads(request->filepaths.count);

    forI(request->filepaths.count){
        while(!ct[i].finished){
            DPTracyMessageL("compiler: waiting on compiler thread");
            ct[i].wait.wait();
        }
        ct[i].wait.deinit();
    }
}

LexedFile* Compiler::start_lexer(str8 filepath, b32 spawn_thread){DPZoneScoped;
    suLog(2, "compiler", "Starting a lexer");

    mutexes.lexer.lock();

    File* file = file_init(filepath, FileAccess_Read);
    if(!file){
		suError("Unable to open file at path ", filepath);
		return 0;
	} 

	str8 filename = file->name;

    suLog(2, filename, "Checking if file has already been lexed");

	if(lexed_files.has(filename)){
		suLog(2, filename, SuccessFormat("File has already been lexed."));
        mutexes.lexer.unlock();
		return &lexed_files[filename];
	}

    Lexer* lexer = (Lexer*)memalloc(sizeof(Lexer));

	suLog(2, filename, "Adding file to lexed files");
	lexed_files.add(filename);
	lexer->lexfile = &lexed_files[filename];
	lexer->lexfile->file = file;
    mutexes.lexer.unlock();

	suLog(2, filename, "Reading file contents into a buffer");
    str8 buffer = file_read_alloc(file, file->bytes, deshi_temp_allocator); 

    if(spawn_thread){
        LexerThread lt;
        lt.buffer = buffer;
        lt.lexer = lexer;
        lt.wait.init();

        suLog(2, filename, "Starting lexer thread.");
        DeshThreadManager->add_job({&lexer_threaded_stub, &lt});
        DeshThreadManager->wake_threads(1);

        DPTracyMessageL("compiler: waiting on lexer thread");
        lt.wait.wait();
        DPTracyMessageL("compiler: lexer thread finished");
        suLog(2, filename, "Lexer thread ended.");
    }else{
        lexer->lex(buffer);
    }

    LexedFile* ret = lexer->lexfile;
    memzfree(lexer);
    return ret;
}

PreprocessedFile* Compiler::start_preprocessor(LexedFile* lexfile, b32 spawn_thread){DPZoneScoped;
    mutexes.preprocessor.lock();

    str8 filename = lexfile->file->name;

    suLog(2, "compiler", "Starting a preprocessor.");

    suLog(2, filename, "Checking if file has already been preprocessed.");

    if(preprocessed_files.has(lexfile->file->name)){
        suLog(2, filename, SuccessFormat("File has already been preprocessed."));
        mutexes.preprocessor.unlock();        
        return &preprocessed_files[lexfile->file->name];
    } 

    Preprocessor* preprocessor = (Preprocessor*)memalloc(sizeof(Preprocessor)); 

    suLog(2, filename, "Adding file to preprocessed files.");
    preprocessed_files.add(lexfile->file->name);
    preprocessor->prefile = &preprocessed_files[lexfile->file->name];
    preprocessor->prefile->lexfile = lexfile;

    mutexes.preprocessor.unlock();

    if(spawn_thread){
        PreprocessorThread pt;
        pt.lexfile = lexfile;
        pt.preprocessor = preprocessor;
        pt.wait.init();
        
        suLog(2, filename, "Starting preprocessor thread.");

        DeshThreadManager->add_job({&preprocessor_thread_stub, &pt});
        DeshThreadManager->wake_threads(1);

        DPTracyMessageL("waiting on preprocessor thread");
        pt.wait.wait();
        DPTracyMessageL("preprocessor thread finished");

        suLog(2, filename, "Preprocessor thread ended.");
    }else{
        preprocessor->preprocess(lexfile);
    }
    

    PreprocessedFile* prefile = preprocessor->prefile;
    memzfree(preprocessor);
    return prefile;
}

ParsedFile* Compiler::start_parser(PreprocessedFile* prefile, b32 spawn_thread){DPZoneScoped;
    mutexes.parser.lock();

    str8 filename = prefile->lexfile->file->name;

    suLog(2, filename, "Starting a parser.");

    suLog(2, filename, "Checking if file has already been parsed");

    if(parsed_files.has(prefile->lexfile->file->name)){
        suLog(2, filename, SuccessFormat("File has already been parsed."));
        mutexes.parser.unlock();
        return &parsed_files[prefile->lexfile->file->name];
    }
    
    Parser* parser = (Parser*)memalloc(sizeof(Parser));

    suLog(2, filename, "Adding file to parsed files.");
    parsed_files.add(prefile->lexfile->file->name);
    parser->parfile = &parsed_files[prefile->lexfile->file->name];
    parser->parfile->prefile = prefile;

    mutexes.parser.unlock();

    if(spawn_thread){
        ParserThread pt;
        pt.pfile = prefile;
        pt.parser = parser;
        pt.wake.init();

        suLog(2, filename, "Starting parser thread.");

        DeshThreadManager->add_job({&parse_threaded_stub, &pt});
        DeshThreadManager->wake_threads(1);

        DPTracyMessageL("Waiting on parser thread");
        pt.wake.wait();
        DPTracyMessageL("Parser thread finished.");
    }else{
        parser->parse(prefile);
    }

    ParsedFile* parfile = parser->parfile;
    memzfree(parser);
    return parfile;
}

void Compiler::reset(){
    forI(lexed_files.count){
        file_deinit(lexed_files[i].file);
    }
    lexed_files.clear();
    preprocessed_files.clear();
    parsed_files.clear();
    memory_clear_arena(arena.functions);
	memory_clear_arena(arena.variables);
	memory_clear_arena(arena.structs);
	memory_clear_arena(arena.scopes);
	memory_clear_arena(arena.expressions);
    memory_clear_temp();
}

