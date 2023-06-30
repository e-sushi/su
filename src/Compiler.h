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

namespace amu {

struct Compiler {
    Stopwatch compiler_time;

    // when we use deshi's memory system, we have to lock it 
    // because it is not yet internally thread safe
    // this should primarily be used by amu::Arena
    mutex deshi_mem_lock;

    Messenger messenger;

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
    }storage;
};

namespace compiler {

// initializes the compiler, storing all information in the global amu::compiler::amu
global void
init();

global void
deinit();

// loads a file as an amu::Source and stores it internally
// returns a handle to the Source
global Source*
load_source(str8 path);

// attempts to locate a given source name
// returns 0 if it doesn't exist
global Source*
lookup_source(str8 name);



// the global compiler object, referred to by anything in the program
// later on, we may allow creating multiple instances of the Compiler
// at the moment, I don't think I want this to be used directly,
// probably best to have an interface to it, but I'll never restrict it
// from direct use
extern Compiler instance;

} // namespace compiler 
} // namespace amu

#endif // AMU_COMPILER_H