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

    compiler.logger.log(Verbosity_StageParts, "Starting compiler chain for ", CyanFormatComma(ct->amufile->file->name));

    //TODO(sushi) support for passing each stages file and starting compilation from stages passed lexing if 
    //            necessary
    if(ct->stage > FileStage_Null){
        compiler.start_lexer(ct->amufile);
    }
    if(ct->stage > FileStage_Lexer){
        compiler.start_preprocessor(ct->amufile);    
    }
    if(ct->stage > FileStage_Preprocessor){
        compiler.start_parser(ct->amufile);
    }
    if(ct->stage > FileStage_Parser){
        compiler.start_validator(ct->amufile);
    }

    ct->finished = 1;
    ct->wait.notify_all();
    //memzfree(ct);
}

void Compiler::init(){DPZoneScoped;
    compiler.logger.owner_str_if_sufile_is_0 = STR8("compiler");
    compiler.mutexes.log.init();
    compiler.mutexes.preprocessor.init();
    compiler.mutexes.parser.init();
    compiler.mutexes.lexer.init();
    compiler.mutexes.compile_request.init();
    
    Expression* caste; //casting expression
    Function*   castf; //casting function
    
    forI(ArrayCount(builtin.types.arr)){
        builtin.types.arr[i] = arena.make_struct();
        builtin.types.arr[i]->conversions.init();
        builtin.types.arr[i]->members.init();
    } 

    builtin.types.unsigned8 ->decl.identifier = STR8("u8");
    builtin.types.unsigned16->decl.identifier = STR8("u16");
    builtin.types.unsigned32->decl.identifier = STR8("u32");
    builtin.types.unsigned64->decl.identifier = STR8("u64");
    builtin.types.signed8   ->decl.identifier = STR8("s8");
    builtin.types.signed16  ->decl.identifier = STR8("s16");
    builtin.types.signed32  ->decl.identifier = STR8("s32");
    builtin.types.signed64  ->decl.identifier = STR8("s64");
    builtin.types.float32   ->decl.identifier = STR8("f32");
    builtin.types.float64   ->decl.identifier = STR8("f64");
    builtin.types.ptr       ->decl.identifier = STR8("ptr");
    builtin.types.any       ->decl.identifier = STR8("any");
    builtin.types.str       ->decl.identifier = STR8("str");

    builtin.types.unsigned8 ->type = DataType_Unsigned8;
    builtin.types.unsigned16->type = DataType_Unsigned16;
    builtin.types.unsigned32->type = DataType_Unsigned32;
    builtin.types.unsigned64->type = DataType_Unsigned64;
    builtin.types.signed8   ->type = DataType_Signed8;
    builtin.types.signed16  ->type = DataType_Signed16;
    builtin.types.signed32  ->type = DataType_Signed32;
    builtin.types.signed64  ->type = DataType_Signed64;
    builtin.types.float32   ->type = DataType_Float32;
    builtin.types.float64   ->type = DataType_Float64;
    builtin.types.ptr       ->type = DataType_Ptr;
    builtin.types.any       ->type = DataType_Any;
    builtin.types.str       ->type = DataType_String;

    builtin.types.unsigned8 ->size = sizeof(u8);
    builtin.types.unsigned16->size = sizeof(u16);
    builtin.types.unsigned32->size = sizeof(u32);
    builtin.types.unsigned64->size = sizeof(u64);
    builtin.types.signed8   ->size = sizeof(s8);
    builtin.types.signed16  ->size = sizeof(s16);
    builtin.types.signed32  ->size = sizeof(s32);
    builtin.types.signed64  ->size = sizeof(s64);
    builtin.types.float32   ->size = sizeof(f32);
    builtin.types.float64   ->size = sizeof(f64);
    builtin.types.ptr       ->size = sizeof(void*);
    builtin.types.any       ->size = sizeof(void*); //pointer to type information, probably?
    builtin.types.str       ->size = sizeof(u8*) + sizeof(u64); //pointer to string and a byte count

    //generate conversions between built in scalar types
    for(Struct* i = builtin.types.unsigned8; i != builtin.types.any; i++){
        for(Struct* j = builtin.types.unsigned8; j != builtin.types.any; j++){
            if(i==j)continue;
            Expression* e = arena.make_expression(STR8("cast"));
            e->type = Expression_CastImplicit;
            e->data.structure = j;
            i->conversions.add(j->decl.identifier, &e->node);
        }
    }

    //make any's member
    //TODO(sushi) make any's members when we figure out how that will work 

    //make str's members
    {
        Variable* ptr = arena.make_variable(STR8("ptr"));
        ptr->decl.identifier = STR8("ptr");
        ptr->decl.declared_identifier = ptr->decl.identifier;
        ptr->data.structure = builtin.types.ptr;
        Variable* count = arena.make_variable(STR8("count"));
        count->decl.identifier = STR8("count");
        count->decl.declared_identifier = count->decl.identifier;
        count->data.structure = builtin.types.unsigned64;

        builtin.types.str->members.add(ptr->decl.identifier, &ptr->decl.node);
        builtin.types.str->members.add(count->decl.identifier, &count->decl.node);
    }
}

