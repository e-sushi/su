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
    SetThreadName("Compiler thread started");

    compiler.logger.log(Verbosity_StageParts, "Starting compiler chain for ", CyanFormatComma(ct->sufile->file->name));

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
    if(ct->stage > FileStage_Parser){
        compiler.start_validator(ct->sufile);
    }

    ct->finished = 1;
    ct->wait.notify_all();
    //memzfree(ct);
}

suArena<suFile*> Compiler::compile(CompilerRequest* request, b32 wait){DPZoneScoped;
    logger.log(Verbosity_StageParts, "Beginning compiler request on ", request->filepaths.count, (request->filepaths.count == 1 ? " file." : " files."));

    //we must lock this function because it is possible it is called from 2 different threads at the same time requesting
    //the same file.
    mutexes.compile_request.lock();

    SetThreadName("Beginning compiler request on ", request->filepaths.count, (request->filepaths.count == 1 ? " file." : " files."));

    if(globals.verbosity == Verbosity_Debug){
        for(str8 s : request->filepaths){
            logger.log(Verbosity_Debug, "Request to compile path ", s);
        }
    }

    CompilerThread* ct = (CompilerThread*)memtalloc(sizeof(CompilerThread) * request->filepaths.count);

    suArena<suFile*> out;
    out.init();

    forI(request->filepaths.count){
        str8 filepath = request->filepaths[i];
        //TODO(sushi) it may be better to just interate the string backwards and look for / or \ instead
        u32 last_slash = str8_find_last(filepath, '/');
        if(last_slash == npos) last_slash = str8_find_last(filepath, '\\');
        if(last_slash == npos) last_slash = 0;
        str8 filename = str8{filepath.str+last_slash+1, filepath.count-(last_slash+1)};

        suFile* sufile = files.atPtrVal(filename);

        //make a new suFile if it doesnt exist already
        if(!sufile){
            logger.log(Verbosity_StageParts, CyanFormatComma(filename), " has not been loaded yet, making a new suFile.");
            File* file = file_init(filepath, FileAccess_Read);
            if(!file){
                logger.error("Unable to open file at path ", filepath);
                mutexes.compile_request.unlock();
                return out;
            } 

            //we must allocate these somewhere because just storing them in map will move them if we have too many
            //invalidating all of the pointers to them
            sufile = (suFile*)memalloc(sizeof(suFile));
            sufile->file = file;
            sufile->stage = FileStage_Null;
            sufile->init();
            sufile->logger.sufile = sufile;

            files.add(filename, sufile);
            out.add(sufile);
        }else{
            out.add(sufile);
            //its possible that a file is actually already being processed by another thread, such as when multiple files
            //import the same file, so we just skip it if it is
            //TODO(sushi) this may not be as simple as this. we may also have to check if the requested stage here
            //            is higher than the one originally requested and change it somehow.
            if(sufile->being_processed) 
                continue;
        }

        sufile->being_processed = 1;
        ct[i].wait.init();
        ct[i].filepath = request->filepaths[i];
        ct[i].stage = request->stage;
        ct[i].sufile = sufile;
        DeshThreadManager->add_job({&compile_threaded_func, &ct[i]});
    }

    mutexes.compile_request.unlock();

    DPTracyMessageL("compiler: waking threads");
    DeshThreadManager->wake_threads(request->filepaths.count);

    if(wait){
        SetThreadName("Waiting on compiler threads to finish");
        forI(request->filepaths.count){
            if(!ct[i].sufile) continue; //this happens when the file was already being processed and we skipped that slot
            while(!ct[i].finished){
                ct[i].wait.wait();
            }
            ct[i].wait.deinit();
            ct[i].sufile->being_processed = 0;
        }
    }
    return out;
}

