/*

    The central object of the compiler, manages files, execution of stages on those files,
    stores handles to storage of anything in amu, etc.
    

*/

#ifndef AMU_COMPILER_H
#define AMU_COMPILER_H

#include "Messenger.h"
#include "storage/Pool.h"
#include "Entity.h"
#include "Result.h"
#include "Lexer.h"
#include "Parser.h"


namespace amu {

struct Compiler {
    util::Stopwatch compiler_time;

    FILE* log_file;

    // in order to make cleaning up the compiler easy, storage of all things in amu 
    // have their handle inside the compiler struct
    // this way when the compiler needs to be deinitialized, it doesn't need to ask other
    // things to clean up
    //
    // this is experimental and depending on how much state needs cleaned up in other 
    // components of the compiler, I may just go back to storing this locally  
    struct {
        Pool<Source>     sources;
        Pool<Lexer>      lexers;
        Pool<Parser>     parsers;
        Pool<Module>     modules;
        Pool<Label>      labels;
        Pool<Structure>  structures;
        Pool<Function>   functions;
        Pool<Statement>  statements;
        Pool<Expression> expressions;
        Pool<Place>      places;
        Pool<Tuple>      tuples;
        Pool<Type>       types;
    }storage;

    Array<Diagnostic> diagnostics;
    
    // anything that is specified on the command line
    struct {
        String entry_path;

        u32 verbosity;
        b32 deliver_debug_immediately;

        b32 quiet;
        
        struct{
            String path;
            b32 exit;
            b32 human;
        }dump_tokens;

        struct{
            String path;
            Array<String> sources;
        }dump_diagnostics; 

    } options;
};

namespace compiler {

global struct{
    Structure* void_;
    Structure* unsigned8;
    Structure* unsigned16;
    Structure* unsigned32;
    Structure* unsigned64;
    Structure* signed8;
    Structure* signed16;
    Structure* signed32;
    Structure* signed64;
    Structure* float32;
    Structure* float64;
    Structure* array;
    Structure* darray;
    Structure* functype; // the internal rep for a function 'type', which is just a function pointer 
}builtins;

// initializes the compiler, storing all information in the global amu::compiler::instance
global void
init();

global void
deinit();

global void
begin(Array<String> args);

// the global compiler object, referred to by anything in the program
// later on, we may allow creating multiple instances of the Compiler
// at the moment, I don't think I want this to be used directly,
// probably best to have an interface to it, but I'll never restrict it
// from direct use
extern Compiler instance;

} // namespace compiler 
} // namespace amu

#endif // AMU_COMPILER_H