amuArena<amuFile*> Compiler::compile(CompilerRequest* request, b32 wait){DPZoneScoped;
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

    amuArena<amuFile*> out;
    out.init();

    forI(request->filepaths.count){
        str8 filepath = request->filepaths[i];
        //TODO(sushi) it may be better to just interate the string backwards and look for / or \ instead
        u32 last_slash = str8_find_last(filepath, '/');
        if(last_slash == npos) last_slash = str8_find_last(filepath, '\\');
        if(last_slash == npos) last_slash = 0;
        str8 filename = str8{filepath.str+last_slash+1, filepath.count-(last_slash+1)};

        amuFile* amufile = files.atPtrVal(filename);

        //make a new amuFile if it doesnt exist already
        if(!amufile){
            logger.log(Verbosity_StageParts, CyanFormatComma(filename), " has not been loaded yet, making a new amuFile.");
            File* file = file_init(filepath, FileAccess_Read);
            if(!file){
                logger.error("Unable to open file at path ", filepath);
                mutexes.compile_request.unlock();
                return out;
            } 

            //we must allocate these somewhere because just storing them in map will move them if we have too many
            //invalidating all of the pointers to them
            amufile = (amuFile*)memalloc(sizeof(amuFile));
            amufile->file = file;
            amufile->stage = FileStage_Null;
            amufile->init();
            amufile->logger.amufile = amufile;

            files.add(filename, amufile);
            out.add(amufile);
        }else{
            out.add(amufile);
            //its possible that a file is actually already being processed by another thread, such as when multiple files
            //import the same file, so we just skip it if it is
            //TODO(sushi) this may not be as simple as this. we may also have to check if the requested stage here
            //            is higher than the one originally requested and change it somehow.
            if(amufile->being_processed) 
                continue;
        }

        amufile->being_processed = 1;
        ct[i].wait.init();
        ct[i].filepath = request->filepaths[i];
        ct[i].stage = request->stage;
        ct[i].amufile = amufile;
        DeshThreadManager->add_job({&compile_threaded_func, &ct[i]});
    }

    mutexes.compile_request.unlock();

    DPTracyMessageL("compiler: waking threads");
    DeshThreadManager->wake_threads(request->filepaths.count);

    if(wait){
        SetThreadName("Waiting on compiler threads to finish");
        forI(request->filepaths.count){
            if(!ct[i].amufile) continue; //this happens when the file was already being processed and we skipped that slot
            while(!ct[i].finished){
                ct[i].wait.wait();
            }
            ct[i].wait.deinit();
            ct[i].amufile->being_processed = 0;
        }
    }
    return out;
}