suFile* Compiler::start_lexer(suFile* sufile){DPZoneScoped;
    Assert(sufile, "Compiler::start_lexer was passed a null suFile*");
    SetThreadName("Starting a lexer");

    logger.log(Verbosity_StageParts, "Starting a lexer for ", CyanFormatComma(sufile->file->name));
    logger.log(Verbosity_StageParts, "Checking if ", CyanFormatComma(sufile->file->name), " has already been lexed");
	if(sufile->stage >= FileStage_Lexer){
		logger.log(Verbosity_StageParts, SuccessFormatComma(CyanFormatComma(sufile->file->name), " has already been lexed."));
		return sufile;
	}

    global_mem_lock.lock();
    Lexer* lexer = (Lexer*)memalloc(sizeof(Lexer));
    global_mem_lock.unlock();
	
    logger.log(Verbosity_StageParts, "Reading file contents of ", CyanFormatComma(sufile->file->name), " into a buffer");
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

    SetThreadName("Starting a preprocessor");

    logger.log(Verbosity_StageParts, "Starting a preprocessor for ", CyanFormatComma(sufile->file->name));
    logger.log(Verbosity_StageParts, "Checking if ", CyanFormatComma(sufile->file->name), " has already been preprocessed.");
    if(sufile->stage >= FileStage_Preprocessor){
        logger.log(Verbosity_StageParts, SuccessFormatComma(CyanFormatComma(sufile->file->name), " has already been preprocessed."));
        return sufile;
    } 

    global_mem_lock.lock();
    Preprocessor* preprocessor = (Preprocessor*)memalloc(sizeof(Preprocessor)); 
    global_mem_lock.unlock();        

    preprocessor->sufile = sufile;
    preprocessor->preprocess();

    sufile->stage = FileStage_Preprocessor;

    memzfree(preprocessor);
    return sufile;
}

suFile* Compiler::start_parser(suFile* sufile){DPZoneScoped;
    Assert(sufile, "Compiler::start_preprocessor was passed a null suFile*");
    Assert(sufile->stage >= FileStage_Lexer, "Compiler::start_parser was given a sufile that has not completed previous stages.");

    SetThreadName("Starting a parser");

    logger.log(Verbosity_StageParts, "Starting a parser for ", CyanFormatComma(sufile->file->name));
    logger.log(Verbosity_StageParts, "Checking if ", CyanFormatComma(sufile->file->name), " has already been parsed");
    if(sufile->stage >= FileStage_Parser){
        logger.log(Verbosity_StageParts, SuccessFormatComma(CyanFormatComma(sufile->file->name), " has already been parsed."));
        return sufile;
    }
    
    global_mem_lock.lock();
    Parser* parser = (Parser*)memalloc(sizeof(Parser)); 
    global_mem_lock.unlock();
    
    parser->sufile = sufile;
    parser->parse();

    sufile->stage = FileStage_Parser;

    memzfree(parser);
    return sufile;
}

suFile* Compiler::start_validator(suFile* sufile){
    Assert(sufile, "Compiler::start_validator was passed a null suFile*.");
    Assert(sufile->stage >= FileStage_Parser, "Compiler::start_validator was given a sufile that has not completed previous stages.");

    SetThreadName("Starting a validator");

    logger.log(Verbosity_StageParts, "Starting a validator for ", CyanFormatComma(sufile->file->name));
    logger.log(Verbosity_StageParts, "Checking if ", CyanFormatComma(sufile->file->name), " has already been validated.");
    if(sufile->stage >= FileStage_Validator){
        logger.log(Verbosity_StageParts, SuccessFormatComma(CyanFormatComma(sufile->file->name), " has already been validated."));
        return sufile;
    }

    global_mem_lock.lock();
    Validator* validator = (Validator*)memalloc(sizeof(Validator));
    global_mem_lock.unlock();

    validator->sufile = sufile;
    validator->init();
    validator->start();

    memzfree(validator);
    return sufile;
}

void Compiler::reset(){
    for(suFile* f : files){
        file_deinit(f->file);
    }
    files.clear();

}

