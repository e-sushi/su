/*

    The central object of the compiler, manages files, execution of stages on those files,
    stores handles to storage of anything in amu, etc.
    

*/

#ifndef AMU_COMPILER_H
#define AMU_COMPILER_H

#include "systems/Messenger.h"
#include "storage/Pool.h"
#include "representations/Entity.h"
#include "processors/Lexer.h"
#include "processors/Parser.h"
#include "processors/Sema.h"
#include "processors/GenTAC.h"
#include "processors/GenAIR.h"
#include "representations/Code.h"
#include "systems/VM.h"
#include "systems/Debugger.h"

namespace amu {

struct Compiler {
    util::Stopwatch compiler_time;

    // anything that is specified on the command line
    // later on all of these things should be changable from within the language
    struct {
        String entry_path;

		Message::Kind verbosity;

        b32 deliver_debug_immediately;
		b32 deliver_all_immediately;

		b32 break_on_error;

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

	static Compiler
	create();

	void
	destroy();

	b32
	begin(Array<String> args);
};

extern Compiler compiler;

namespace compiler {

// initializes the compiler, storing all information in the global amu::compiler::instance
global void
init();

global void
deinit();

// the entry point of the compiler given a list of command line arguments
global b32
begin(Array<String> args);

// the global compiler object, referred to by anything in the program
// later on, we may allow creating multiple instances of the Compiler
// at the moment, I don't think I want this to be used directly,
// probably best to have an interface to it, but I'll never restrict it
// from direct use
extern Compiler instance;

// global compiler module, which is built up with information accessible to
// the language itself
extern Module* module;

} // namespace compiler 
} // namespace amu

#endif // AMU_COMPILER_H
