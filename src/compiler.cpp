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
    
    //TODO(sushi) support for passing each stages file and starting compilation from stages passed lexing if 
    //            necessary
    if(ct->stage > FileStage_Null){
        compiler.start_lexer(ct->sufile);
    }
    if(ct->stage > FileStage_Lexer){
        compiler.start_preprocessor(ct->sufile);    
    }
    if(ct->stage > FileStage_Preprocessor){
        compiler.start_parser(ct->sufile);
    }

    ct->finished = 1;
    DPTracyMessageL("compiler: notifying condvar");
    ct->wait.notify_all();
}

void Compiler::compile(CompilerRequest* request){DPZoneScoped;
    DPTracyMessageL("compiler: compile requested");
    logger.log(1, "Beginning compiler request on ", request->filepaths.count, " files.");

    if(globals.verbosity > 3){
        for(str8 s : request->filepaths){
            logger.log(4, "Request to compile path ", s);
        }
    }

    DPTracyMessageL("compiler: creating CompilerThreads");
    CompilerThread* ct = (CompilerThread*)memalloc(sizeof(CompilerThread) * request->filepaths.count);

    DPTracyMessageL("compiler: filling out CompilerThreads");
    forI(request->filepaths.count){
        str8 filepath = request->filepaths[i];
        //TODO(sushi) it may be better to just interate the string backwards and look for / or \ instead
        u32 last_slash = str8_find_last(filepath, '/');
        if(last_slash == npos) last_slash = str8_find_last(filepath, '\\');
        if(last_slash == npos) last_slash = 0;
        str8 filename = str8{filepath.str+last_slash+1, filepath.count-(last_slash+1)};
        
        suFile* sufile = files.at(filename);

        //make a new suFile if it doesnt exist already
        if(!sufile){
            logger.log(2, "File has not been loaded yet, making a new suFile.");
            File* file = file_init(filepath, FileAccess_Read);
            if(!file){
                logger.error("Unable to open file at path ", filepath);
                return;
            } 

            suFile nufile;
            nufile.file = file;
            nufile.stage = FileStage_Null;
            
            sufile = files.atIdx(files.add(filename, nufile));
            sufile->logger.sufile = sufile;
        } 

        ct[i].wait.init();
        ct[i].filepath = request->filepaths[i];
        ct[i].stage = request->stage;
        ct[i].sufile = sufile;
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

suFile* Compiler::start_lexer(suFile* sufile){DPZoneScoped;
    Assert(sufile, "Compiler::start_lexer was passed a null suFile*");
    
    logger.log(2, "Starting a lexer");
    logger.log(2, "Checking if file has already been lexed");
	if(sufile->stage >= FileStage_Lexer){
		logger.log(2, SuccessFormat("File has already been lexed."));
		return sufile;
	}

    mutexes.lexer.lock();
    Lexer* lexer = (Lexer*)memalloc(sizeof(Lexer));
    mutexes.lexer.unlock();
	
    logger.log(2, "Reading file contents into a buffer");
    sufile->file_buffer = file_read_alloc(sufile->file, sufile->file->bytes, deshi_temp_allocator); 
    lexer->sufile = sufile;
    lexer->lex();

    sufile->stage = FileStage_Lexer;

    memzfree(lexer);
    return sufile;
}

suFile* Compiler::start_preprocessor(suFile* sufile){DPZoneScoped;
    Assert(sufile, "Compiler::start_preprocessor was passed a null suFile*");
    Assert(sufile->stage >= FileStage_Lexer, "Compiler::start_preprocessor was given a sufile that has not completed previous stages.");

    logger.log(2, "Starting a preprocessor.");
    logger.log(2, "Checking if file has already been preprocessed.");
    if(sufile->stage >= FileStage_Preprocessor){
        logger.log(2, SuccessFormat("File has already been preprocessed."));
        return sufile;
    } 

    mutexes.preprocessor.lock();
    Preprocessor* preprocessor = (Preprocessor*)memalloc(sizeof(Preprocessor)); 
    mutexes.preprocessor.unlock();        

    preprocessor->sufile = sufile;
    preprocessor->preprocess();

    sufile->stage = FileStage_Preprocessor;

    memzfree(preprocessor);
    return sufile;
}

suFile* Compiler::start_parser(suFile* sufile){DPZoneScoped;
    Assert(sufile, "Compiler::start_preprocessor was passed a null suFile*");
    Assert(sufile->stage >= FileStage_Lexer, "Compiler::start_parser was given a sufile that has not completed previous stages.");

    logger.log(2, "Starting a parser.");
    logger.log(2, "Checking if file has already been parsed");
    if(sufile->stage >= FileStage_Parser){
        logger.log(2, SuccessFormat("File has already been parsed."));
        return sufile;
    }
    
    mutexes.parser.lock();
    Parser* parser = (Parser*)memalloc(sizeof(Parser)); 
    mutexes.parser.unlock();
    
    parser->sufile = sufile;
    parser->parse();

    sufile->stage = FileStage_Parser;

    memzfree(parser);
    return sufile;
}

void Compiler::reset(){
    for(suFile& f : files){
        file_deinit(f.file);
    }
    files.clear();
    memory_clear_arena(arena.functions);
	memory_clear_arena(arena.variables);
	memory_clear_arena(arena.structs);
	memory_clear_arena(arena.scopes);
	memory_clear_arena(arena.expressions);
    memory_clear_temp();
}