amuFile* Compiler::start_lexer(amuFile* amufile){DPZoneScoped;
    Assert(amufile, "Compiler::start_lexer was passed a null amuFile*");
    SetThreadName("Starting a lexer");

    logger.log(Verbosity_StageParts, "Starting a lexer for ", CyanFormatComma(amufile->file->name));
    logger.log(Verbosity_StageParts, "Checking if ", CyanFormatComma(amufile->file->name), " has already been lexed");
	if(amufile->stage >= FileStage_Lexer){
		logger.log(Verbosity_StageParts, SuccessFormatComma(CyanFormatComma(amufile->file->name), " has already been lexed."));
		return amufile;
	}

    global_mem_lock.lock();
    Lexer* lexer = (Lexer*)memalloc(sizeof(Lexer));
    global_mem_lock.unlock();
	
    logger.log(Verbosity_StageParts, "Reading file contents of ", CyanFormatComma(amufile->file->name), " into a buffer");
    amufile->file_buffer = file_read_alloc(amufile->file, amufile->file->bytes, deshi_temp_allocator); 
    lexer->amufile = amufile;
    lexer->lex();

    amufile->stage = FileStage_Lexer;

    memzfree(lexer);
    return amufile;
}

amuFile* Compiler::start_preprocessor(amuFile* amufile){DPZoneScoped;
    Assert(amufile, "Compiler::start_preprocessor was passed a null amuFile*");
    Assert(amufile->stage >= FileStage_Lexer, "Compiler::start_preprocessor was given a amufile that has not completed previous stages.");

    SetThreadName("Starting a preprocessor");

    logger.log(Verbosity_StageParts, "Starting a preprocessor for ", CyanFormatComma(amufile->file->name));
    logger.log(Verbosity_StageParts, "Checking if ", CyanFormatComma(amufile->file->name), " has already been preprocessed.");
    if(amufile->stage >= FileStage_Preprocessor){
        logger.log(Verbosity_StageParts, SuccessFormatComma(CyanFormatComma(amufile->file->name), " has already been preprocessed."));
        return amufile;
    } 

    global_mem_lock.lock();
    Preprocessor* preprocessor = (Preprocessor*)memalloc(sizeof(Preprocessor)); 
    global_mem_lock.unlock();        

    preprocessor->amufile = amufile;
    preprocessor->preprocess();

    amufile->stage = FileStage_Preprocessor;

    memzfree(preprocessor);
    return amufile;
}

amuFile* Compiler::start_parser(amuFile* amufile){DPZoneScoped;
    Assert(amufile, "Compiler::start_preprocessor was passed a null amuFile*");
    Assert(amufile->stage >= FileStage_Lexer, "Compiler::start_parser was given a amufile that has not completed previous stages.");

    SetThreadName("Starting a parser");

    logger.log(Verbosity_StageParts, "Starting a parser for ", CyanFormatComma(amufile->file->name));
    logger.log(Verbosity_StageParts, "Checking if ", CyanFormatComma(amufile->file->name), " has already been parsed");
    if(amufile->stage >= FileStage_Parser){
        logger.log(Verbosity_StageParts, SuccessFormatComma(CyanFormatComma(amufile->file->name), " has already been parsed."));
        return amufile;
    }
    
    global_mem_lock.lock();
    Parser* parser = (Parser*)memalloc(sizeof(Parser)); 
    global_mem_lock.unlock();
    
    parser->amufile = amufile;
    parser->parse();

    amufile->stage = FileStage_Parser;

    memzfree(parser);
    return amufile;
}

amuFile* Compiler::start_validator(amuFile* amufile){
    Assert(amufile, "Compiler::start_validator was passed a null amuFile*.");
    Assert(amufile->stage >= FileStage_Parser, "Compiler::start_validator was given a amufile that has not completed previous stages.");

    SetThreadName("Starting a validator");

    logger.log(Verbosity_StageParts, "Starting a validator for ", CyanFormatComma(amufile->file->name));
    logger.log(Verbosity_StageParts, "Checking if ", CyanFormatComma(amufile->file->name), " has already been validated.");
    if(amufile->stage >= FileStage_Validator){
        logger.log(Verbosity_StageParts, SuccessFormatComma(CyanFormatComma(amufile->file->name), " has already been validated."));
        return amufile;
    }

    global_mem_lock.lock();
    Validator* validator = (Validator*)memalloc(sizeof(Validator));
    global_mem_lock.unlock();

    validator->amufile = amufile;
    validator->init();
    validator->start();

    memzfree(validator);
    return amufile;
}

void Compiler::reset(){
    for(amuFile* f : files){
        file_deinit(f->file);
    }
    files.clear();

}

