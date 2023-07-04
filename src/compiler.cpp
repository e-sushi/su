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

    defer{
        ct->finished = 1;
        condition_variable_notify_all(&ct->wait);
        // since each stage may wait on another, we must make sure that stages
        // who wait on a stage that will never be reached wake up.
        condition_variable_notify_all(&ct->amufile->cv.lex);
        condition_variable_notify_all(&ct->amufile->cv.preprocess);
        condition_variable_notify_all(&ct->amufile->cv.parse);
        condition_variable_notify_all(&ct->amufile->cv.validate);
        ct->amufile->stage = FileStage_ERROR;
    };

    if(ct->stage > FileStage_Null){
        compiler.start_lexer(ct->amufile);
        if(ct->amufile->lexical_analyzer.failed) return;
    }
    if(ct->stage > FileStage_Lexer){
        compiler.start_preprocessor(ct->amufile); 
        if(ct->amufile->preprocessor.failed) return;
    }
    if(ct->stage > FileStage_Preprocessor){
        compiler.start_parser(ct->amufile);
        if(ct->amufile->syntax_analyzer.failed) return;
    }
    if(ct->stage > FileStage_Parser){
        compiler.start_validator(ct->amufile);
        if(ct->amufile->semantic_analyzer.failed) return;
    }
}

void Compiler::init(){DPZoneScoped;
    compiler.logger.owner_str_if_sufile_is_0 = STR8("compiler");
    compiler.mutexes.log = mutex_init();
    compiler.mutexes.preprocessor = mutex_init();
    compiler.mutexes.syntax_analyzer = mutex_init();
    compiler.mutexes.lexer = mutex_init();
    compiler.mutexes.compile_request = mutex_init();
    
    Expression* caste; //casting expression
    Function*   castf; //casting function
    
    forI(ArrayCount(builtin.types.arr)){
        builtin.types.arr[i] = arena.make_struct();
        builtin.types.arr[i]->conversions.init();
        builtin.types.arr[i]->members.init();
        builtin.labels.arr[i] = arena.make_label();
    } 

    builtin.labels.void_     ->identifier = STR8("void");
    builtin.labels.unsigned8 ->identifier = STR8("u8");
    builtin.labels.unsigned16->identifier = STR8("u16");
    builtin.labels.unsigned32->identifier = STR8("u32");
    builtin.labels.unsigned64->identifier = STR8("u64");
    builtin.labels.signed8   ->identifier = STR8("s8");
    builtin.labels.signed16  ->identifier = STR8("s16");
    builtin.labels.signed32  ->identifier = STR8("s32");
    builtin.labels.signed64  ->identifier = STR8("s64");
    builtin.labels.float32   ->identifier = STR8("f32");
    builtin.labels.float64   ->identifier = STR8("f64");
    builtin.labels.ptr       ->identifier = STR8("ptr");
    builtin.labels.any       ->identifier = STR8("any");
    builtin.labels.str       ->identifier = STR8("str");

    builtin.labels.void_     ->entity = (Entity*)builtin.types.void_;
    builtin.labels.unsigned8 ->entity = (Entity*)builtin.types.unsigned8;
    builtin.labels.unsigned16->entity = (Entity*)builtin.types.unsigned16;
    builtin.labels.unsigned32->entity = (Entity*)builtin.types.unsigned32;
    builtin.labels.unsigned64->entity = (Entity*)builtin.types.unsigned64;
    builtin.labels.signed8   ->entity = (Entity*)builtin.types.signed8;
    builtin.labels.signed16  ->entity = (Entity*)builtin.types.signed16;
    builtin.labels.signed32  ->entity = (Entity*)builtin.types.signed32;
    builtin.labels.signed64  ->entity = (Entity*)builtin.types.signed64;
    builtin.labels.float32   ->entity = (Entity*)builtin.types.float32;
    builtin.labels.float64   ->entity = (Entity*)builtin.types.float64;
    builtin.labels.ptr       ->entity = (Entity*)builtin.types.ptr;
    builtin.labels.any       ->entity = (Entity*)builtin.types.any;
    builtin.labels.str       ->entity = (Entity*)builtin.types.str;

    builtin.types.void_     ->entity.label = builtin.labels.void_;
    builtin.types.unsigned8 ->entity.label = builtin.labels.unsigned8;
    builtin.types.unsigned16->entity.label = builtin.labels.unsigned16;
    builtin.types.unsigned32->entity.label = builtin.labels.unsigned32;
    builtin.types.unsigned64->entity.label = builtin.labels.unsigned64;
    builtin.types.signed8   ->entity.label = builtin.labels.signed8;
    builtin.types.signed16  ->entity.label = builtin.labels.signed16;
    builtin.types.signed32  ->entity.label = builtin.labels.signed32;
    builtin.types.signed64  ->entity.label = builtin.labels.signed64;
    builtin.types.float32   ->entity.label = builtin.labels.float32;
    builtin.types.float64   ->entity.label = builtin.labels.float64;
    builtin.types.ptr       ->entity.label = builtin.labels.ptr;
    builtin.types.any       ->entity.label = builtin.labels.any;
    builtin.types.str       ->entity.label = builtin.labels.str;

    builtin.types.void_     ->type = DataType_Void;
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

    builtin.types.void_     ->size = 0;
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
            i->conversions.add(j->entity.label->identifier, &e->node);
        }
    }

    //TODO(sushi) generate array information when we get around to it
}

