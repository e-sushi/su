/*

    The central object of the compiler, manages files, execution of stages on those files,
    stores handles to storage of anything in amu, etc.
    

*/

#ifndef AMU_COMPILER_H
#define AMU_COMPILER_H

namespace deshi {
    #include "core/time.h"
    #include "core/threading.h"
}

#include "Messenger.h"
#include "storage/Pool.h"
#include "Entity.h"
#include "Result.h"
#include "Lexer.h"
#include "Parser.h"


namespace amu {

struct Compiler {
    Stopwatch compiler_time;

    // when we use deshi's memory system, we have to lock it 
    // because it is not yet internally thread safe
    // this should primarily be used by amu::Arena
    mutex deshi_mem_lock;

    File* log_file;

    // in order to make cleaning up the compiler easy, storage of all things in amu 
    // have their handle inside the compiler struct
    // this way when the compiler needs to be deinitialized, it doesn't need to ask other
    // things to clean up
    //
    // this is experimental and depending on how much state needs cleaned up in other 
    // components of the compiler, I may just go back to storing this locally  
    struct {
        Pool<Source> sources;
        Pool<Entity> entities;
        Pool<Label> labels;
        Pool<Lexer> lexers;
        Pool<Parser> parsers;
        Pool<Statement> statements;
        Pool<Tuple> tuples;
        Pool<Expression> expressions;
    }storage;
    

    struct {
        u32 verbosity;
        b32 deliver_debug_immediately;
    } options;
};

namespace compiler {

// initializes the compiler, storing all information in the global amu::compiler::amu
global void
init();

global void
deinit();

global void
begin(Array<String> args);

// loads a file as an amu::Source and stores it internally
// returns a handle to the Source
global Result<Source*, Message>
load_source(String path);

// attempts to locate a given source name
// returns 0 if it doesn't exist
global Source*
lookup_source(String name);

// creates a Label and passes a handle to it
global Label*
create_label();

global Entity*
create_entity();

global Statement*
create_statement();

global Tuple*
create_tuple();

global Expression*
create_expression();

// the global compiler object, referred to by anything in the program
// later on, we may allow creating multiple instances of the Compiler
// at the moment, I don't think I want this to be used directly,
// probably best to have an interface to it, but I'll never restrict it
// from direct use
extern Compiler instance;

} // namespace compiler 
} // namespace amu

#endif // AMU_COMPILER_H