CompilerReport Compiler::compile(CompilerRequest* request, b32 wait){DPZoneScoped;

    //we must lock this function because it is possible it is called from 2 different threads at the same time requesting the same file.
    mutex_lock(&mutexes.compile_request);

    logger.log(Verbosity_StageParts, "Beginning compiler request on ", request->filepaths.count, (request->filepaths.count == 1 ? " file." : " files."));

    SetThreadName("Beginning compiler request on ", request->filepaths.count, (request->filepaths.count == 1 ? " file." : " files."));

    if(globals.verbosity == Verbosity_Debug){
        for(str8 s : request->filepaths){
            logger.log(Verbosity_Debug, "Request to compile path ", s);
        }
    }

    CompilerThread* ct = (CompilerThread*)memtalloc(sizeof(CompilerThread) * request->filepaths.count);

    CompilerReport report = {0};
    report.units.init();

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
                mutex_unlock(&mutexes.compile_request);
                return report;
            } 

            //we must allocate these somewhere because just storing them in map will move them if we have too many
            //invalidating all of the pointers to them
            amufile = (amuFile*)memalloc(sizeof(amuFile));
            amufile->file = file;
            amufile->stage = FileStage_Null;
            amufile->init();
            amufile->logger.amufile = amufile;

            files.add(filename, amufile);
            report.units.add(amufile);
        }else{
            report.units.add(amufile);
            //its possible that a file is actually already being processed by another thread, such as when multiple files
            //import the same file, so we just skip it if it is
            //TODO(sushi) this may not be as simple as this. we may also have to check if the requested stage here
            //            is higher than the one originally requested and change it somehow.
            if(amufile->being_processed) 
                continue;
        }

        amufile->being_processed = 1;
        ct[i].wait = condition_variable_init();
        ct[i].filepath = request->filepaths[i];
        ct[i].stage = request->stage;
        ct[i].amufile = amufile;
        threader_add_job({&compile_threaded_func, &ct[i]});
    }

    mutex_unlock(&mutexes.compile_request);

    DPTracyMessageL("compiler: waking threads");
    threader_wake_threads(request->filepaths.count);

    if(wait){
        SetThreadName("Waiting on compiler threads to finish");
        forI(request->filepaths.count){
            if(!ct[i].amufile) continue; //this happens when the file was already being processed and we skipped that slot
            while(!ct[i].finished){
                condition_variable_wait(&ct[i].wait);
                if(ct->amufile->stage == FileStage_ERROR){
                    report.failed = 1;
                }
            }
            condition_variable_deinit(&ct[i].wait);
            ct[i].amufile->being_processed = 0;
        }
    }
    return report;
}

amuFile* Compiler::start_lexer(amuFile* amufile){DPZoneScoped;
    Assert(amufile, "Compiler::start_lexer was passed a null amuFile*");
    SetThreadName("Starting a lexical_analyzer");

    logger.log(Verbosity_StageParts, "Starting a lexical_analyzer for ", CyanFormatComma(amufile->file->name));
    logger.log(Verbosity_StageParts, "Checking if ", CyanFormatComma(amufile->file->name), " has already been lexed");
	if(amufile->stage >= FileStage_Lexer){
		logger.log(Verbosity_StageParts, SuccessFormatComma(CyanFormatComma(amufile->file->name), " has already been lexed."));
		return amufile;
	}

    mutex_lock(&global_mem_lock);
    Lexer* lexical_analyzer = (Lexer*)memalloc(sizeof(Lexer));
    mutex_unlock(&global_mem_lock);
	
    logger.log(Verbosity_StageParts, "Reading file contents of ", CyanFormatComma(amufile->file->name), " into a buffer");
    amufile->file_buffer = file_read_alloc(amufile->file, amufile->file->bytes, deshi_temp_allocator); 
    lexical_analyzer->amufile = amufile;
    lexical_analyzer->lex();

    amufile->stage = FileStage_Lexer;

    memzfree(lexical_analyzer);
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

    mutex_lock(&global_mem_lock);
    Preprocessor* preprocessor = (Preprocessor*)memalloc(sizeof(Preprocessor)); 
    mutex_unlock(&global_mem_lock);       

    preprocessor->amufile = amufile;
    preprocessor->preprocess();

    amufile->stage = FileStage_Preprocessor;

    memzfree(preprocessor);

    return amufile;
}

amuFile* Compiler::start_parser(amuFile* amufile){DPZoneScoped;
    Assert(amufile, "Compiler::start_preprocessor was passed a null amuFile*");
    Assert(amufile->stage >= FileStage_Lexer, "Compiler::start_parser was given a amufile that has not completed previous stages.");

    SetThreadName("Starting a syntax_analyzer");

    logger.log(Verbosity_StageParts, "Starting a syntax_analyzer for ", CyanFormatComma(amufile->file->name));
    logger.log(Verbosity_StageParts, "Checking if ", CyanFormatComma(amufile->file->name), " has already been parsed");
    if(amufile->stage >= FileStage_Parser){
        logger.log(Verbosity_StageParts, SuccessFormatComma(CyanFormatComma(amufile->file->name), " has already been parsed."));
        return amufile;
    }
    
    mutex_lock(&global_mem_lock);
    SyntaxAnalyzer* syntax_analyzer = (SyntaxAnalyzer*)memalloc(sizeof(SyntaxAnalyzer)); 
    mutex_unlock(&global_mem_lock);
    
    syntax_analyzer->amufile = amufile;
    syntax_analyzer->analyze();

    amufile->stage = FileStage_Parser;
    memzfree(syntax_analyzer);

    return amufile;
}

amuFile* Compiler::start_validator(amuFile* amufile){
    Assert(amufile, "Compiler::start_validator was passed a null amuFile*.");
    Assert(amufile->stage >= FileStage_Parser, "Compiler::start_validator was given a amufile that has not completed previous stages.");

    SetThreadName("Starting a semantic_analyzer");

    logger.log(Verbosity_StageParts, "Starting a semantic_analyzer for ", CyanFormatComma(amufile->file->name));
    logger.log(Verbosity_StageParts, "Checking if ", CyanFormatComma(amufile->file->name), " has already been validated.");
    if(amufile->stage >= FileStage_Validator){
        logger.log(Verbosity_StageParts, SuccessFormatComma(CyanFormatComma(amufile->file->name), " has already been validated."));
        return amufile;
    }

    mutex_lock(&global_mem_lock);
    SemanticAnalyzer* semantic_analyzer = (SemanticAnalyzer*)memalloc(sizeof(SemanticAnalyzer));
    mutex_unlock(&global_mem_lock);

    semantic_analyzer->amufile = amufile;
    semantic_analyzer->init();
    semantic_analyzer->start();

    memzfree(semantic_analyzer);
    return amufile;
}

void Compiler::reset(){
    for(amuFile* f : files){
        file_deinit(f->file);
    }
    files.clear();

}